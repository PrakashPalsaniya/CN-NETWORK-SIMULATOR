#include <iostream>
#include <limits>

#include "simulator/Scenarios.h"

int main() {
    while (true) {
        Scenarios::showMenu();
        std::cout << "\nEnter choice: ";

        int choice = -1;
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Please enter a valid number.\n";
            continue;
        }

        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        if (!Scenarios::runChoice(choice)) {
            break;
        }

        std::cout << "\nPress Enter to continue...";
        std::cin.get();
    }

    return 0;
}
