#include "networkmanager.hpp"

#include <Libraries/Etc/Logging.hpp>
#include <Libraries/Processes/ProcessInvoker.hpp>
#include <Libraries/Internal/Structures.hpp>
#include <Libraries/Datawork/HWNodesWork.hpp>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if_arp.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <string>
#include <sys/types.h>

#include <regex>

#if (__cplusplus > 201402L)
#include <filesystem>
namespace stdfs = std::filesystem;
#else
#include <experimental/filesystem>
namespace stdfs = std::experimental::filesystem;
#endif

namespace Hardware
{

struct NetworkManager::NetworkManagerPrivate
{
    std::vector<std::string> getNetworkAdaptorNameList() const;
    std::vector<Libraries::Internal::NetworkAdaptor> m_adaptors;

    void addNetwork(hwNode* pNode) {

        auto tmpSerial = pNode->getSerial();
        auto foundNet = std::find_if(m_adaptors.begin(), m_adaptors.end(), [tmpSerial](auto& netParametersA){
            if (!netParametersA.serial.has_value()) {
                return true;
            }
            if (netParametersA.serial->empty()) {
                return true;
            }
            return (netParametersA.serial == tmpSerial);
        });
        if (foundNet == m_adaptors.end()) {
            m_adaptors.push_back({});
            foundNet = m_adaptors.end() - 1;
        }
        Libraries::Internal::NetworkAdaptor& netParameters = *foundNet;

        setupFromNode(pNode, &netParameters);

        if (!netParameters.description.has_value()) netParameters.description     = pNode->getDescription();
        if (!netParameters.speed.has_value())       netParameters.speed           = pNode->getConfig("speed"); // in Gb/s
        if (!netParameters.capacity.has_value())    netParameters.capacity        = pNode->getCapacity();

        std::regex dataRgx("\\[[^\\]]+\\]");
        std::smatch matches;
        if (std::regex_search(netParameters.vendor.tryGetValue(), matches, dataRgx)) {
            netParameters.vendor->erase(matches.position());
        }

        if (std::regex_search(netParameters.vendor.tryGetValue(), matches, dataRgx)) {
            netParameters.vendor->erase(matches.position());
        }

        if (netParameters.busInfo->size() > 10) {
            netParameters.busInfo->erase(0, 9);
            netParameters.busInfo->erase(2, netParameters.busInfo->size());
        }

        netParameters.valuesCheckup();
        if (m_adaptors.empty()) {
            m_adaptors.push_back(netParameters);
            return;
        }
        if (netParameters != m_adaptors.back()) {
            m_adaptors.push_back(netParameters);
        }
    }
};

void NetworkManager::dump()
{
    for (auto& adapter : d->m_adaptors) {
        adapter.dump();
    }
}

void NetworkManager::updateAdaptorList()
{
    parseNodeTree();
//    d->m_adaptors.clear();

//    std::string lshwOutput;
//    if (Libraries::ProcessInvoker::invoke("lshw", "-class network",
//                                           lshwOutput))
//    {
//        LOG_ERROR("lshw execution error");
//        return;
//    }

//    std::string::iterator beginpos = lshwOutput.begin(),
//                          endpos = lshwOutput.end(), currentPos = beginpos,
//                          endlinePos = beginpos, currentAdaptor = beginpos;

//    std::string currentDataName;

//    Libraries::Internal::NetworkAdaptor bufferStruct;

//    const std::string parameters[] = {"description",  "product",      "vendor",
//                                      "bus info",     "logical name", "serial",
//                                      "configuration"};
//    const std::string configurationParameters[] = {"ip="};

//    while (currentAdaptor != endpos)
//    {
//        for (int i = 0; i < 7; i++)
//        {
//            currentPos =
//                std::search(currentAdaptor, endpos, parameters[i].begin(),
//                            parameters[i].end());
//            endlinePos = std::find(currentPos, endpos, '\n');

//            if (currentPos != endlinePos)
//            {
//                currentPos += 2 + parameters[i].size();

//                switch (i)
//                {
//                case 0:
//                    bufferStruct.description =
//                        std::string(currentPos, endlinePos);
//                    break;

//                case 1:
//                    bufferStruct.product = std::string(currentPos, endlinePos);
//                    break;

//                case 2:
//                    bufferStruct.vendor = std::string(currentPos, endlinePos);
//                    break;

//                case 3:
//                    bufferStruct.busInfo = std::string(currentPos, endlinePos);
//                    break;

//                case 4:
//                    bufferStruct.logicalName =
//                        std::string(currentPos, endlinePos);
//                    break;

//                case 5:
//                    bufferStruct.serial = std::string(currentPos, endlinePos);
//                    currentAdaptor      = currentPos;
//                    break;

//                case 6:
//                    // Configuration

//                    // Find IP
//                    currentPos = std::search(currentPos, endpos,
//                                             configurationParameters[0].begin(),
//                                             configurationParameters[0].end());
//                    endlinePos = std::find(currentPos, endpos, ' ');

//                    if (currentPos != endlinePos)
//                    {
//                        currentPos += configurationParameters[0].length();
//                        bufferStruct.ip = std::string(currentPos, endlinePos);
//                    }

//                    bufferStruct.guid = Libraries::generateGuid(
//                        bufferStruct.logicalName + bufferStruct.product +
//                        bufferStruct.serial + bufferStruct.vendor);

//                    d->m_adaptors.push_back(bufferStruct);
//                    bufferStruct.description.clear();
//                    bufferStruct.product.clear();
//                    bufferStruct.vendor.clear();
//                    bufferStruct.busInfo.clear();
//                    bufferStruct.logicalName.clear();
//                    bufferStruct.serial.clear();
//                    break;
//                }
//            } else
//            {
//                currentAdaptor = endpos;
//            }
//        }
//    }

    /// -------------------------------------------------------------------------------------
    // NEXT GEN
    /// -------------------------------------------------------------------------------------

    //    if (!stdfs::exists("/sys/class/net"))
    //    {
    //        LOG_CRITICAL("Error searching for network adaptors");
    //        return;
    //    }

    //    NetworkAdaptor bufferStruct;
    //    std::fstream tempFile;
    //    std::string adapterBasepath;
    //    std::string readbuf;
    //    for (const auto& entry : stdfs::directory_iterator("/sys/class/net"))
    //    {
    //        if (!entry.is_directory()) continue;

    //        adapterBasepath = entry.path();
    //        if (adapterBasepath.find("/lo") != std::string::npos) // Skip
    //        loopback
    //            continue;

    //        LOG_DEBUG("Found adapter:", entry.path());
    //        tempFile.open(adapterBasepath + "/address", std::ios_base::in);
    //        if (!tempFile.is_open()) continue;

    //        bufferStruct             = NetworkAdaptor();
    //        bufferStruct.logicalName = entry.path().filename();

    //        std::getline(tempFile, readbuf, '\n');
    //        bufferStruct.serial = readbuf;

    //        // Get driver info
    //        if (!stdfs::exists(adapterBasepath + "/device/driver"))
    //        {
    //            LOG_ERROR("Did not found driver info for interface",
    //                      bufferStruct.logicalName);
    //            continue;
    //        }

    //        tempFile.close();
    //        d->m_adaptors.push_back(bufferStruct);
    //    }
}

bool NetworkManager::connectWifi(const std::string& ssid,
                                 const std::string& pass)
{
    const std::string commandText =
        "nmcli device wifi connect " + ssid + " password " + pass;
    FILE* commandPipe = popen(commandText.c_str(), "r");

    char buffer[128];
    std::string commandOutput;

    while (!feof(commandPipe))
    {
        if (fgets(buffer, 128, commandPipe) != NULL)
        {
            commandOutput += buffer;
        }
    }
    pclose(commandPipe);

    return (commandOutput.find("Error:") == std::string::npos);
}

std::string NetworkManager::foreignIp(const std::string& interfaceLogicalName) const
{
    std::string output;
    if (!Libraries::ProcessInvoker::invoke("curl", "-q ifconfig.me", output))
        return "Error getting F-IP";
//    LOG_DEBUG("Foreign IP:", output);
    return output;

//    int fd = socket(AF_INET, SOCK_DGRAM, 0);
//    if (fd < 0) return "Undefined";

//    struct ifreq ifr;
//    memset(&ifr, 0, sizeof(ifr));
//    ifr.ifr_addr.sa_family = AF_INET;
//    strcpy(ifr.ifr_name, interfaceLogicalName.c_str());

//    std::string result;
//    if ((ioctl(fd, SIOCGIFDSTADDR, &ifr) == 0))
//    {
//        result = std::string(inet_ntoa( ((sockaddr_in *) & ifr.ifr_dstaddr) ->sin_addr));
//    }

//    close(fd);

//    LOG_DEBUG("Foreign IP:", result);
//    return result;
}

std::string NetworkManager::localIp(const std::string& interfaceLogicalName) const
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) return "Undefined";

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_addr.sa_family = AF_INET;
    strcpy(ifr.ifr_name, interfaceLogicalName.c_str());

    std::string result;
    if (ioctl(fd, SIOCGIFADDR, &ifr) == 0)
    {
        // IP address is in ifr.ifr_addr
        result = std::string(inet_ntoa( ((sockaddr_in *) (&ifr.ifr_addr)) ->sin_addr ) );
    }

    close(fd);

