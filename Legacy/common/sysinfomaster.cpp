#include "sysinfomaster.hpp"

#include "opencladapter.hpp"
#include "../Internal/structures.hpp"
#include "../Etc/loggers.hpp"
#include "../Processes/processinvoker.hpp"
#include "../Datawork/numberic.hpp"
#include "../Datawork/hwnodeswork.hpp"
#include "../Constants/constantmaster.hpp"
#include "../Filework/fileworkutil.hpp"
#include <boost/lexical_cast.hpp>
#include <fstream>
#include <regex>
#include <unistd.h>
#include <vector>
#include <algorithm>

// lshw headers
#include <lshw-dmi/common.h>

#include <boost/algorithm/string.hpp>




namespace Libraries
{

struct SysinfoMaster::DmiManagerPrivate {

    std::string getHostnameString()
    {
        char hostname[80];
        if (gethostname(hostname, sizeof(hostname)) == 0)
            return hostname;
        return "";
    }

    OpenCLAdapter openclAdapter;

    uint16_t nvidiaCurrentId    {0};
    uint16_t amdCurrentId       {0};

    std::vector<std::string> disksList;

    // lshw internal things
    std::string hostname;
    hwNode computer;

    DmiManagerPrivate() :
        hostname {getHostnameString()},
        computer{hostname, hw::system}
    {
        disksList = FileworkUtil::getContentNames("/dev", "nvme|sd[a-z]");
    }
};

SysinfoMaster::SysinfoMaster() : d{new DmiManagerPrivate} {

}

SysinfoMaster::~SysinfoMaster() {}

void SysinfoMaster::updateInfo()
{
    COMPLOG_INFO("DmiManager info update started");

    if (!Libraries::ProcessInvoker::isSuperuser()) {
        COMPLOG_ERROR("Not a super user. No information available");
        return;
    }

    d->openclAdapter.updateInfo();
    scanDevices();

    COMPLOG_INFO("DmiManager info update complete");
}

hwNode *SysinfoMaster::getPropertyTree()
{
    return &d->computer;
}

void SysinfoMaster::scanDevices()
{
    scan_dmi(d->computer);
    scan_network(d->computer);
    scan_nvme(d->computer);
    if(!scan_pci(d->computer))
    {
        COMPLOG_WARNING("Error scanning PCI, trying legacy scan");
        scan_pci_legacy(d->computer);
    }
//     It gather bad data, maybe next time
//    scan_cpuinfo(d->computer);

//     It gets total memory on PC, other places are empty
//    scan_memory(d->computer);

// TODO: Later
//    COMPLOG_DEBUG("USB");
//    scan_usb(d->computer);

//    COMPLOG_DEBUG("INPUT");
//    scan_input(d->computer);


// Available in future (if actually is needed)
//    scan_sound(d->computer);

// Useless, but available
//    scan_parisc(d->computer);
//    scan_device_tree(d->computer);
//    scan_spd(d->computer);
//    scan_smp(d->computer);
//    scan_pcmcia(d->computer);
//    scan_pcmcialegacy(d->computer);
//    scan_virtio(d->computer);
//    scan_vio(d->computer);
//    scan_sysfs(d->computer);
//    scan_ide(d->computer);
//    scan_ideraid(d->computer);
//    scan_scsi(d->computer);
//    scan_mmc(d->computer);
//    scan_graphics(d->computer);
//    scan_s390_devices(d->computer);
//    scan_mounts(d->computer);
//    scan_fb(d->computer);
//    scan_display(d->computer);
//    scan_abi(d->computer);
//    scan_isapnp(d->computer);
    //    scan_pnp(d->computer);
}

} // namespace Libraries
