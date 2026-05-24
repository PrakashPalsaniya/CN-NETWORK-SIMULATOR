#include "devices/Switch.h"

#include "core/Logger.h"

namespace {

bool isBroadcastMac(const std::string& mac) {
    return mac == "FF:FF:FF:FF:FF:FF";
}

}

Switch::Switch(const std::string& name) : Device(name, "Switch") {
}

void Switch::receiveFrame(Port* ingress, const EthernetFrame& frame) {
    macTable_[frame.srcMac] = ingress;
    Logger::info("DATA-LINK", name() + " learns " + frame.srcMac + " on " + ingress->name + ".");

    if (isBroadcastMac(frame.dstMac)) {
        Logger::info("DATA-LINK", name() + " floods broadcast traffic.");
        for (const auto& port : ports_) {
            if (port.get() != ingress) {
                sendOut(port.get(), frame);
            }
        }
        return;
    }

    auto it = macTable_.find(frame.dstMac);
    if (it == macTable_.end()) {
        Logger::info("DATA-LINK", name() + " floods unknown destination " + frame.dstMac + ".");
        for (const auto& port : ports_) {
            if (port.get() != ingress) {
                sendOut(port.get(), frame);
            }
        }
        return;
    }

    if (it->second == ingress) {
        Logger::info("DATA-LINK", name() + " filters frame because source and destination are on the same port.");
        return;
    }

    Logger::info("DATA-LINK", name() + " forwards the frame directly to " + it->second->name + ".");
    sendOut(it->second, frame);
}

void Switch::printMacTable() const {
    Logger::section(name() + " MAC Table");
    for (const auto& [mac, port] : macTable_) {
        Logger::data("DATA-LINK", mac, port->name);
    }
}
