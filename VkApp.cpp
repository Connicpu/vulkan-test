#include "VkApp.h"

VkApp::VkApp()
{
    auto appInfo = vk::ApplicationInfo()
        .setPApplicationName("Cnnr's Vulkan Renderer")
        .setEngineVersion(1)
        .setApiVersion(VK_API_VERSION_1_0);

    const char *extensions[] =
    {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
    };

    auto instInfo = vk::InstanceCreateInfo()
        .setPApplicationInfo(&appInfo)
        .setEnabledExtensionCount(ARRAYSIZE(extensions))
        .setPpEnabledExtensionNames(extensions);

    instance = vk::createInstance(instInfo);
}

VkApp::~VkApp()
{
    instance.destroy();
}