//    LOG_DEBUG("Local IP:", result);
    return result;
}

void NetworkManager::parseNodeTree()
{
    auto selfDevice = Libraries::ConstantMaster::getInstance().getDmiManager().getPropertyTree();
    std::vector<hwNode*> netNodes;
    Libraries::searchForDevices(hw::hwClass::network, selfDevice, netNodes);

    for (auto pNode : netNodes) {
        d->addNetwork(pNode);
    }
}

void NetworkManager::init()
{
    d = std::make_shared<NetworkManagerPrivate>();
    updateAdaptorList();

    if (!d->m_adaptors.size()) {
        setErrorText("Network init error (not found any)");
        return;
    }

    // Setup IPs
    for (auto& adaptor : d->m_adaptors)
    {
        if (!adaptor.logicalName->empty())
        {
            adaptor.ip = localIp(adaptor.logicalName);
            adaptor.foreignIp = foreignIp(adaptor.logicalName);
            adaptor.guid = Libraries::generateGuid(adaptor.logicalName);
        }
    }
    setInited();
}

void NetworkManager::start() {}

nlohmann::json
NetworkManager::processInfoRequestPrivate(const std::string& uuid)
{
    nlohmann::json adaptorList, adaptorListPart, adaptorInfo;
    for (auto& netAdaptor : d->m_adaptors)
    {
        adaptorListPart["id"] = netAdaptor.guid;

            adaptorInfo["globalIP"]     = foreignIp(netAdaptor.logicalName);
            adaptorInfo["localIP"]      = localIp(netAdaptor.logicalName);
            adaptorInfo["manufacturer"] = netAdaptor.vendor;
            adaptorInfo["periphery"]    = netAdaptor.product;
            adaptorInfo["logicalName"]  = netAdaptor.logicalName;
            adaptorInfo["size"]         = netAdaptor.speed;
            adaptorInfo["capacity"]     = netAdaptor.capacity;
            adaptorInfo["mac"]          = netAdaptor.serial;
        adaptorListPart["information"]  = adaptorInfo;

        adaptorList.push_back(adaptorListPart);
    }
    return adaptorList;
}

nlohmann::json
NetworkManager::processDynamicRequestPrivate(const std::string& uuid)
{
    nlohmann::json adaptorList, adaptorListPart;

    for (auto& netAdaptor : d->m_adaptors)
    {
        adaptorListPart["id"] = netAdaptor.guid;
        adaptorListPart["isConnected"] = true;
        adaptorListPart["speed"] = netAdaptor.speed;
        LOG_WARNING("Requested for Wi-Fi net, but gather not defined");
        adaptorListPart["networkName"] = "Undefined";
        adaptorListPart["networkQuality"] = 3; // from 0 (bad connection) to 3 (really good connection)
    }
    return adaptorList;
}

bool NetworkManager::processOverclockRequestPrivate(
    const nlohmann::json& payload, const std::string& uuid)
{
    return false;
}

} // namespace Hardware
