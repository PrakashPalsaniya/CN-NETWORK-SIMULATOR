#pragma once

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

enum class FrameKind {
    Data,
    ArpRequest,
    ArpReply,
    RipUpdate
};

struct ApplicationMessage {
    std::string service;
    std::string payload;
};

struct TransportSegment {
    int srcPort = 0;
    int dstPort = 0;
    int seqNumber = 0;
    int ackNumber = -1;
    int messageId = 0;
    bool isAck = false;
    bool isLastChunk = true;
    std::string service;
    std::string payload;
};

struct NetworkPacket {
    std::string srcIp;
    std::string dstIp;
    int ttl = 8;
    std::vector<std::string> hopTrace;
    TransportSegment segment;
};

struct ArpMessage {
    std::string senderIp;
    std::string senderMac;
    std::string targetIp;
    std::string targetMac;
};

struct RipAdvertisement {
    std::string network;
    int metric = 1;
};

struct EthernetFrame {
    FrameKind kind = FrameKind::Data;
    std::string srcMac;
    std::string dstMac;
    int parityBit = 0;
    std::string payloadBits;
    NetworkPacket packet;
    ArpMessage arp;
    std::vector<RipAdvertisement> ripRoutes;
    std::string controlSourceIp;
};

struct RouteEntry {
    std::string network;
    std::string nextHop;
    std::string outInterface;
    int metric = 0;
    bool dynamic = false;
};

struct DomainReport {
    int collisionDomains = 0;
    int broadcastDomains = 0;
};

inline std::string textToBits(const std::string& text) {
    std::string bits;
    bits.reserve(text.size() * 8);
    for (unsigned char ch : text) {
        for (int bit = 7; bit >= 0; --bit) {
            bits.push_back(((ch >> bit) & 1U) ? '1' : '0');
        }
    }
    return bits;
}

inline std::string packetSignature(const NetworkPacket& packet) {
    std::ostringstream builder;
    builder << packet.srcIp << '|'
            << packet.dstIp << '|'
            << packet.segment.srcPort << '|'
            << packet.segment.dstPort << '|'
            << packet.segment.seqNumber << '|'
            << packet.segment.ackNumber << '|'
            << packet.segment.messageId << '|'
            << (packet.segment.isAck ? 1 : 0) << '|'
            << (packet.segment.isLastChunk ? 1 : 0) << '|'
            << packet.segment.service << '|'
            << packet.segment.payload;
    return builder.str();
}

inline std::string messageKey(const std::string& remoteIp, int remotePort, int localPort, int messageId) {
    std::ostringstream builder;
    builder << remoteIp << ':' << remotePort << "->" << localPort << '#' << messageId;
    return builder.str();
}

inline unsigned int ipv4ToUInt(const std::string& ip) {
    std::istringstream stream(ip);
    std::string token;
    unsigned int parts[4] = {0, 0, 0, 0};
    int index = 0;

    while (std::getline(stream, token, '.')) {
        if (index >= 4) {
            throw std::invalid_argument("Invalid IPv4 address");
        }

        const int value = std::stoi(token);
        if (value < 0 || value > 255) {
            throw std::invalid_argument("Invalid IPv4 address");
        }
        parts[index++] = static_cast<unsigned int>(value);
    }

    if (index != 4) {
        throw std::invalid_argument("Invalid IPv4 address");
    }

    return (parts[0] << 24U) | (parts[1] << 16U) | (parts[2] << 8U) | parts[3];
}

inline int prefixLengthFromCidr(const std::string& cidr) {
    const std::size_t slash = cidr.find('/');
    if (slash == std::string::npos) {
        return 32;
    }
    return std::stoi(cidr.substr(slash + 1));
}

inline std::string networkIpFromCidr(const std::string& cidr) {
    const std::size_t slash = cidr.find('/');
    if (slash == std::string::npos) {
        return cidr;
    }
    return cidr.substr(0, slash);
}

inline bool cidrContainsIp(const std::string& cidr, const std::string& ip) {
    const int prefix = prefixLengthFromCidr(cidr);
    const unsigned int network = ipv4ToUInt(networkIpFromCidr(cidr));
    const unsigned int address = ipv4ToUInt(ip);
    const unsigned int mask = prefix == 0 ? 0U : 0xFFFFFFFFU << (32 - prefix);
    return (network & mask) == (address & mask);
}
