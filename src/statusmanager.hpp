#pragma once

#include <stdint.h>
#include <string>

#include <vector>

namespace SystemProcessing {

class StatusManager
{
public:
    /**
     * @brief getCPUCurrentTemperature  Получить температуру в градусах Цельсия
     * @return
     */
    double getCPUCurrentTemperature() const noexcept;

    /**
     * @brief getCPULoad
     * @return  Загруженность в процентах
     */
    double getCPULoad() const noexcept;

    unsigned long long getUptimeSec() const;

private:
    std::vector<size_t> getCPUtimes() const;
    bool getCPUtimes(size_t &idleTime, size_t &totalTime) const;
};

} // namespace SystemProcessing

