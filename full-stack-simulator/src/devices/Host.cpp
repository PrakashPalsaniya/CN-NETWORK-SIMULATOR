#include "devices/Host.h"

#include "core/Logger.h"
#include "physical/Link.h"

namespace {

bool isBroadcastMac(const std::string& mac) {
    return mac == "FF:FF:FF:FF:FF:FF";
}

std::string senderWindowKey(
    const std::string& remoteIp,
    int remotePort,
    int localPort,
    int messageId) {
    return messageKey(remoteIp, remotePort, localPort, messageId);
}

}

Host::Host(const std::string& name) : Device(name, "Host") {
}

void Host::setDefaultGateway(const std::string& gatewayIp) {
    defaultGatewayIp_ = gatewayIp;
}

void Host::addNeighbor(const std::string& ip, const std::string& mac) {
    neighborMacs_[ip] = mac;
}

void Host::installService(const std::shared_ptr<ApplicationService>& service) {
    services_[service->wellKnownPort()] = service;
    Logger::info("APPLICATION", name() + " binds " + service->name() + " to port " + std::to_string(service->wellKnownPort()) + ".");
}

bool Host::isSameSubnet(const std::string& ipA, const std::string& ipB) const {
    const auto a = ipA.substr(0, ipA.find_last_of('.'));
    const auto b = ipB.substr(0, ipB.find_last_of('.'));
    return a == b;
}

std::vector<std::string> Host::splitPayload(const std::string& payload) const {
    constexpr std::size_t chunkSize = 18;
    std::vector<std::string> chunks;
    for (std::size_t index = 0; index < payload.size(); index += chunkSize) {
        chunks.push_back(payload.substr(index, chunkSize));
    }
    if (chunks.empty()) {
        chunks.push_back("");
    }
    return chunks;
}

bool Host::resolveMac(Port* port, const std::string& nextHopIp, std::string& resolvedMac) {
    const auto cached = neighborMacs_.find(nextHopIp);
    if (cached != neighborMacs_.end()) {
        resolvedMac = cached->second;
        Logger::info("NETWORK", name() + " uses ARP cache entry for " + nextHopIp + " -> " + resolvedMac + ".");
        return true;
    }

    sendArpRequest(port, nextHopIp);
    const auto refreshed = neighborMacs_.find(nextHopIp);
    if (refreshed == neighborMacs_.end()) {
        return false;
    }

    resolvedMac = refreshed->second;
    return true;
}

void Host::sendArpRequest(Port* port, const std::string& targetIp) {
    EthernetFrame frame;
    frame.kind = FrameKind::ArpRequest;
    frame.srcMac = port->mac;
    frame.dstMac = "FF:FF:FF:FF:FF:FF";
    frame.arp.senderIp = port->ip;
    frame.arp.senderMac = port->mac;
    frame.arp.targetIp = targetIp;

    Logger::info("NETWORK", name() + " broadcasts ARP request for " + targetIp + ".");
    sendOut(port, frame);
}

void Host::sendArpReply(Port* port, const ArpMessage& request) {
    EthernetFrame frame;
    frame.kind = FrameKind::ArpReply;
    frame.srcMac = port->mac;
    frame.dstMac = request.senderMac;
    frame.arp.senderIp = port->ip;
    frame.arp.senderMac = port->mac;
    frame.arp.targetIp = request.senderIp;
    frame.arp.targetMac = request.senderMac;

    Logger::info("NETWORK", name() + " replies to ARP for " + request.senderIp + ".");
    sendOut(port, frame);
}

void Host::sendApplication(
    const std::string& service,
    const std::string& destinationIp,
    int destinationPort,
    const std::string& payload) {
    Logger::section(name() + " Application Request");
    Logger::data("APPLICATION", "service", service);
    Logger::data("APPLICATION", "destination", destinationIp + ":" + std::to_string(destinationPort));
    Logger::data("APPLICATION", "payload", payload);

    const int sourcePort = nextEphemeralPort_++;
    Logger::info("TRANSPORT", name() + " allocates ephemeral source port " + std::to_string(sourcePort) + ".");
    sendTransport(destinationIp, sourcePort, destinationPort, service, payload);
}

