#ifndef USBOBJECTMANAGER_H
#define USBOBJECTMANAGER_H

#include <vector>
#include <string>

namespace Hardware
{

struct USBObject
{
    // System
    uint16_t busNumber {};
    uint16_t deviceNumber {};

    // USB things
    std::string vid;
    std::string did;

    // Name if exist
    std::string deviceName;
};

class USBObjectManager
{
public:
    USBObjectManager();
    ~USBObjectManager();

    void updateObjects();
    std::vector<USBObject> objects() const;

private:
    std::vector<USBObject> m_objects;
};

}

#endif // USBOBJECTMANAGER_H
