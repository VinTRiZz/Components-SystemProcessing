#pragma once

#include <string>

#include <Libraries/Internal/AbstractHardware.hpp>

/**
 * @brief Contains working classes for harddrive
 */
namespace Hardware
{
class OSManager final : public Libraries::AbstractHardware
{
  public:
    void setAmdDriverVersion(const std::string& vers);
    void setNvidiaDriverVersion(const std::string& vers);

    DECLARE_HARDWARE(OSManager, Libraries::HardwareType::OS)

    void dump() override;

  private:
    void setupPackages();
    void setupServices();
    void setupOsInfo();

    struct OSManagerPrivate;
    std::shared_ptr<OSManagerPrivate> d;
};
} // namespace Hardware
