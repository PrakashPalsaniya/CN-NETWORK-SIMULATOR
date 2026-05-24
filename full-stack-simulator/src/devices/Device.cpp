#include "devices/Device.h"

#include "core/Logger.h"
#include "physical/Link.h"

Device::Device(std::string name, std::string type) : name_(std::move(name)), type_(std::move(type)) {
}

Port* Device::addPort(const std::string& portName, const std::string& mac, const std::string& ip) {
    auto port = std::make_unique<Port>();
    port->name = portName;
    port->mac = mac;
    port->ip = ip;
    port->owner = this;
    ports_.push_back(std::move(port));
    return ports_.back().get();
}

Port* Device::portByName(const std::string& portName) const {
    for (const auto& port : ports_) {
        if (port->name == portName) {
            return port.get();
        }
    }
    return nullptr;
}

const std::string& Device::name() const {
    return name_;
}

const std::string& Device::type() const {
    return type_;
}

void Device::sendOut(Port* egress, const EthernetFrame& frame) {
    if (egress == nullptr || egress->link == nullptr) {
        Logger::warn("PHYSICAL", name_ + " cannot send from an unconnected port.");
        return;
    }
    egress->link->transmit(egress, frame);
}
