#include "devices/Router.h"

#include "core/Logger.h"

namespace {

bool beginsWith(const std::string& value, const std::string& prefix) {
    return value.rfind(prefix, 0) == 0;
}

}

Router::Router(const std::string& name) : Device(name, "Router") {
}

void Router::addRoute(const RouteEntry& entry) {
    routes_.push_back(entry);
}

void Router::mapNeighbor(const std::string& ip, const std::string& mac) {
    neighborMacs_[ip] = mac;
}

RouteEntry* Router::findRoute(const std::string& destinationIp) {
    RouteEntry* best = nullptr;
    int bestPrefix = -1;

    for (auto& route : routes_) {
        if (!cidrContainsIp(route.network, destinationIp)) {
            continue;
        }

        const int prefix = prefixLengthFromCidr(route.network);
        if (prefix > bestPrefix) {
            bestPrefix = prefix;
            best = &route;
        }
    }
    return best;
}

bool Router::ownsIpOnPort(Port* port, const std::string& ip) const {
    return port != nullptr && port->ip == ip;
}

void Router::sendArpRequest(Port* egress, const std::string& targetIp) {
    EthernetFrame frame;
    frame.kind = FrameKind::ArpRequest;
    frame.srcMac = egress->mac;
    frame.dstMac = "FF:FF:FF:FF:FF:FF";
    frame.arp.senderIp = egress->ip;
    frame.arp.senderMac = egress->mac;
    frame.arp.targetIp = targetIp;

    Logger::info("NETWORK", name() + " broadcasts ARP request for " + targetIp + " on " + egress->name + ".");
    sendOut(egress, frame);
}

void Router::sendArpReply(Port* egress, const ArpMessage& request) {
    EthernetFrame frame;
    frame.kind = FrameKind::ArpReply;
    frame.srcMac = egress->mac;
    frame.dstMac = request.senderMac;
    frame.arp.senderIp = egress->ip;
    frame.arp.senderMac = egress->mac;
    frame.arp.targetIp = request.senderIp;
    frame.arp.targetMac = request.senderMac;

    Logger::info("NETWORK", name() + " replies to ARP for " + request.senderIp + " from " + egress->name + ".");
    sendOut(egress, frame);
}

bool Router::resolveMac(Port* egress, const std::string& nextHopIp, std::string& resolvedMac) {
    const auto cached = neighborMacs_.find(nextHopIp);
    if (cached != neighborMacs_.end()) {
        resolvedMac = cached->second;
        Logger::info("NETWORK", name() + " uses ARP cache entry for " + nextHopIp + " -> " + resolvedMac + ".");
        return true;
    }

    sendArpRequest(egress, nextHopIp);
    const auto refreshed = neighborMacs_.find(nextHopIp);
    if (refreshed == neighborMacs_.end()) {
        return false;
    }

    resolvedMac = refreshed->second;
    return true;
}

void Router::sendRipUpdate() {
    for (const auto& port : ports_) {
        EthernetFrame frame;
        frame.kind = FrameKind::RipUpdate;
        frame.srcMac = port->mac;
        frame.dstMac = "FF:FF:FF:FF:FF:FF";
        frame.controlSourceIp = port->ip;

        for (const auto& route : routes_) {
            frame.ripRoutes.push_back({route.network, route.metric});
        }

        Logger::info("NETWORK", name() + " sends RIP update from " + port->name + ".");
        sendOut(port.get(), frame);
    }
}

void Router::receiveFrame(Port* ingress, const EthernetFrame& frame) {
    if (frame.kind == FrameKind::ArpRequest) {
        neighborMacs_[frame.arp.senderIp] = frame.arp.senderMac;
        if (ownsIpOnPort(ingress, frame.arp.targetIp)) {
            sendArpReply(ingress, frame.arp);
        }
        return;
    }

    if (frame.kind == FrameKind::ArpReply) {
        neighborMacs_[frame.arp.senderIp] = frame.arp.senderMac;
        Logger::info("NETWORK", name() + " learns ARP mapping " + frame.arp.senderIp + " -> " + frame.arp.senderMac + ".");
        return;
    }

    if (frame.kind == FrameKind::RipUpdate) {
        for (const auto& advertised : frame.ripRoutes) {
            bool alreadyDirect = false;
            for (const auto& route : routes_) {
                if (!route.dynamic && route.network == advertised.network) {
                    alreadyDirect = true;
                    break;
                }
            }
            if (alreadyDirect) {
                continue;
            }

            const int newMetric = advertised.metric + 1;
            RouteEntry* existing = nullptr;
            for (auto& route : routes_) {
                if (route.network == advertised.network) {
                    existing = &route;
                    break;
                }
            }

            if (existing == nullptr || newMetric < existing->metric) {
                RouteEntry learned;
                learned.network = advertised.network;
                learned.nextHop = frame.controlSourceIp;
                learned.outInterface = ingress->name;
                learned.metric = newMetric;
                learned.dynamic = true;

                if (existing == nullptr) {
                    routes_.push_back(learned);
                    Logger::info("NETWORK", name() + " learns dynamic route " + learned.network + " via " + learned.nextHop + ".");
                } else {
                    *existing = learned;
                    Logger::info("NETWORK", name() + " refreshes dynamic route " + learned.network + " via " + learned.nextHop + ".");
                }
            }
        }
        return;
    }

    if (frame.kind != FrameKind::Data) {
        return;
    }

    if (!parity_.validate(frame.payloadBits, frame.parityBit)) {
        Logger::warn("DATA-LINK", name() + " drops the frame because parity validation failed.");
        return;
    }

    Logger::info("NETWORK", name() + " decapsulates the frame and checks the routing table.");
    NetworkPacket packet = frame.packet;
    packet.hopTrace.push_back(name());
    --packet.ttl;
    if (packet.ttl <= 0) {
        Logger::warn("NETWORK", name() + " drops the packet because TTL expired.");
        return;
    }

    RouteEntry* route = findRoute(packet.dstIp);
    if (route == nullptr) {
        Logger::warn("NETWORK", name() + " has no route for " + packet.dstIp + ".");
        return;
    }

    Port* egress = portByName(route->outInterface);
    if (egress == nullptr) {
        Logger::warn("NETWORK", name() + " cannot find output interface " + route->outInterface + ".");
        return;
    }

    const std::string nextHopIp = route->nextHop == "direct" ? packet.dstIp : route->nextHop;
    std::string destinationMac;
    if (!resolveMac(egress, nextHopIp, destinationMac)) {
        Logger::warn("NETWORK", name() + " has no MAC mapping for next destination " + nextHopIp + ".");
        return;
    }

    EthernetFrame forwarded;
    forwarded.kind = FrameKind::Data;
    forwarded.srcMac = egress->mac;
    forwarded.dstMac = destinationMac;
    forwarded.packet = packet;
    forwarded.payloadBits = textToBits(packetSignature(packet));
    forwarded.parityBit = parity_.compute(forwarded.payloadBits);

    Logger::info(
        "NETWORK",
        name() + " forwards packet " + packet.srcIp + " -> " + packet.dstIp + " through " + egress->name + ".");
    sendOut(egress, forwarded);
}

void Router::printRoutingTable() const {
    Logger::section(name() + " Routing Table");
    for (const auto& route : routes_) {
        Logger::data("NETWORK", route.network, "next-hop=" + route.nextHop + ", out=" + route.outInterface);
    }
}
