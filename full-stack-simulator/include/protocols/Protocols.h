#pragma once

#include <string>

class CsmaCdController {
public:
    bool requestSend(const std::string& mediumName, const std::string& deviceName);

private:
    bool mediumBusy_ = false;
};

class ParityChecker {
public:
    int compute(const std::string& bitString) const;
    bool validate(const std::string& bitString, int expectedParity) const;
};

class GoBackNWindow {
public:
    explicit GoBackNWindow(int windowSize);

    bool canSend() const;
    void onSend(int seqNumber);
    void onAck(int ackNumber);
    std::string snapshot() const;

private:
    int windowSize_;
    int base_ = 0;
    int nextSeq_ = 0;
};
