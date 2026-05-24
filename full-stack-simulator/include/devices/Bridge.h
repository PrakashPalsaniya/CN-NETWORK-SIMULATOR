#pragma once

#include <map>
#include <string>

#include "devices/Device.h"

class Bridge : public Device {
public:
    explicit Bridge(const std::string& name);
    void receiveFrame(Port* ingress, const EthernetFrame& frame) override;
    void printMacTable() const;

private:
    std::map<std::string, Port*> macTable_;
};
