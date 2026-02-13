#include "amdfrequencymanager.h"


#include <Libraries/Filework/FileworkUtils.hpp>
#include <Libraries/Etc/Logging.hpp>
#include <iostream>

#include <algorithm>
#include <fstream>

#include <thread>

#include <Libraries/Datawork/Numberic.hpp>
#include <Libraries/Processes/ProcessInvoker.hpp>

#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <regex>


namespace Hardware
{
namespace GPU
{

std::vector<std::pair<int64_t, int64_t> > getFreqVect(const std::string& fileDataSection) {
    std::vector<std::pair<int64_t, int64_t> > result;

    std::vector<std::string> lines;
    boost::iter_split(lines, fileDataSection, boost::first_finder("\n"));
    if (lines.size() > 1) {
        lines.erase(lines.begin());
    }

    std::pair<int64_t, int64_t> values;
    int currentVal = 0;
    std::regex erasematcher("[a-zA-Z]");

    for (auto& line : lines) {
        boost::tokenizer valuesTokenizer(line);
        for (auto& val : valuesTokenizer) {
            if (currentVal == 0) {
                currentVal++;
                continue;
            }

            auto numberOnlyStr = std::regex_replace(val, erasematcher, "");
            if (currentVal == 1) {
                values.first = Libraries::safeSton<int64_t>(numberOnlyStr);
            } else {
                values.second = Libraries::safeSton<int64_t>(numberOnlyStr);
            }
            currentVal++;
        }
        result.push_back(values);
    }

    return result;
}


std::vector<std::pair<int64_t, int64_t> > getFreqLimits(const std::string& fileDataSection) {
    std::vector<std::pair<int64_t, int64_t> > result;

    std::vector<std::string> lines;
    boost::iter_split(lines, fileDataSection, boost::first_finder("\n"));
    if (lines.size() > 1) {
        lines.erase(lines.begin());
    }

    std::pair<int64_t, int64_t> values;
    int currentVal = 0;
    std::regex erasematcher("[a-zA-Z]");

    for (auto& line : lines) {
        boost::tokenizer valuesTokenizer(line);
        currentVal = 0;
        values.first = -1;
        for (auto& val : valuesTokenizer) {
            if (currentVal == 0) {
                currentVal++;
                continue;
            }

            auto numberOnlyStr = std::regex_replace(val, erasematcher, "");
            if (currentVal == 1) {
                values.first = Libraries::safeSton<int64_t>(numberOnlyStr);
            } else if (!numberOnlyStr.empty()) {
                values.second = Libraries::safeSton<int64_t>(numberOnlyStr);
            }
            currentVal++;
        }
        if (values.first != -1) {
            result.push_back(values);
        }
    }

    return result;
}

int64_t getCurrentFreq(const std::string& freqFileData) {
    std::vector<std::string> lines;
    boost::iter_split(lines, freqFileData, boost::first_finder("\n"));

    int currentVal = 0;
    std::regex erasematcher("[a-zA-Z]");
    std::regex currentMatcher("[*]");

    std::vector<std::string> currentLines;
    for (auto& line : lines) {
        boost::iter_find(currentLines, line, boost::first_finder("*"));
        if (currentLines.size() < 1) {
            continue;
        }

        boost::tokenizer valuesTokenizer(line);
        for (auto& val : valuesTokenizer) {
            if (currentVal == 0) {
                currentVal++;
                continue;
            }

            auto numberOnlyStr = std::regex_replace(val, erasematcher, "");
            if (currentVal == 1) {
                return Libraries::safeSton<int64_t>(numberOnlyStr);
            }

            currentVal++;
        }
    }
    return {};
}


int64_t getCurrentVoltage(const std::string& freqFileData) {
    std::vector<std::string> lines;
    boost::iter_split(lines, freqFileData, boost::first_finder("\n"));

    int currentVal = 0;
    std::regex erasematcher("[a-zA-Z]");
    std::regex currentMatcher("[*]");

    std::vector<std::string> currentLines;
    for (auto& line : lines) {
        boost::iter_find(currentLines, line, boost::first_finder("*"));
        if (currentLines.size() < 1) {
            continue;
        }

        boost::tokenizer valuesTokenizer(line);
        for (auto& val : valuesTokenizer) {
            if (currentVal == 0) {
                currentVal++;
                continue;
            }

            auto numberOnlyStr = std::regex_replace(val, erasematcher, "");
            if (currentVal == 2) {
                return Libraries::safeSton<int64_t>(numberOnlyStr);
            }

            currentVal++;
        }
    }
    return {};
}



AMDFrequencyManager::AMDFrequencyManager(int64_t gpuId) :
    AbstractFrequencyManager(gpuId),
    m_configFreqFilePath{std::string("/sys/class/drm/card") + m_gpuId + "/device/pp_od_clk_voltage"},
    m_currentCoreFreqFilePath{std::string("/sys/class/drm/card") + m_gpuId + "/device/pp_dpm_sclk"},
    m_currentMemFreqFilePath{std::string("/sys/class/drm/card") + m_gpuId + "/device/pp_dpm_mclk"}

{
    setupHwmonDir();
    updateFreqs();
}

AMDFrequencyManager::AMDFrequencyManager(const std::string& gpuId) :
    AbstractFrequencyManager(gpuId),
    m_configFreqFilePath{std::string("/sys/class/drm/card") + m_gpuId + "/device/pp_od_clk_voltage"},
    m_currentCoreFreqFilePath{std::string("/sys/class/drm/card") + m_gpuId + "/device/pp_dpm_sclk"},
    m_currentMemFreqFilePath{std::string("/sys/class/drm/card") + m_gpuId + "/device/pp_dpm_mclk"}
{
    setupHwmonDir();
    updateFreqs();
}

AMDFrequencyManager::~AMDFrequencyManager()
{

}

bool AMDFrequencyManager::updateFreqs()
{
    std::string readBuf;
    if (!Libraries::FileworkUtil::readFileData(m_configFreqFilePath, readBuf)) {
        COMPLOG_WARNING("Error reading freq settings file path");
        return false;
    }

    if (readBuf.size() > 10)
    {
        std::vector<std::string> midiStrings;
        boost::iter_split(midiStrings, readBuf, boost::first_finder("OD_"));
        if (midiStrings.size() < 3) {
            COMPLOG_WARNING("Not parsed AMD frequencies");
            return false;
        }
        midiStrings.erase(midiStrings.begin()); // Erase first one

        // Setup core values
        auto values = getFreqVect(midiStrings[0]);
        defaultCoreFreqBuffer = values.back().first;
        defaultCoreFreqBuffer = values.back().second;

        // Setup memory values
        values = getFreqVect(midiStrings[1]);
        defaultMemFreqBuffer = values.back().first;
        defaultMemFreqBuffer = values.back().second;

        auto limits = getFreqLimits(midiStrings[2]);
        if (limits.size() < 3) {
            COMPLOG_WARNING("Limits not parsed");
            return false;
        }

        m_limits.m_minCoreFreq = limits[0].first;
        m_limits.m_maxCoreFreq = limits[0].second;

        m_limits.m_minMemFreq = limits[1].first;
        m_limits.m_maxMemFreq = limits[1].second;

        m_limits.m_minCoreVoltage = limits[2].first;
        m_limits.m_maxCoreVoltage = limits[2].second;
    }
    return true;
}

void AMDFrequencyManager::resetToDefault()
{
    setCoreLock(defaultCoreFreqBuffer);
    setCoreVoltage(defaultCoreVoltageBuffer);

    setMemoryLock(defaultMemFreqBuffer);
    setMemoryVoltage(defaultMemVoltageBuffer);
}

FrequencyValue_t AMDFrequencyManager::getCurrentMemoryFreq() const
{
    std::string fileReadBuffer;
    if (!Libraries::FileworkUtil::readFileData(m_currentMemFreqFilePath, fileReadBuffer)) {
        COMPLOG_ERROR("Error getting AMD memory freq");
        return {};
    }

    return getCurrentFreq(fileReadBuffer);
}

FrequencyValue_t AMDFrequencyManager::getCurrentCoreFreq() const
{
    std::string fileReadBuffer;
    if (!Libraries::FileworkUtil::readFileData(m_currentCoreFreqFilePath, fileReadBuffer)) {
        COMPLOG_ERROR("Error getting AMD core freq");
        return {};
    }

    return getCurrentFreq(fileReadBuffer);
}

FrequencyValue_t AMDFrequencyManager::getCurrentMemoryLock() const
{
    COMPLOG_ERROR("AMD: Get current memory lock");
    return {};
}

FrequencyValue_t AMDFrequencyManager::getCurrentCoreLock() const
{
    COMPLOG_ERROR("AMD: Get current core lock");
    return {};
}

FrequencyValue_t AMDFrequencyManager::getCurrentCoreVoltage() const
{
    std::string fileReadBuffer;
    if (!Libraries::FileworkUtil::readFileData(m_currentCoreVoltageFilePath, fileReadBuffer)) {
        COMPLOG_ERROR("Error getting AMD current core voltage");
        return {};
    }
    boost::trim(fileReadBuffer);
    return Libraries::safeSton<int64_t>(fileReadBuffer);
}

FrequencyValue_t AMDFrequencyManager::getCurrentMemVoltage() const
{
    COMPLOG_ERROR("AMD: Get current mem voltage");
    return {};
}

bool AMDFrequencyManager::setCoreLock(int64_t clockLock)
{
    if ((clockLock > m_limits.m_maxCoreFreq) || (clockLock < m_limits.m_minCoreFreq)) {
        return false;
    }

    const std::string inputCommand = "s " + std::to_string(maxCoreFreqIndex) + " " +
                               std::to_string(clockLock) + " " +
                               std::to_string(currentCoreVoltageBuffer);

    if (!Libraries::ProcessInvoker::invoke(std::string("echo"), inputCommand))
        return false;

    const std::string confirmArgs = "\"c\" > " + m_configFreqFilePath;
    if (!Libraries::ProcessInvoker::invoke(std::string("echo"), confirmArgs))
        return false;

    currentCoreFreqBuffer = clockLock;
    return true;
}

bool AMDFrequencyManager::setMemoryLock(int64_t clockLock)
{
    if ((clockLock > m_limits.m_maxMemFreq) || (clockLock < m_limits.m_minMemFreq)) {
        return false;
    }

    const std::string inputCommand = "m " + std::to_string(maxMemFreqIndex) + " " +
                               std::to_string(clockLock) + " " +
                               std::to_string(currentMemVoltageBuffer);

    if (!Libraries::ProcessInvoker::invoke(std::string("echo"), inputCommand))
        return false;

    const std::string confirmArgs = "\"c\" > " + m_configFreqFilePath;
    if (!Libraries::ProcessInvoker::invoke(std::string("echo"), confirmArgs))
        return false;

    currentMemFreqBuffer = clockLock;
    return true;
}

bool AMDFrequencyManager::setCoreVoltage(int64_t clockVoltage)
{
    if ((clockVoltage > m_limits.m_maxCoreFreq) || (clockVoltage < m_limits.m_minCoreFreq)) {
        return false;
    }

    const std::string inputCommand = "m " + std::to_string(maxCoreFreqIndex) + " " +
                               std::to_string(currentCoreFreqBuffer) + " " +
                               std::to_string(clockVoltage);

    if (!Libraries::ProcessInvoker::invoke(std::string("echo"), inputCommand))
        return false;

    const std::string confirmArgs = "\"c\" > " + m_configFreqFilePath;
    if (!Libraries::ProcessInvoker::invoke(std::string("echo"), confirmArgs))
        return false;

    currentCoreVoltageBuffer = clockVoltage;
    return true;
}

bool AMDFrequencyManager::setMemoryVoltage(int64_t clockVoltage)
{
    if ((clockVoltage > m_limits.m_maxMemFreq) || (clockVoltage < m_limits.m_minMemFreq)) {
        return false;
    }

    const std::string inputCommand = "m " + std::to_string(maxMemFreqIndex) + " " +
                               std::to_string(currentMemFreqBuffer) + " " +
                               std::to_string(clockVoltage);

    if (!Libraries::ProcessInvoker::invoke(std::string("echo"), inputCommand))
        return false;

    const std::string confirmArgs = "\"c\" > " + m_configFreqFilePath;
    if (!Libraries::ProcessInvoker::invoke(std::string("echo"), confirmArgs))
        return false;

    currentMemVoltageBuffer = clockVoltage;
    return true;
}

FrequencyValue_t AMDFrequencyManager::getDefaultCoreClock() const
{
    return defaultCoreFreqBuffer;
}

FrequencyValue_t AMDFrequencyManager::getDefaultCoreVoltage() const
{
    return defaultCoreVoltageBuffer;
}

FrequencyValue_t AMDFrequencyManager::getDefaultMemoryClock() const
{
    return defaultMemFreqBuffer;
}

FrequencyValue_t AMDFrequencyManager::getDefaultMemoryVoltage() const
{
    return defaultMemVoltageBuffer;
}

void AMDFrequencyManager::setupHwmonDir()
{
    auto hwmonDirBase = std::string("/sys/class/drm/card") + m_gpuId + "/device/hwmon";

    auto hwmonFiles = Libraries::FileworkUtil::getContentPaths(hwmonDirBase, "hwmon[0-9]+");
    if (hwmonFiles.empty()) {
        return;
    }

    m_hwmonDir = hwmonFiles.front();
    m_currentCoreVoltageFilePath = m_hwmonDir + "/in0_input";
    m_currentMemVoltageFilePath = m_hwmonDir + "/in1_input";
}

}
}
