#pragma once

#include <map>
#include <string>

#include "devices/Device.h"

class Switch : public Device {
public:
    explicit Switch(const std::string& name);
    void receiveFrame(Port* ingress, const EthernetFrame& frame) override;
    void printMacTable() const;

private:
    std::map<std::string, Port*> macTable_;
};
