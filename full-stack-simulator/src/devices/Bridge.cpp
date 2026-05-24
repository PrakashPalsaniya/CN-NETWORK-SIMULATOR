#include "devices/Bridge.h"

#include "core/Logger.h"

namespace {

bool isBroadcastMac(const std::string& mac) {
    return mac == "FF:FF:FF:FF:FF:FF";
}

}

Bridge::Bridge(const std::string& name) : Device(name, "Bridge") {
}

void Bridge::receiveFrame(Port* ingress, const EthernetFrame& frame) {
    macTable_[frame.srcMac] = ingress;
    Logger::info("DATA-LINK", name() + " learns " + frame.srcMac + " on " + ingress->name + ".");

    if (isBroadcastMac(frame.dstMac)) {
        for (const auto& port : ports_) {
            if (port.get() != ingress) {
                Logger::info("DATA-LINK", name() + " forwards broadcast traffic across the bridge.");
                sendOut(port.get(), frame);
            }
        }
        return;
    }

    auto it = macTable_.find(frame.dstMac);
    if (it == macTable_.end()) {
        for (const auto& port : ports_) {
            if (port.get() != ingress) {
                Logger::info("DATA-LINK", name() + " floods unknown destination " + frame.dstMac + ".");
                sendOut(port.get(), frame);
            }
        }
        return;
    }

    if (it->second != ingress) {
        Logger::info("DATA-LINK", name() + " forwards the frame to " + it->second->name + ".");
        sendOut(it->second, frame);
    }
}

void Bridge::printMacTable() const {
    Logger::section(name() + " MAC Table");
    for (const auto& [mac, port] : macTable_) {
        Logger::data("DATA-LINK", mac, port->name);
    }
}
