#include "clinfo.hpp"

#include "../Etc/loggers.hpp"

namespace Hardware
{
namespace GPU
{

void AMD_GPUInfoGetter::init()
{
    std::vector<cl::Platform> platformsDetected;
    try
    {
        cl::Platform::get(&platformsDetected);
    } catch (std::exception& ex)
    {
        COMPLOG_ERROR("OpenCL error: %s", ex.what());
        return;
    }

    for (auto& platform : platformsDetected)
    {
        auto platName = platform.getInfo<CL_PLATFORM_NAME>();
        if (platName != "AMD Accelerated Parallel Processing")
        {
            continue;
        }
        processPlatform(platform);
    }

    std::cout << "Init complete" << std::endl;
}

AMD_GPUInfoGetter::CardInfo
AMD_GPUInfoGetter::getCardInfo(const std::string& pciId)
{
    std::string realPci = pciId;

    if (realPci.size() == 1) realPci = "0" + realPci;

    for (auto& card : m_infoVect)
    {
        if (card.pciId == realPci) return card;
    }
    return {};
}

std::string AMD_GPUInfoGetter::getPciId(cl::Device& dev)
{
    uint8_t t[24];
    if (clGetDeviceInfo(dev.get(), 0x4037, sizeof(t), &t, NULL) == CL_SUCCESS)
    {
        std::ostringstream s;
        s << std::setfill('0') << std::setw(2) << std::hex
          << (unsigned int)(t[21]) << ":" << std::setw(2)
          << (unsigned int)(t[22]) << "." << (unsigned int)(t[23]);
        return s.str();
    }
    return {};
}

void AMD_GPUInfoGetter::processPlatform(cl::Platform& platform)
{
    std::vector<cl::Device> cards;
    try
    {
        platform.getDevices(CL_DEVICE_TYPE_GPU | CL_DEVICE_TYPE_ACCELERATOR,
                            &cards);
    } catch (std::exception& ex)
    {
        COMPLOG_ERROR("OpenCL get cards error: %s", ex.what());
        return;
    }

    for (auto& card : cards)
    {
        auto devType = card.getInfo<CL_DEVICE_TYPE>();
        if ((devType != CL_DEVICE_TYPE_GPU) &&
            (devType != CL_DEVICE_TYPE_ACCELERATOR))
            continue;

        CardInfo cardInfo;

        // cardInfo.boardName = card.getInfo<CL_DEVICE_BOARD_NAME_AMD>();
        card.getInfo(CL_DEVICE_BOARD_NAME_AMD, &cardInfo.boardName);
        cardInfo.name  = card.getInfo<CL_DEVICE_NAME>();
        cardInfo.pciId = getPciId(card);
        cardInfo.pciId =
            std::string(cardInfo.pciId.begin(), cardInfo.pciId.begin() + 2);

        cardInfo.totalMemory  = card.getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>();
        cardInfo.maxMemAlloc  = card.getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>();
        cardInfo.maxWorkGroup = card.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>();

        cardInfo.clDeviceVersion = card.getInfo<CL_DEVICE_VERSION>();

        cardInfo.maxComputeUnits = card.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
        cardInfo.maxComputeUnits =
            cardInfo.maxComputeUnits == 14 ? 36 : cardInfo.maxComputeUnits;

        // TODO: До лучших времён
        //        deviceDescriptor.clDeviceVersionMajor =
        //            std::stoi(deviceDescriptor.clDeviceVersion.substr(7, 1));
        //        deviceDescriptor.clDeviceVersionMinor =
        //            std::stoi(deviceDescriptor.clDeviceVersion.substr(9, 1));

        m_infoVect.push_back(cardInfo);

        COMPLOG_DEBUG("AMD Device info:");
        COMPLOG_DEBUG("Board name: %s", cardInfo.boardName.c_str());
        COMPLOG_DEBUG("Name: %s", cardInfo.name.c_str());
        COMPLOG_DEBUG("PCI ID: %s", cardInfo.pciId.c_str());
        COMPLOG_DEBUG("Total mem: %s",
                  std::to_string(cardInfo.totalMemory).c_str());
        COMPLOG_DEBUG("Max mem alloc: %s",
                  std::to_string(cardInfo.maxMemAlloc).c_str());
        COMPLOG_DEBUG("CL Dev version: %s", cardInfo.clDeviceVersion.c_str());
        COMPLOG_DEBUG("Max compute units: %s",
                  std::to_string(cardInfo.maxComputeUnits).c_str());
    }
}

} // namespace GPU
} // namespace Hardware
