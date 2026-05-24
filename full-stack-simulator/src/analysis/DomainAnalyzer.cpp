#include "analysis/DomainAnalyzer.h"

DomainReport DomainAnalyzer::pointToPoint() {
    return {1, 1};
}

DomainReport DomainAnalyzer::hubStar() {
    return {1, 1};
}

DomainReport DomainAnalyzer::switchStar(int hostCount) {
    return {hostCount, 1};
}

DomainReport DomainAnalyzer::hybridHubSwitch() {
    return {2, 1};
}

DomainReport DomainAnalyzer::routedLan() {
    return {6, 3};
}
