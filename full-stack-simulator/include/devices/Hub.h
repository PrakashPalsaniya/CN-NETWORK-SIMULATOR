#pragma once

#include "devices/Device.h"

class Hub : public Device {
public:
    explicit Hub(const std::string& name);
    void receiveFrame(Port* ingress, const EthernetFrame& frame) override;
};
