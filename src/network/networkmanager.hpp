#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <memory>
#include <vector>

#include <Libraries/Internal/AbstractHardware.hpp>

namespace Hardware
{

class NetworkManager final : public Libraries::AbstractHardware
{
  public:
    DECLARE_HARDWARE(NetworkManager, Libraries::HardwareType::Internet)

    void dump() override;

    void updateAdaptorList();
    bool connectWifi(const std::string& ssid, const std::string& pass);
    std::string foreignIp(const std::string &interfaceLogicalName) const;
    std::string localIp(const std::string &interfaceLogicalName) const;

  private:
    struct NetworkManagerPrivate;
    std::shared_ptr<NetworkManagerPrivate> d;

    void parseNodeTree();
};

} // namespace Hardware

#endif // NETWORKMANAGER_H