void Host::sendTransport(
    const std::string& destinationIp,
    int sourcePort,
    int destinationPort,
    const std::string& service,
    const std::string& payload) {
    Port* port = ports_.empty() ? nullptr : ports_.front().get();
    if (port == nullptr) {
        Logger::warn("PHYSICAL", name() + " has no interface to send from.");
        return;
    }

    const auto nextHopIp = isSameSubnet(port->ip, destinationIp) ? destinationIp : defaultGatewayIp_;
    std::string destinationMac;
    if (!resolveMac(port, nextHopIp, destinationMac)) {
        Logger::warn("NETWORK", name() + " has no next-hop MAC entry for " + nextHopIp + ".");
        return;
    }

    const auto chunks = splitPayload(payload);
    const int messageId = nextMessageId_++;
    const std::string windowKey = senderWindowKey(destinationIp, destinationPort, sourcePort, messageId);
    auto [windowIt, inserted] = senderWindows_.emplace(windowKey, GoBackNWindow(4));
    if (inserted) {
        Logger::data("TRANSPORT", "window", windowIt->second.snapshot());
    }

    for (std::size_t index = 0; index < chunks.size(); ++index) {
        if (!windowIt->second.canSend()) {
            Logger::warn("TRANSPORT", "Window full. Transmission halted.");
            return;
        }

        TransportSegment segment;
        segment.srcPort = sourcePort;
        segment.dstPort = destinationPort;
        segment.seqNumber = static_cast<int>(index);
        segment.messageId = messageId;
        segment.isAck = false;
        segment.isLastChunk = index + 1 == chunks.size();
        segment.service = service;
        segment.payload = chunks[index];

        windowIt->second.onSend(segment.seqNumber);
        Logger::info("TRANSPORT", name() + " sends segment seq=" + std::to_string(segment.seqNumber) + " with state " + windowIt->second.snapshot() + ".");

        NetworkPacket packet;
        packet.srcIp = port->ip;
        packet.dstIp = destinationIp;
        packet.hopTrace.push_back(name());
        packet.segment = segment;

        EthernetFrame frame;
        frame.kind = FrameKind::Data;
        frame.srcMac = port->mac;
        frame.dstMac = destinationMac;
        frame.packet = packet;
        frame.payloadBits = textToBits(packetSignature(packet));
        frame.parityBit = parity_.compute(frame.payloadBits);

        Logger::data("DATA-LINK", "parity", std::to_string(frame.parityBit));
        if (!access_.requestSend(port->link ? port->link->name() : "disconnected", name())) {
            return;
        }

        sendOut(port, frame);
        Logger::info("TRANSPORT", name() + " sender window state is now " + windowIt->second.snapshot() + ".");
    }
}

void Host::sendAck(
    const std::string& destinationIp,
    int sourcePort,
    int destinationPort,
    int ackNumber,
    int messageId) {
    Port* port = ports_.empty() ? nullptr : ports_.front().get();
    if (port == nullptr) {
        return;
    }

    const auto nextHopIp = isSameSubnet(port->ip, destinationIp) ? destinationIp : defaultGatewayIp_;
    std::string destinationMac;
    if (!resolveMac(port, nextHopIp, destinationMac)) {
        Logger::warn("NETWORK", name() + " cannot send ACK because next-hop MAC is missing.");
        return;
    }

    TransportSegment segment;
    segment.srcPort = sourcePort;
    segment.dstPort = destinationPort;
    segment.seqNumber = 0;
    segment.ackNumber = ackNumber;
    segment.messageId = messageId;
    segment.isAck = true;
    segment.service = "ack";
    segment.payload = "ACK";

    NetworkPacket packet;
    packet.srcIp = port->ip;
    packet.dstIp = destinationIp;
    packet.hopTrace.push_back(name());
    packet.segment = segment;

    EthernetFrame frame;
    frame.kind = FrameKind::Data;
    frame.srcMac = port->mac;
    frame.dstMac = destinationMac;
    frame.packet = packet;
    frame.payloadBits = textToBits(packetSignature(packet));
    frame.parityBit = parity_.compute(frame.payloadBits);

    Logger::info("TRANSPORT", name() + " sends ACK " + std::to_string(ackNumber) + " to " + destinationIp + ".");
    sendOut(port, frame);
}

