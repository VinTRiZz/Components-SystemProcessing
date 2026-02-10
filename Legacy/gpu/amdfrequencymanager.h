#ifndef AMDFREQUENCYMANAGER_H
#define AMDFREQUENCYMANAGER_H

#include "freqmanager.hpp"

namespace Hardware
{
namespace GPU
{

class AMDFrequencyManager final : public AbstractFrequencyManager
{
public:
    AMDFrequencyManager(int64_t gpuId);
    AMDFrequencyManager(const std::string& gpuId);
    ~AMDFrequencyManager();

    bool updateFreqs() override;
    void resetToDefault() override;

    FrequencyValue_t getCurrentMemoryFreq() const override;
    FrequencyValue_t getCurrentCoreFreq() const override;

    FrequencyValue_t getCurrentMemoryLock() const override;
    FrequencyValue_t getCurrentCoreLock() const override;

    FrequencyValue_t getCurrentCoreVoltage() const override;
    FrequencyValue_t getCurrentMemVoltage() const override;

    // Set locked freq
    bool setCoreLock(int64_t clockLock) override;
    bool setMemoryLock(int64_t clockLock) override;

    // Set locked voltage
    bool setCoreVoltage(int64_t clockVoltage) override;
    bool setMemoryVoltage(int64_t clockVoltage) override;

    FrequencyValue_t getDefaultCoreClock() const override;
    FrequencyValue_t getDefaultCoreVoltage() const override;

    FrequencyValue_t getDefaultMemoryClock() const override;
    FrequencyValue_t getDefaultMemoryVoltage() const override;

private:
    const std::string m_configFreqFilePath;
    const std::string m_currentCoreFreqFilePath;
    const std::string m_currentMemFreqFilePath;
    std::string m_currentCoreVoltageFilePath;
    std::string m_currentMemVoltageFilePath;

    // Затравка на будущее
    std::string m_hwmonDir;

    // For voltage setting
    int64_t currentCoreFreqBuffer {0};
    int64_t currentCoreVoltageBuffer {0};
    int64_t currentMemFreqBuffer {0};
    int64_t currentMemVoltageBuffer {0};

    // For AMD troubleshooting
    uint maxCoreFreqIndex {0};
    uint maxMemFreqIndex {0};

    int64_t defaultCoreFreqBuffer {0};
    int64_t defaultCoreVoltageBuffer {0};
    int64_t defaultMemFreqBuffer {0};
    int64_t defaultMemVoltageBuffer {0};

    void setupHwmonDir();
};

}
}

#endif // AMDFREQUENCYMANAGER_H
