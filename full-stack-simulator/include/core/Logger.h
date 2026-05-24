#pragma once

#include <string>

class Logger {
public:
    static void section(const std::string& title);
    static void info(const std::string& layer, const std::string& message);
    static void warn(const std::string& layer, const std::string& message);
    static void data(const std::string& layer, const std::string& label, const std::string& value);
};
