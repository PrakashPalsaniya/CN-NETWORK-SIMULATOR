#pragma once

#include <string>

#include "core/Models.h"

struct Port;

class Link {
public:
    explicit Link(std::string name);

    void connect(Port* left, Port* right);
    void transmit(Port* sender, const EthernetFrame& frame);
    const std::string& name() const;

private:
    std::string name_;
    Port* left_ = nullptr;
    Port* right_ = nullptr;
};
