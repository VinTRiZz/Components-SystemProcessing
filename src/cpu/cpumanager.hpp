#pragma once

#include <memory>

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

#include <Libraries/Internal/AbstractHardware.hpp>

namespace Hardware
{
class CPU_Manager final : public Libraries::AbstractHardware
{
  public:
    DECLARE_HARDWARE(CPU_Manager, Libraries::HardwareType::CPU)

    void dump() override;

  private:
    nlohmann::json getAllInfo();
    nlohmann::json dynamicData();

    struct CPUManagerPrivate;
    std::shared_ptr<CPUManagerPrivate> d;

    void parseNodeTree();
};
} // namespace Hardware