void Host::receiveFrame(Port* ingress, const EthernetFrame& frame) {
    if (frame.kind == FrameKind::ArpRequest) {
        neighborMacs_[frame.arp.senderIp] = frame.arp.senderMac;
        if (frame.arp.targetIp == ingress->ip) {
            sendArpReply(ingress, frame.arp);
        }
        return;
    }

    if (frame.kind == FrameKind::ArpReply) {
        neighborMacs_[frame.arp.senderIp] = frame.arp.senderMac;
        Logger::info("NETWORK", name() + " learns ARP mapping " + frame.arp.senderIp + " -> " + frame.arp.senderMac + ".");
        return;
    }

    if (frame.kind != FrameKind::Data) {
        return;
    }

    if (!isBroadcastMac(frame.dstMac) && frame.dstMac != ingress->mac) {
        Logger::info("DATA-LINK", name() + " ignores frame not addressed to its MAC.");
        return;
    }

    if (!parity_.validate(frame.payloadBits, frame.parityBit)) {
        Logger::warn("DATA-LINK", name() + " drops the frame because parity validation failed.");
        return;
    }

    Logger::info("DATA-LINK", name() + " accepts the frame and decapsulates it.");
    const NetworkPacket& packet = frame.packet;
    if (packet.dstIp != ingress->ip) {
        Logger::info("NETWORK", name() + " discards packet because IP destination does not match.");
        return;
    }

    if (packet.segment.isAck) {
        const std::string windowKey = senderWindowKey(
            packet.srcIp,
            packet.segment.srcPort,
            packet.segment.dstPort,
            packet.segment.messageId);
        auto it = senderWindows_.find(windowKey);
        if (it != senderWindows_.end()) {
            it->second.onAck(packet.segment.ackNumber);
            Logger::info("TRANSPORT", name() + " receives ACK " + std::to_string(packet.segment.ackNumber) + " with state " + it->second.snapshot() + ".");
        } else {
            Logger::info("TRANSPORT", name() + " receives ACK " + std::to_string(packet.segment.ackNumber) + ".");
        }
        return;
    }

    Logger::info(
        "TRANSPORT",
        name() + " receives segment seq=" + std::to_string(packet.segment.seqNumber) + " for port " +
            std::to_string(packet.segment.dstPort) + ".");

    const std::string key = messageKey(packet.srcIp, packet.segment.srcPort, packet.segment.dstPort, packet.segment.messageId);
    auto& reassembly = reassembly_[key];
    reassembly.data += packet.segment.payload;
    reassembly.complete = packet.segment.isLastChunk;

    sendAck(packet.srcIp, packet.segment.dstPort, packet.segment.srcPort, packet.segment.seqNumber, packet.segment.messageId);

    if (!reassembly.complete) {
        return;
    }

    ApplicationMessage message;
    message.service = packet.segment.service;
    message.payload = reassembly.data;
    reassembly_.erase(key);

    Logger::data("APPLICATION", "service", message.service);
    Logger::data("APPLICATION", "payload", message.payload);

    const auto serviceIt = services_.find(packet.segment.dstPort);
    if (serviceIt == services_.end()) {
        Logger::info("APPLICATION", name() + " delivers the reassembled response to client port " + std::to_string(packet.segment.dstPort) + ".");
        return;
    }

    const std::string response = serviceIt->second->handle(message);
    Logger::data("APPLICATION", "response", response);

    if (!response.empty()) {
        sendTransport(packet.srcIp, packet.segment.dstPort, packet.segment.srcPort, serviceIt->second->name(), response);
    }
}
