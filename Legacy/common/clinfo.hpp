#ifndef GPU_CLINFO_HPP
#define GPU_CLINFO_HPP

#include <iostream>

#define CL_HPP_TARGET_OPENCL_VERSION 300

#include <CL/opencl.hpp>
#include <iomanip>
#include <vector>

#include <algorithm>

namespace Hardware
{
namespace GPU
{

class AMD_GPUInfoGetter
{
    struct CardInfo {
        std::string boardName;
        std::string name;
        std::string pciId;
        uint64_t totalMemory;
        uint64_t maxMemAlloc;
        uint64_t maxWorkGroup;
        uint64_t maxComputeUnits;
        std::string clDeviceVersion;
    };

  public:
    void init();

    CardInfo getCardInfo(const std::string& pciId);

  private:
    std::string getPciId(cl::Device& dev);
    void processPlatform(cl::Platform& platform);

    std::vector<CardInfo> m_infoVect;
};

} // namespace GPU
} // namespace Hardware

#endif // GPU_CLINFO_HPP