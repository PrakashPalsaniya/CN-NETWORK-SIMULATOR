#include "physical/Link.h"

#include "core/Logger.h"
#include "core/Port.h"
#include "devices/Device.h"

Link::Link(std::string name) : name_(std::move(name)) {
}

void Link::connect(Port* left, Port* right) {
    left_ = left;
    right_ = right;
    if (left_) {
        left_->link = this;
    }
    if (right_) {
        right_->link = this;
    }
}

void Link::transmit(Port* sender, const EthernetFrame& frame) {
    Port* receiver = nullptr;
    if (sender == left_) {
        receiver = right_;
    } else if (sender == right_) {
        receiver = left_;
    }

    if (receiver == nullptr || receiver->owner == nullptr) {
        Logger::warn("PHYSICAL", "Frame lost on link " + name_ + " because the other end is missing.");
        return;
    }

    Logger::info(
        "PHYSICAL",
        sender->owner->name() + ":" + sender->name + " pushes signal on " + name_ + " toward " +
            receiver->owner->name() + ":" + receiver->name + ".");
    receiver->owner->receiveFrame(receiver, frame);
}

const std::string& Link::name() const {
    return name_;
}
