#pragma once

#include <map>
#include <memory>
#include <string>

#include "core/Models.h"

class ApplicationService {
public:
    virtual ~ApplicationService() = default;
    virtual std::string name() const = 0;
    virtual int wellKnownPort() const = 0;
    virtual std::string handle(const ApplicationMessage& message) = 0;
};

class EchoService : public ApplicationService {
public:
    std::string name() const override;
    int wellKnownPort() const override;
    std::string handle(const ApplicationMessage& message) override;
};

class FileService : public ApplicationService {
public:
    FileService();

    std::string name() const override;
    int wellKnownPort() const override;
    std::string handle(const ApplicationMessage& message) override;

private:
    std::map<std::string, std::string> files_;
};

std::shared_ptr<ApplicationService> makeEchoService();
std::shared_ptr<ApplicationService> makeFileService();
