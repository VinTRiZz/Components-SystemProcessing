#include "usbobjectmanager.hpp"

#include <Libraries/Datawork/Numberic.hpp>
#include <Libraries/Processes/ProcessInvoker.hpp>
#include <Libraries/Etc/Logging.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>

#include <boost/algorithm/hex.hpp>
#include <boost/format.hpp>

/*

Bus 001 Device 001: ID 1d6b:0002 Linux Foundation 2.0 root hub
Bus 001 Device 002: ID 30fa:0400  USB OPTICAL MOUSE
Bus 001 Device 003: ID 0db0:0b30 Micro Star International MSI GK30 Gaming Keyboard
Bus 001 Device 004: ID 048d:5702 Integrated Technology Express, Inc. RGB LED Controller
Bus 002 Device 001: ID 1d6b:0003 Linux Foundation 3.0 root hub
Bus 003 Device 001: ID 1d6b:0002 Linux Foundation 2.0 root hub
Bus 003 Device 002: ID 2357:011e TP-Link AC600 wireless Realtek RTL8811AU [Archer T2U Nano]
Bus 004 Device 001: ID 1d6b:0003 Linux Foundation 3.0 root hub
Bus 005 Device 001: ID 1d6b:0002 Linux Foundation 2.0 root hub
Bus 006 Device 001: ID 1d6b:0003 Linux Foundation 3.0 root hub
012 456 890123 5678 01 345678901 3
*/
namespace Hardware
{

USBObjectManager::USBObjectManager()
{

}

USBObjectManager::~USBObjectManager()
{

}

void USBObjectManager::updateObjects()
{
    std::string output, errorStr;
    if (!Libraries::ProcessInvoker::invoke("lsusb", {}, output, errorStr, 10000)) {
        COMPLOG_ERROR("Error PCI info updating");
        return;
    }

    USBObject tempObject;

    std::vector<std::string> lines;
    boost::iter_split(lines, output, boost::first_finder("\n"));
    for (auto& line : lines) {
        if (line.empty()) {
            continue;
        }

        tempObject.busNumber = Libraries::safeSton<uint16_t>( std::string(line.begin() + 4, line.begin() + 7) ).tryGetValue();
        tempObject.deviceNumber = Libraries::safeSton<uint16_t>( std::string(line.begin() + 15, line.begin() + 18) ).tryGetValue();
        tempObject.vid = std::string(line.begin() + 23, line.begin() + 27);
        tempObject.did = std::string(line.begin() + 28, line.begin() + 32);
        tempObject.deviceName = std::string(line.begin() + 33, line.end());

//        COMPLOG_DEBUG("BUS", tempObject.busNumber, "DEV", tempObject.deviceNumber, "ID", tempObject.vid, ":", tempObject.did, "NAME", tempObject.deviceName);

        m_objects.push_back(tempObject);
    }
}

std::vector<USBObject> USBObjectManager::objects() const
{
    return m_objects;
}

}
