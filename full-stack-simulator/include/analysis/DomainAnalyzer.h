#pragma once

#include "core/Models.h"

class DomainAnalyzer {
public:
    static DomainReport pointToPoint();
    static DomainReport hubStar();
    static DomainReport switchStar(int hostCount);
    static DomainReport hybridHubSwitch();
    static DomainReport routedLan();
};
