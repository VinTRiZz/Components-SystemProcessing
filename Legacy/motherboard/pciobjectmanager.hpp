#ifndef PCIOBJECTMANAGER_H
#define PCIOBJECTMANAGER_H

#include <string>
#include <vector>

namespace Hardware
{

struct PCIObject
{
    uint16_t busNumber {};
    uint16_t deviceNumber {};
    uint16_t functionNumber {};

    enum class ObjectType {
        Unknown,
        HostBridge,
        PCIBridge,
        IOMMU,
        Ethernet,
        VGACompatible,
        USB,
        SATA,
        Other
    };
    ObjectType type;
    std::string deviceName;

    void setPciNumber(const std::string& pciNo);
    std::string getPciNumber() const;

    bool isFree() const;

    bool operator =(const PCIObject& o_) const {
        return (getPciNumber() == o_.getPciNumber());
    }
};

class PCIObjectManager
{
public:
    PCIObjectManager();
    ~PCIObjectManager();

    void updateObjectList();
    std::vector<PCIObject> objects() const;

    // x1 x4 x8 x16
    std::tuple<uint16_t, uint16_t, uint16_t, uint16_t> getPciBusCount() const;

private:
    std::vector<PCIObject> m_objects;
};

}

#endif // PCIOBJECTMANAGER_H
