#include "cpumanager.hpp"

#include <Libraries/Internal/Structures.hpp>
#include <Libraries/Constants/ConstantMaster.hpp>
#include <Libraries/Datawork/HWNodesWork.hpp>

#include <lshw-dmi/common.h>

#include "cpu.hpp"

namespace Hardware
{

struct CPU_Manager::CPUManagerPrivate {
    std::vector<CPU> cpus;

    std::vector<Libraries::Internal::CPU_Parameters> cpuParameters;

    void addCpu(hwNode* pNode)
    {
        decltype(cpuParameters)::iterator cpuIt;
        if (cpuParameters.empty()) {
            cpuParameters.push_back({});
            cpuIt = cpuParameters.end() - 1;
        }
        auto& cpuParams = *cpuIt;
        Libraries::setupFromNode(pNode, &cpuParams);

        cpuParams.vendor = std::regex_replace(cpuParams.vendor.tryGetValue(), std::regex("Advanced Micro Devices, Inc."), "AMD");

        // TODO: Hardcode, replace with real data for Intel
        if (cpuParams.vendor == pNode->getVendor()) {
            cpuParams.vendor = "Intel";
//            cpuParams.vendor = std::regex_replace(cpuParams.vendor.tryGetValue(), std::regex("Advanced Micro Devices, Inc."), "AMD");
        }

        // TODO: Get architecture and operation mode
        cpuParams.opmode = "32-bit, 64-bit";
        cpuParams.architecture = "x86_64";

        cpuParams.clock.minVal    = pNode->getClock() / 1e6;
        cpuParams.clock.maxVal    = pNode->getSize() / 1e6;
        cpuParams.coreCount       = Libraries::safeSton<int64_t>(pNode->getConfig("cores"));
        cpuParams.enabledCores    = Libraries::safeSton<int64_t>(pNode->getConfig("enabledcores"));
        cpuParams.threadTotal     = Libraries::safeSton<int64_t>(pNode->getConfig("threads"));
        cpuParams.socketCount     = 1; // TODO: Get real value?

        // Setup depend values
        if (cpuParams.threadTotal.has_value()) {
            if (cpuParams.coreCount.has_value() && (cpuParams.coreCount.value() != 0))
                cpuParams.threadPerCore = (cpuParams.threadTotal.tryGetValue() / cpuParams.coreCount.tryGetValue());

            if (cpuParams.socketCount.has_value() && (cpuParams.socketCount.value() != 0))
                cpuParams.coresPerSocket = (cpuParams.threadTotal.tryGetValue() / cpuParams.socketCount.tryGetValue());
        }

        // TODO: Deal with it?
        cpuParams.temperature.maxVal        = 95;
        cpuParams.temperature.defaultVal    = 95;

        cpuParams.valuesCheckup();
        if (cpuParameters.empty()) {
            cpuParameters.push_back(cpuParams);
            return;
        }
        if (cpuParams != cpuParameters.back()) {
            cpuParameters.push_back(cpuParams);
        }
    }
};

void CPU_Manager::init()
{
    d = std::make_shared<CPUManagerPrivate>();    
    parseNodeTree();

    for (auto cpuInfo : d->cpuParameters)
    {
        CPU cpu(cpuInfo);
        cpu.init();
        d->cpus.push_back(cpu);
    }

    setInited(true);
    setCanWork(true);
}

void CPU_Manager::start() {}

nlohmann::json CPU_Manager::processInfoRequestPrivate(const std::string& uuid)
{
    // Process data request
    return getAllInfo();
}

nlohmann::json
CPU_Manager::processDynamicRequestPrivate(const std::string& uuid)
{
    for (auto cpu : d->cpus)
        cpu.updateDynamic();
    return dynamicData();
}

bool CPU_Manager::processOverclockRequestPrivate(const nlohmann::json& payload,
                                                 const std::string& uuid)
{
    for (auto cpu : d->cpus)
    {
        if (cpu.uuid() == uuid) return cpu.setOverclock(payload);
    }
    return false;
}

void CPU_Manager::dump()
{
    for (auto cpu : d->cpus) {
        cpu.dump();
    }
}

nlohmann::json CPU_Manager::getAllInfo()
{
    nlohmann::json result;
    for (auto cpu : d->cpus)
    {
        result.push_back(cpu.getAllInfo());
    }
    return result;
}

nlohmann::json CPU_Manager::dynamicData()
{
    decltype(dynamicData()) result;
    for (auto cpu : d->cpus)
    {
        result.push_back(cpu.getDynamic());
    }
    return result;
}

void CPU_Manager::parseNodeTree()
{
    auto selfDevice = Libraries::ConstantMaster::getInstance().getDmiManager().getPropertyTree();
    std::vector<hwNode*> processorNodes;
    Libraries::searchForDevices(hw::hwClass::processor, selfDevice, processorNodes);

    std::vector<hwNode*> memoryNodes;
    Libraries::searchForDevices(hw::hwClass::memory, selfDevice, memoryNodes);

    for (auto pNode : processorNodes) {
        d->addCpu(pNode);
    }

    for (auto pNode : memoryNodes) {
        if ((pNode->getSize() == 0) || (pNode->getDescription() == "BIOS") || ((pNode->getWidth() != 0))) {
            continue;
        }

        auto& rCpu = d->cpuParameters.back(); // TODO: Idk how to work with more than 1 CPU

        // Get cache info
        auto cacheLevel = Libraries::safeSton<int64_t>(pNode->getConfig("level"));
        switch (cacheLevel.tryGetValue())
        {
        case 1: rCpu.cacheSize.l1 = pNode->getSize() / 1024; break;
        case 2: rCpu.cacheSize.l2 = pNode->getSize() / 1024; break;
        case 3: rCpu.cacheSize.l3 = pNode->getSize() / 1024; break;
        }
    }
}

} // namespace Hardware
