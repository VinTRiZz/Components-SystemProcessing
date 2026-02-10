#ifndef NVIDIASETTINGSWORKER_H
#define NVIDIASETTINGSWORKER_H

#include "settingsworker.hpp"

typedef void PDisplay;

namespace Hardware {
namespace GPU
{

class NvidiaSettingsWorker final : public CardSettingsWorker
{
public:
    NvidiaSettingsWorker();
    ~NvidiaSettingsWorker();

    void init(int64_t gpuId) override;

    bool setPowerLimit(int64_t lim) override;
    Libraries::JOptional<int64_t> getPowerCurrent() override;
    Libraries::JOptional<int64_t> getPowerLimitDefault() override;
    Libraries::JOptional<int64_t> getPowerLimitCurrent() override;
    Libraries::JOptional<int64_t> getPowerMax() override;
    Libraries::JOptional<int64_t> getPowerMin() override;

    bool setTemp(int64_t temp) override;
    Libraries::JOptional<int64_t> getTempCurrent() override;
    Libraries::JOptional<int64_t> getTempMax() override;

    bool setFan(int64_t fanSpeed) override;
    bool setFanMode(GPUFanOperatingMode fanMode) override;
    Libraries::JOptional<int64_t> getFanCurrent() override;

    Libraries::JOptional<std::string> getCudaVersion() const;
    std::pair<int64_t, int64_t> getFanLimits() const;

    void setDisplay(std::shared_ptr<PDisplay> pDisplay);

private:
    struct NvidiaSettingsWorkerPrivate;
    std::shared_ptr<NvidiaSettingsWorkerPrivate> d;
};

}
}

#endif // NVIDIASETTINGSWORKER_H
