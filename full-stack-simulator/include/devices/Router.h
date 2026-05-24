#pragma once

#include <map>
#include <string>
#include <vector>

#include "devices/Device.h"
#include "protocols/Protocols.h"

class Router : public Device {
public:
    explicit Router(const std::string& name);

    void addRoute(const RouteEntry& entry);
    void mapNeighbor(const std::string& ip, const std::string& mac);
    void sendRipUpdate();
    void receiveFrame(Port* ingress, const EthernetFrame& frame) override;
    void printRoutingTable() const;

private:
    RouteEntry* findRoute(const std::string& destinationIp);
    bool resolveMac(Port* egress, const std::string& nextHopIp, std::string& resolvedMac);
    void sendArpRequest(Port* egress, const std::string& targetIp);
    void sendArpReply(Port* egress, const ArpMessage& request);
    bool ownsIpOnPort(Port* port, const std::string& ip) const;

    std::vector<RouteEntry> routes_;
    std::map<std::string, std::string> neighborMacs_;
    ParityChecker parity_;
};
