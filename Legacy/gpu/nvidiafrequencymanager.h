#ifndef NVIDIAFREQUENCYMANAGER_H
#define NVIDIAFREQUENCYMANAGER_H

#include "freqmanager.hpp"

typedef void PDisplay;

namespace Hardware
{
namespace GPU
{

class NvidiaFrequencyManager final : public AbstractFrequencyManager
{
public:
    NvidiaFrequencyManager(int64_t gpuId);
    NvidiaFrequencyManager(const std::string& gpuId);
    ~NvidiaFrequencyManager();

    bool updateFreqs() override;
    void resetToDefault() override;

    FrequencyValue_t getCurrentMemoryFreq() const override;
    FrequencyValue_t getCurrentCoreFreq() const override;

    FrequencyValue_t getCurrentMemoryLock() const override;
    FrequencyValue_t getCurrentCoreLock() const override;

    FrequencyValue_t getCurrentCoreVoltage() const override;
    FrequencyValue_t getCurrentMemVoltage() const override;

    FrequencyValue_t getCurrentCoreOffset() const;
    FrequencyValue_t getCurrentMemoryOffset() const;

    bool setCoreOffset(int64_t clockOffset);
    bool setMemoryOffset(int64_t clockOffset);

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

    void setDisplay(std::shared_ptr<PDisplay> pDisplay);

private:
    struct Impl;
    std::shared_ptr<Impl> d;
};

}
}

#endif // NVIDIAFREQUENCYMANAGER_H
