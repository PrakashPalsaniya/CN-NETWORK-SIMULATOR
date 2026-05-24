#include "devices/Hub.h"

#include "core/Logger.h"

Hub::Hub(const std::string& name) : Device(name, "Hub") {
}

void Hub::receiveFrame(Port* ingress, const EthernetFrame& frame) {
    Logger::info("DATA-LINK", name() + " repeats the frame to every port except " + ingress->name + ".");
    for (const auto& port : ports_) {
        if (port.get() != ingress) {
            sendOut(port.get(), frame);
        }
    }
}
