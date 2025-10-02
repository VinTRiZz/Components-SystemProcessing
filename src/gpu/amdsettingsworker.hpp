#ifndef AMDSETTINGSWORKER_HPP
#define AMDSETTINGSWORKER_HPP

#include <Libraries/Filework/SettingsFileWorker.hpp>
#include "settingsworker.hpp"

namespace Hardware {
namespace GPU
{

class AMDSettingsWorker final : public CardSettingsWorker, Libraries::SettingsFileWorker
{
  public:
    AMDSettingsWorker();
    ~AMDSettingsWorker();

    void init(int64_t gpuId)            override;

    bool setPowerLimit(int64_t lim)     override;
    Libraries::JOptional<int64_t> getPowerCurrent()           override;
    Libraries::JOptional<int64_t> getPowerLimitDefault()      override;
    Libraries::JOptional<int64_t> getPowerLimitCurrent()      override;
    Libraries::JOptional<int64_t> getPowerMax()               override;
    Libraries::JOptional<int64_t> getPowerMin()               override;

    bool setTemp(int64_t temp)          override;
    Libraries::JOptional<int64_t> getTempCurrent()            override;
    Libraries::JOptional<int64_t> getTempMax()                override;

    bool setFan(int64_t fanSpeed)       override;
    bool setFanMode(GPUFanOperatingMode fanMode)    override;
    Libraries::JOptional<int64_t> getFanCurrent()             override;
};

}
}

#endif // AMDSETTINGSWORKER_HPP
