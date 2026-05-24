#include "app/Services.h"

#include <sstream>

std::string EchoService::name() const {
    return "echo";
}

int EchoService::wellKnownPort() const {
    return 7;
}

std::string EchoService::handle(const ApplicationMessage& message) {
    return "ECHO RESPONSE: " + message.payload;
}

FileService::FileService() {
    files_["lab-spec.txt"] =
        "Computer Networks project submission. Minimal full stack simulator response.";
    files_["routing-table.txt"] =
        "R1: 10.0.1.0/24 direct via eth0, 10.0.2.0/24 direct via eth1.";
}

std::string FileService::name() const {
    return "file";
}

int FileService::wellKnownPort() const {
    return 21;
}

std::string FileService::handle(const ApplicationMessage& message) {
    auto it = files_.find(message.payload);
    if (it == files_.end()) {
        return "FILE ERROR: requested file not found";
    }

    std::ostringstream builder;
    builder << "FILE RESPONSE " << message.payload << ": " << it->second;
    return builder.str();
}

std::shared_ptr<ApplicationService> makeEchoService() {
    return std::make_shared<EchoService>();
}

std::shared_ptr<ApplicationService> makeFileService() {
    return std::make_shared<FileService>();
}
