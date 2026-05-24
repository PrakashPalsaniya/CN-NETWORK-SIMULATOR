#pragma once

class Scenarios {
public:
    static void showMenu();
    static bool runChoice(int choice);

private:
    static void pointToPoint();
    static void hubStar();
    static void switchStar();
    static void hybridHubSwitch();
    static void fullStackRouted();
    static void runAll();
};
