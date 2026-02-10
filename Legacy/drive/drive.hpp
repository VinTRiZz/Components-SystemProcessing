#ifndef DRIVE_H
#define DRIVE_H

#include <nlohmann/json.hpp>
#include <stdint.h>
#include <string>
#include <vector>

#include <Libraries/Internal/AbstractHardware.hpp>

namespace Hardware
{

class Drive
{
  public:
    Drive(const Libraries::Internal::DriveParameters& info);
    ~Drive() = default;

    void dump();

    void init();
    void updateDynamic();

    std::string uuid() const;

    nlohmann::json getAllParameters();
    nlohmann::json getDynamicParameters();

private:
  Libraries::Internal::DriveParameters m_info;

  int64_t free(std::string path = "/") const;
  int64_t available(std::string path = "/") const;
  int64_t capacity(std::string path = "/") const;
  int64_t temperature() const;
};

}

#endif // DRIVE_H
