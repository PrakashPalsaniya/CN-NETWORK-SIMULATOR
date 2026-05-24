#pragma once

#include <memory>
#include <string>
#include <vector>

#include "core/Models.h"
#include "core/Port.h"

class Device {
public:
    Device(std::string name, std::string type);
    virtual ~Device() = default;
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;
    Device(Device&&) = default;
    Device& operator=(Device&&) = default;

    Port* addPort(const std::string& portName, const std::string& mac, const std::string& ip = "");
    Port* portByName(const std::string& portName) const;
    const std::string& name() const;
    const std::string& type() const;

    virtual void receiveFrame(Port* ingress, const EthernetFrame& frame) = 0;

protected:
    void sendOut(Port* egress, const EthernetFrame& frame);
    std::vector<std::unique_ptr<Port>> ports_;

private:
    std::string name_;
    std::string type_;
};
