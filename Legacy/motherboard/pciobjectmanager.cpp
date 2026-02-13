#include "pciobjectmanager.hpp"

#include <Libraries/Datawork/Numberic.hpp>
#include <Libraries/Processes/ProcessInvoker.hpp>
#include <Libraries/Etc/Logging.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>

#include <boost/algorithm/hex.hpp>
#include <boost/format.hpp>

/*

00:00.0 Host bridge: Advanced Micro Devices, Inc. [AMD] Renoir/Cezanne Root Complex
00:00.2 IOMMU: Advanced Micro Devices, Inc. [AMD] Renoir/Cezanne IOMMU
00:01.0 Host bridge: Advanced Micro Devices, Inc. [AMD] Renoir PCIe Dummy Host Bridge
00:01.1 PCI bridge: Advanced Micro Devices, Inc. [AMD] Renoir PCIe GPP Bridge
00:02.0 Host bridge: Advanced Micro Devices, Inc. [AMD] Renoir PCIe Dummy Host Bridge
00:02.1 PCI bridge: Advanced Micro Devices, Inc. [AMD] Renoir/Cezanne PCIe GPP Bridge
00:02.2 PCI bridge: Advanced Micro Devices, Inc. [AMD] Renoir/Cezanne PCIe GPP Bridge
00:08.0 Host bridge: Advanced Micro Devices, Inc. [AMD] Renoir PCIe Dummy Host Bridge
00:08.1 PCI bridge: Advanced Micro Devices, Inc. [AMD] Renoir Internal PCIe GPP Bridge to Bus
00:14.0 SMBus: Advanced Micro Devices, Inc. [AMD] FCH SMBus Controller (rev 51)
00:14.3 ISA bridge: Advanced Micro Devices, Inc. [AMD] FCH LPC Bridge (rev 51)
00:18.0 Host bridge: Advanced Micro Devices, Inc. [AMD] Cezanne Data Fabric; Function 0
00:18.1 Host bridge: Advanced Micro Devices, Inc. [AMD] Cezanne Data Fabric; Function 1
00:18.2 Host bridge: Advanced Micro Devices, Inc. [AMD] Cezanne Data Fabric; Function 2
00:18.3 Host bridge: Advanced Micro Devices, Inc. [AMD] Cezanne Data Fabric; Function 3
00:18.4 Host bridge: Advanced Micro Devices, Inc. [AMD] Cezanne Data Fabric; Function 4
00:18.5 Host bridge: Advanced Micro Devices, Inc. [AMD] Cezanne Data Fabric; Function 5
00:18.6 Host bridge: Advanced Micro Devices, Inc. [AMD] Cezanne Data Fabric; Function 6
00:18.7 Host bridge: Advanced Micro Devices, Inc. [AMD] Cezanne Data Fabric; Function 7
01:00.0 VGA compatible controller: NVIDIA Corporation AD106 [GeForce RTX 4060 Ti] (rev a1)
01:00.1 Audio device: NVIDIA Corporation AD106M High Definition Audio Controller (rev a1)
02:00.0 USB controller: Advanced Micro Devices, Inc. [AMD] Device 43ec
02:00.1 SATA controller: Advanced Micro Devices, Inc. [AMD] 500 Series Chipset SATA Controller
02:00.2 PCI bridge: Advanced Micro Devices, Inc. [AMD] 500 Series Chipset Switch Upstream Port
03:03.0 PCI bridge: Advanced Micro Devices, Inc. [AMD] Device 43ea
04:00.0 Ethernet controller: Realtek Semiconductor Co., Ltd. RTL8111/8168/8411 PCI Express Gigabit Ethernet Controller (rev 15)
05:00.0 Non-Volatile memory controller: ADATA Technology Co., Ltd. ADATA XPG GAMMIXS1 1L Media (rev 01)
06:00.0 Non-Essential Instrumentation [1300]: Advanced Micro Devices, Inc. [AMD] Zeppelin/Raven/Raven2 PCIe Dummy Function (rev c9)
06:00.1 Audio device: Advanced Micro Devices, Inc. [AMD/ATI] Renoir Radeon High Definition Audio Controller
06:00.2 Encryption controller: Advanced Micro Devices, Inc. [AMD] Family 17h (Models 10h-1fh) Platform Security Processor
06:00.3 USB controller: Advanced Micro Devices, Inc. [AMD] Renoir/Cezanne USB 3.1
06:00.4 USB controller: Advanced Micro Devices, Inc. [AMD] Renoir/Cezanne USB 3.1
06:00.6 Audio device: Advanced Micro Devices, Inc. [AMD] Family 17h/19h HD Audio Controller

*/

