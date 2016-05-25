#include "VkApp.h"
#include "Window.h"

VkApp::VkApp()
{
    InitInstance();
    InitDevice();
    InitWindow();
    InitCommandPool();
    InitSetupCmd();
    InitSwapChain();
    // TODO
    FlushSetupCmd();
}

VkApp::~VkApp()
{
    if (device && swapChain)
        device.destroySwapchainKHR(swapChain);
    FlushSetupCmd();
    if (device && commandPool)
        device.destroyCommandPool(commandPool);
    if (instance && surface)
        instance.destroySurfaceKHR(surface);
    if (window)
        window.reset();
    if (device)
        device.destroy();
    if (instance)
        instance.destroy();
}

Window *VkApp::GetWindow()
{
    return window.get();
}

void VkApp::InitInstance()
{
    // Set up our app info
    auto appInfo = vk::ApplicationInfo()
        .setPApplicationName("Cnnr's Vulkan Renderer")
        .setEngineVersion(1)
        .setApiVersion(VK_API_VERSION_1_0);

    // Extensions we need
    const char *extensions[] =
    {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
    };

    // Tell the instance about the extensions
    auto instInfo = vk::InstanceCreateInfo()
        .setPApplicationInfo(&appInfo)
        .setEnabledExtensionCount(ARRAYSIZE(extensions))
        .setPpEnabledExtensionNames(extensions);

    // Create the vulkan instance
    instance = vk::createInstance(instInfo);
}

void VkApp::InitDevice()
{
    // Get the first device
    physicalDevice = instance.enumeratePhysicalDevices()[0];

    // Set up the queue info
    float queuePriorities[] = { 1.0f };
    auto devQueueInfo = vk::DeviceQueueCreateInfo()
        .setQueueCount(1)
        .setPQueuePriorities(queuePriorities);

    // Set up the device info
    auto devInfo = vk::DeviceCreateInfo()
        .setQueueCreateInfoCount(1)
        .setPQueueCreateInfos(&devQueueInfo);

    // Create the device
    device = physicalDevice.createDevice(devInfo);

    // Find a good queue to use
    nodeIndex = FindQueue();
    queue = device.getQueue(nodeIndex, 0);
}

void VkApp::InitWindow()
{
    window = std::make_unique<Window>();

#ifdef _WIN32
    auto hwnd = window->GetHandle();
    auto surfaceInfo = vk::Win32SurfaceCreateInfoKHR()
        .setHinstance(window->GetHInst())
        .setHwnd(hwnd);

    // Get the window rectangle
    RECT rect;
    GetClientRect(hwnd, &rect);
    clientWidth = rect.right - rect.left;
    clientHeight = rect.bottom - rect.top;

    // Create a surface for the window
    surface = instance.createWin32SurfaceKHR(surfaceInfo);
#else
#error "Non-windows not yet supported"
#endif

    // Get the color format to use
    auto surfaceFormats = physicalDevice.getSurfaceFormatsKHR(surface);
    if (surfaceFormats.size() == 1 && surfaceFormats[0].format == vk::Format::eUndefined)
        colorFormat = vk::Format::eB8G8R8A8Unorm;
    else
        colorFormat = surfaceFormats[0].format;
    colorSpace = surfaceFormats[0].colorSpace;
}

void VkApp::InitCommandPool()
{
    auto poolInfo = vk::CommandPoolCreateInfo()
        .setQueueFamilyIndex(nodeIndex)
        .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);

    commandPool = device.createCommandPool(poolInfo);
}

void VkApp::InitSwapChain()
{
    auto oldSwap = swapChain;
    auto surfaceCaps = physicalDevice.getSurfaceCapabilitiesKHR(surface).value;
    auto presentModes = physicalDevice.getSurfacePresentModesKHR(surface);

    // Figure out the extent of the surface to use
    vk::Extent2D swapExtent;
    if (surfaceCaps.currentExtent.width == -1)
    {
        swapExtent.width = clientWidth;
        swapExtent.height = clientHeight;
    }
    else
    {
        swapExtent.width = surfaceCaps.currentExtent.width;
        swapExtent.height = surfaceCaps.currentExtent.height;
    }

    // Prefer Mailbox -> FifoRelaxed -> Fifo
    vk::PresentModeKHR swapchainPresentMode = vk::PresentModeKHR::eFifo;
    for (auto mode : presentModes)
    {
        if (mode == vk::PresentModeKHR::eMailbox)
        {
            swapchainPresentMode = vk::PresentModeKHR::eMailbox;
            break;
        }
        else if (mode == vk::PresentModeKHR::eFifoRelaxed)
        {
            swapchainPresentMode = vk::PresentModeKHR::eFifoRelaxed;
        }
    }

    uint32_t desiredImages = surfaceCaps.minImageCount + 1;
    if (surfaceCaps.maxImageCount > 0 && desiredImages > surfaceCaps.maxImageCount)
    {
        desiredImages = surfaceCaps.maxImageCount;
    }

    auto preTransform = surfaceCaps.currentTransform;
    if (surfaceCaps.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity)
        preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;


}

