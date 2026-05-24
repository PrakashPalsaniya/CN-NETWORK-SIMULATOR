#include "protocols/Protocols.h"

#include <sstream>

#include "core/Logger.h"

bool CsmaCdController::requestSend(const std::string& mediumName, const std::string& deviceName) {
    Logger::info("DATA-LINK", deviceName + " senses medium " + mediumName + " before transmission.");
    if (mediumBusy_) {
        Logger::warn("DATA-LINK", "Medium busy. Transmission delayed.");
        return false;
    }

    mediumBusy_ = true;
    Logger::info("DATA-LINK", "Medium is free. " + deviceName + " transmits using CSMA/CD.");
    mediumBusy_ = false;
    return true;
}

int ParityChecker::compute(const std::string& bitString) const {
    int ones = 0;
    for (char bit : bitString) {
        if (bit == '1') {
            ++ones;
        }
    }
    return ones % 2;
}

bool ParityChecker::validate(const std::string& bitString, int expectedParity) const {
    return compute(bitString) == expectedParity;
}

GoBackNWindow::GoBackNWindow(int windowSize) : windowSize_(windowSize) {
}

bool GoBackNWindow::canSend() const {
    return nextSeq_ < base_ + windowSize_;
}

void GoBackNWindow::onSend(int seqNumber) {
    if (seqNumber >= nextSeq_) {
        nextSeq_ = seqNumber + 1;
    }
}

void GoBackNWindow::onAck(int ackNumber) {
    if (ackNumber >= base_) {
        base_ = ackNumber + 1;
    }
}

std::string GoBackNWindow::snapshot() const {
    std::ostringstream builder;
    builder << "base=" << base_ << ", next=" << nextSeq_ << ", window=" << windowSize_;
    return builder.str();
}
