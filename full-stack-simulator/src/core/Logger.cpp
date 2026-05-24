#include "core/Logger.h"

#include <iostream>

void Logger::section(const std::string& title) {
    std::cout << "\n============================================================\n";
    std::cout << title << '\n';
    std::cout << "============================================================\n";
}

void Logger::info(const std::string& layer, const std::string& message) {
    std::cout << "[" << layer << "] " << message << '\n';
}

void Logger::warn(const std::string& layer, const std::string& message) {
    std::cout << "[" << layer << "][WARN] " << message << '\n';
}

void Logger::data(const std::string& layer, const std::string& label, const std::string& value) {
    std::cout << "[" << layer << "] " << label << ": " << value << '\n';
}
