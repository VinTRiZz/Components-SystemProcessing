#include "osmanager.hpp"

#include <Libraries/Etc/Logging.hpp>
#include <Libraries/Filework/ConfigParser.hpp>
#include <Libraries/Internal/Structures.hpp>
#include <Libraries/Processes/PackageManager.hpp>
#include <Libraries/Processes/ProcessInvoker.hpp>

#include <future>

namespace Hardware
{



struct OSManager::OSManagerPrivate {
    Libraries::Internal::OSInfo conf;
};

void OSManager::setAmdDriverVersion(const std::string &vers)
{
    d->conf.amdDriverVersion = vers;
}

void OSManager::setNvidiaDriverVersion(const std::string &vers)
{
    d->conf.nvidiaDriverVersion = vers;
}

void OSManager::dump()
{
    d->conf.dump();
}

void OSManager::setupPackages()
{
    if (!Libraries::ProcessInvoker::isSuperuser())
    {
        LOG_ERROR("Not a root user");
        return;
    }

    std::vector<std::string> requiredPackages;

    std::ifstream packagesFile(PACKAGE_FILE);

    LOG_INFO("Reading file");
    if (packagesFile.is_open())
    {
        std::string package;

        while (std::getline(packagesFile, package))
            requiredPackages.push_back(package);

        packagesFile.close();
    } else
    {
        LOG_CRITICAL("Did not found packages file. Copy file \"", PACKAGE_FILE,
                     "\" to binary directory");
    }
    LOG_OK("Reading packages file complete");

    LOG_INFO("Installing packages...");
    for (auto& pkg : requiredPackages)
    {
        LOG_INFO("Checking package:", pkg);
        if (!Libraries::PackageManager::isInstalled(pkg))
            Libraries::PackageManager::install(pkg);
    }
    LOG_INFO("Install check completed");
}

void OSManager::setupOsInfo()
{
    std::string output;

    if (!Libraries::ProcessInvoker::invoke("uname", "-r", output)) {
        d->conf.linuxKernelVersion = output;
    }

    if (!Libraries::ProcessInvoker::invoke("nvcc", "--version | grep -oP 'release \\K[0-9.]+'", output)) {
        d->conf.cudaVersion = output;
    }

    if (!Libraries::ProcessInvoker::invoke("dkms", "status nvidia | grep -oP 'nvidia/\\K[^,]+' | sort -u", output)) {
        d->conf.nvidiaDriverVersion = output;
    }

    if (!Libraries::ProcessInvoker::invoke("apt", "show amdgpu-install 2> /dev/null | grep -oP \"Version: \\K[^ ]+\"", output)) {
        d->conf.amdDriverVersion = output;
    }

    if (!Libraries::ProcessInvoker::invoke("apt", "show ocl-icd-opencl-dev 2> /dev/null | grep -oP \"Version: \\K[^ ]+\"", output)) {
        d->conf.openclDriverVersion = output;
    }
}

void OSManager::init()
{
    d = std::make_shared<OSManagerPrivate>();

    // Setup packages (disabled on 18.01.2025 in case of useless)
//    setupPackages();

    // Setup services
    if (Libraries::ProcessInvoker::isSuperuser())
    {
        if (!Libraries::ProcessInvoker::invoke("dpkg", "--configure -a"))
            LOG_ERROR("Error invoking dpkg configure");
    } else
    {
        LOG_WARNING("Not a root user");
        return;
    }

    // Setup OS info
    setupOsInfo();

    d->conf.valuesCheckup();

    setCanWork();
    setInited();
}

void OSManager::start() {}

nlohmann::json OSManager::processInfoRequestPrivate(const std::string& uuid)
{
    nlohmann::json result;
    result["minuxVersion"]        = {}; // TODO: Get version of Minux
    result["linuxVersion"]        = d->conf.linuxKernelVersion;
    result["amdDriverVersion"]    = d->conf.amdDriverVersion;
    result["nvidiaDriverVersion"] = d->conf.nvidiaDriverVersion;
    result["intelDriverVersion"]  = {};
    result["openCLVersion"]       = d->conf.openclDriverVersion;
    result["cudaVersion"]         = d->conf.cudaVersion;
    return result;
}

nlohmann::json OSManager::processDynamicRequestPrivate(const std::string& uuid)
{
    return {};
}

bool OSManager::processOverclockRequestPrivate(const nlohmann::json& payload,
                                               const std::string& uuid)
{
    return false;
}

} // namespace Hardware
