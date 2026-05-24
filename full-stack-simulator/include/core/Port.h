#pragma once

#include <string>

class Device;
class Link;

struct Port {
    std::string name;
    std::string mac;
    std::string ip;
    Device* owner = nullptr;
    Link* link = nullptr;
};