namespace Hardware
{

PCIObject::ObjectType getPciObjectType(const std::string& iString) {
    if (iString == "PCI bridge") {
        return PCIObject::ObjectType::PCIBridge;
    }
    if (iString == "IOMMU") {
        return PCIObject::ObjectType::IOMMU;
    }
    if (iString == "Host bridge") {
        return PCIObject::ObjectType::HostBridge;
    }
    if (iString == "USB controller") {
        return PCIObject::ObjectType::USB;
    }
    if (iString == "Ethernet controller") {
        return PCIObject::ObjectType::Ethernet;
    }
    if (iString == "VGA compatible controller") {
        return PCIObject::ObjectType::VGACompatible;
    }
    if (iString == "SATA controller") {
        return PCIObject::ObjectType::SATA;
    }
    return PCIObject::ObjectType::Other;
}


PCIObjectManager::PCIObjectManager()
{

}

PCIObjectManager::~PCIObjectManager()
{

}

void PCIObjectManager::updateObjectList()
{
    std::string output, errorStr;
    if (!Libraries::ProcessInvoker::invoke("lspci", {}, output, errorStr, 10000)) {
        COMPLOG_ERROR("Error PCI info updating");
        return;
    }

    PCIObject tempObject;

    std::vector<std::string> lines;
    boost::iter_split(lines, output, boost::first_finder("\n"));
    for (auto& line : lines) {
        if (line.empty()) {
            continue;
        }

        tempObject.setPciNumber(std::string(line.begin(), line.begin() + 7));

        auto typeStringEnd = std::find(line.begin() + 7, line.end(), ':');
        auto pciType = std::string(line.begin() + 8, typeStringEnd);
        tempObject.type = getPciObjectType(pciType);

//        COMPLOG_DEBUG("PCI ID: \"", tempObject.getPciNumber(), "\", PCI type:", int(tempObject.type));

        m_objects.push_back(tempObject);
    }
}

std::vector<PCIObject> PCIObjectManager::objects() const
{
    return m_objects;
}

std::tuple<uint16_t, uint16_t, uint16_t, uint16_t> PCIObjectManager::getPciBusCount() const
{
    std::tuple<uint16_t, uint16_t, uint16_t, uint16_t> res;

    // TODO: Setup rightly
    std::get<0>(res) = 0;
    std::get<1>(res) = 0;
    std::get<2>(res) = 0;
    std::get<3>(res) = m_objects.size();

    return res;
}

void PCIObject::setPciNumber(const std::string &pciNo)
{
    try {
        busNumber = std::stoi(std::string(pciNo.begin(), pciNo.begin() + 2), nullptr, 16);
        deviceNumber = std::stoi(std::string(pciNo.begin() + 4, pciNo.begin() + 6), nullptr, 16);
        functionNumber = std::stoi(std::string(pciNo.begin() + 6, pciNo.begin() + 7), nullptr, 16);
    } catch (std::invalid_argument& ex) {
        COMPLOG_ERROR("PCI id set error (invalid string)");
    }
}

std::string PCIObject::getPciNumber() const
{
    return  (boost::format("%02X") % busNumber).str() + ":" +
            (boost::format("%02X") % deviceNumber).str() + "." +
            (boost::format("%01X") % functionNumber).str();
}

bool PCIObject::isFree() const
{
    return  (type == ObjectType::HostBridge) ||
            (type == ObjectType::PCIBridge);
}



}
