#pragma once

#include <memory>
#include <Libraries/Internal/Structures.hpp>

namespace Hardware {
namespace GPU
{

enum class GPUFanOperatingMode
{
    undefinedState,
    autoState,
    manualState,
    amdUnknownState // TODO: Get what mode this is
};

// Used only in inheritance
class CardSettingsWorker
{
public:
    CardSettingsWorker() {}
    virtual ~CardSettingsWorker() {}

    virtual void init(int64_t gpuId)         = 0;

    virtual bool setPowerLimit(int64_t lim)  = 0;
    virtual Libraries::JOptional<int64_t> getPowerCurrent()         = 0;
    virtual Libraries::JOptional<int64_t> getPowerLimitDefault()    = 0;
    virtual Libraries::JOptional<int64_t> getPowerLimitCurrent()    = 0;
    virtual Libraries::JOptional<int64_t> getPowerMax()             = 0;
    virtual Libraries::JOptional<int64_t> getPowerMin()             = 0;

    virtual bool setTemp(int64_t temp)       = 0;
    virtual Libraries::JOptional<int64_t> getTempCurrent()          = 0;
    virtual Libraries::JOptional<int64_t> getTempMax()              = 0;

    virtual bool setFan(int64_t fanSpeed)                           = 0;
    virtual bool setFanMode(GPUFanOperatingMode fanMode)                        = 0;
    virtual Libraries::JOptional<int64_t> getFanCurrent()           = 0;

protected:
    int64_t gpuId {-1};
};

} // namespace GPU
} // namespace Hardware
