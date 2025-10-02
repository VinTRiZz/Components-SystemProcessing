#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include <memory>

#include <Libraries/Internal/Structures.hpp>

namespace Hardware
{
namespace GPU
{

typedef Libraries::JOptional<int64_t> FrequencyValue_t;

struct FreqList {
    std::vector<uint64_t> freqVect;
    std::vector<uint64_t> voltageVect;
};

struct CardOverdriveLimits {
    FrequencyValue_t m_minMemFreq      {};
    FrequencyValue_t m_maxMemFreq      {};
    FrequencyValue_t m_minMemVoltage   {};
    FrequencyValue_t m_maxMemVoltage   {};

    FrequencyValue_t m_minCoreFreq     {};
    FrequencyValue_t m_maxCoreFreq     {};
    FrequencyValue_t m_minCoreVoltage  {};
    FrequencyValue_t m_maxCoreVoltage  {};
};



class AbstractFrequencyManager
{
  public:
    AbstractFrequencyManager(const std::string &gpuId) :
        m_gpuId{gpuId} {}
    AbstractFrequencyManager(int64_t gpuId) :
        m_gpuId{std::to_string(gpuId)} {}
    virtual ~AbstractFrequencyManager() = default;

    virtual bool updateFreqs() = 0;
    virtual void resetToDefault() = 0;

    virtual FrequencyValue_t getCurrentMemoryFreq() const = 0;
    virtual FrequencyValue_t getCurrentCoreFreq() const = 0;

    virtual FrequencyValue_t getCurrentMemoryLock() const = 0;
    virtual FrequencyValue_t getCurrentCoreLock() const = 0;

    virtual FrequencyValue_t getCurrentCoreVoltage() const = 0;
    virtual FrequencyValue_t getCurrentMemVoltage() const = 0;

    // Set locked freq
    virtual bool setCoreLock(int64_t clockLock) = 0;
    virtual bool setMemoryLock(int64_t clockLock) = 0;

    // Set locked voltage
    virtual bool setCoreVoltage(int64_t clockVoltage) = 0;
    virtual bool setMemoryVoltage(int64_t clockVoltage) = 0;

    virtual FrequencyValue_t getDefaultCoreClock() const = 0;
    virtual FrequencyValue_t getDefaultCoreVoltage() const = 0;

    virtual FrequencyValue_t getDefaultMemoryClock() const = 0;
    virtual FrequencyValue_t getDefaultMemoryVoltage() const = 0;

    inline CardOverdriveLimits getLimits() const {
        return m_limits;
    }

protected:
    const std::string m_gpuId;

    int64_t m_defaultCoreFreq {0};
    int64_t m_defaultMemFreq {0};

    int64_t m_coreOffset {0};
    int64_t m_memOffset {0};

    CardOverdriveLimits m_limits;
};

} // namespace GPU

} // namespace Hardware
