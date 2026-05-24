#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "app/Services.h"
#include "devices/Device.h"
#include "protocols/Protocols.h"

class Host : public Device {
public:
    explicit Host(const std::string& name);

    void setDefaultGateway(const std::string& gatewayIp);
    void addNeighbor(const std::string& ip, const std::string& mac);
    void installService(const std::shared_ptr<ApplicationService>& service);
    void receiveFrame(Port* ingress, const EthernetFrame& frame) override;
    void sendApplication(
        const std::string& service,
        const std::string& destinationIp,
        int destinationPort,
        const std::string& payload);

private:
    struct ReassemblyState {
        std::string data;
        bool complete = false;
    };

    bool resolveMac(Port* port, const std::string& nextHopIp, std::string& resolvedMac);
    void sendArpRequest(Port* port, const std::string& targetIp);
    void sendArpReply(Port* port, const ArpMessage& request);
    void sendTransport(
        const std::string& destinationIp,
        int sourcePort,
        int destinationPort,
        const std::string& service,
        const std::string& payload);
    void sendAck(
        const std::string& destinationIp,
        int sourcePort,
        int destinationPort,
        int ackNumber,
        int messageId);
    bool isSameSubnet(const std::string& ipA, const std::string& ipB) const;
    std::vector<std::string> splitPayload(const std::string& payload) const;

    std::map<std::string, std::string> neighborMacs_;
    std::map<int, std::shared_ptr<ApplicationService>> services_;
    std::map<std::string, ReassemblyState> reassembly_;
    std::map<std::string, GoBackNWindow> senderWindows_;
    std::string defaultGatewayIp_;
    int nextEphemeralPort_ = 49152;
    int nextMessageId_ = 1;
    ParityChecker parity_;
    CsmaCdController access_;
};