size_t VkApp::FindQueue()
{
    // Find the first queue that supports graphics and presenting.
    auto queueProperties = physicalDevice.getQueueFamilyProperties();
    for (uint32_t i = 0; i < queueProperties.size(); ++i)
    {
        if (queueProperties[i].queueFlags & vk::QueueFlagBits::eGraphics)
        {
            if (physicalDevice.getSurfaceSupportKHR(i, surface))
            {
                return i;
            }
        }
    }

    throw std::runtime_error{ "Device does not support graphics and presenting in any queues" };
}

void VkApp::SetImageLayout(
    vk::CommandBuffer commandBuffer,
    vk::Image image,
    vk::ImageAspectFlags aspectMask,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout)
{
    auto memBarrier = vk::ImageMemoryBarrier()
        .setOldLayout(oldLayout)
        .setNewLayout(newLayout)
        .setImage(image)
        .setSubresourceRange(
            vk::ImageSubresourceRange()
            .setAspectMask(aspectMask)
            .setLevelCount(1)
            .setLayerCount(1));

    // Determine flags on the old layout
    if (oldLayout == vk::ImageLayout::eUndefined)
        memBarrier.setSrcAccessMask(vk::AccessFlagBits::eHostWrite | vk::AccessFlagBits::eTransferWrite);
    else if (oldLayout == vk::ImageLayout::eColorAttachmentOptimal)
        memBarrier.setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);
    else if (oldLayout == vk::ImageLayout::eTransferSrcOptimal)
        memBarrier.setSrcAccessMask(vk::AccessFlagBits::eTransferRead);

    // Determine flags on the new layout
    if (newLayout == vk::ImageLayout::eTransferDstOptimal)
        memBarrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
    else if (newLayout == vk::ImageLayout::eTransferSrcOptimal)
        memBarrier
        .setSrcAccessMask(memBarrier.srcAccessMask | vk::AccessFlagBits::eTransferRead)
        .setDstAccessMask(vk::AccessFlagBits::eTransferRead);
    else if (newLayout == vk::ImageLayout::eColorAttachmentOptimal)
        memBarrier
        .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
        .setSrcAccessMask(vk::AccessFlagBits::eTransferRead);
    else if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
        memBarrier.setDstAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite);
    else if (newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
        memBarrier
        .setSrcAccessMask(vk::AccessFlagBits::eHostWrite | vk::AccessFlagBits::eTransferWrite)
        .setDstAccessMask(vk::AccessFlagBits::eShaderRead);

    commandBuffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eTopOfPipe,
        vk::PipelineStageFlagBits::eTopOfPipe,
        vk::DependencyFlags(),
        nullptr,
        nullptr,
        memBarrier
    );
}

void VkApp::InitSetupCmd()
{
    FlushSetupCmd();

    auto allocateInfo = vk::CommandBufferAllocateInfo()
        .setCommandPool(commandPool)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(1);

    vk::createResultValue(
        device.allocateCommandBuffers(&allocateInfo, &setupCmd),
        "vk::Device::allocateCommandBuffers"
    );

    setupCmd.begin(vk::CommandBufferBeginInfo{});
}

void VkApp::FlushSetupCmd()
{
    if (!setupCmd)
        return;

    setupCmd.end();

    auto submitInfo = vk::SubmitInfo()
        .setCommandBufferCount(1)
        .setPCommandBuffers(&setupCmd);

    queue.submit(submitInfo, nullptr);
    queue.waitIdle();

    device.freeCommandBuffers(commandPool, setupCmd);
}


