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
    InitCommandBuffers();
    InitDepthStencil();
    InitRenderPass();
    InitPipelineCache();
    InitFrameBuffer();
    FlushSetupCmd();
}

VkApp::~VkApp()
{
    FreeFramebuffers();

    if (device && renderPass)
        device.destroyRenderPass(renderPass);

    FreeDepthStencil();
    FreeCommandBuffers();

    // Free swap chain
    if (device && swapChain)
    {
        for (auto &buffer : swapBuffers)
        {
            device.destroyImageView(buffer.view);
        }
        device.destroySwapchainKHR(swapChain);
    }

    FlushSetupCmd();

    // Free command pool
    if (device && commandPool)
        device.destroyCommandPool(commandPool);
    // Free surface
    if (instance && surface)
        instance.destroySurfaceKHR(surface);
    // Free window
    if (window)
        window.reset();
    // Free device
    if (device)
        device.destroy();
    // Free instance
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
    queueIndex = FindQueue();
    queue = device.getQueue(queueIndex, 0);
}

void VkApp::InitWindow()
{
    window = std::make_unique<Window>();

#ifdef _WIN32
    auto hwnd = window->GetHandle();
    auto surfaceInfo = vk::Win32SurfaceCreateInfoKHR()
        .setHinstance(window->GetHInst())
        .setHwnd(hwnd);

    window->SetHandler(WM_SIZE, [this](HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) -> LRESULT
    {
        return DefWindowProcW(hwnd, msg, wp, lp);
    });

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
        .setQueueFamilyIndex(queueIndex)
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
        clientWidth = swapExtent.width = surfaceCaps.currentExtent.width;
        clientHeight = swapExtent.height = surfaceCaps.currentExtent.height;
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
        desiredImages = surfaceCaps.maxImageCount;

    auto preTransform = surfaceCaps.currentTransform;
    if (surfaceCaps.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity)
        preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;

    auto swapInfo = vk::SwapchainCreateInfoKHR()
        .setSurface(surface)
        .setMinImageCount(desiredImages)
        .setImageFormat(colorFormat)
        .setImageColorSpace(colorSpace)
        .setImageExtent(swapExtent)
        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
        .setPreTransform(preTransform)
        .setImageArrayLayers(1)
        .setImageSharingMode(vk::SharingMode::eExclusive)
        .setPresentMode(swapchainPresentMode)
        .setOldSwapchain(oldSwap)
        .setClipped(true)
        .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);

    swapChain = device.createSwapchainKHR(swapInfo);

    if (oldSwap)
    {
        for (auto &buffer : swapBuffers)
        {
            device.destroyImageView(buffer.view);
            buffer.view = nullptr;
        }
        device.destroySwapchainKHR(oldSwap);
    }

    auto images = device.getSwapchainImagesKHR(swapChain);

    swapBuffers.resize(images.size());
    for (uint32_t i = 0; i < images.size(); ++i)
    {
        auto image = images[i];
        auto viewInfo = vk::ImageViewCreateInfo()
            .setFormat(colorFormat)
            .setComponents({
                vk::ComponentSwizzle::eR,
                vk::ComponentSwizzle::eG,
                vk::ComponentSwizzle::eB,
                vk::ComponentSwizzle::eA,
        })
        .setSubresourceRange(vk::ImageSubresourceRange()
            .setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setLevelCount(1)
            .setLayerCount(1))
        .setViewType(vk::ImageViewType::e2D)
        .setImage(image);

        SetImageLayout(
            setupCmdBuffer,
            image,
            vk::ImageAspectFlagBits::eColor,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::ePresentSrcKHR
        );

        swapBuffers[i].image = image;
        swapBuffers[i].view = device.createImageView(viewInfo);
    }
}

void VkApp::InitCommandBuffers()
{
    // Allocate per-buffer command buffers
    auto bufferCount = (uint32_t)swapBuffers.size();
    auto allocateInfo = vk::CommandBufferAllocateInfo()
        .setCommandPool(commandPool)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(bufferCount);
    drawCmdBuffers = device.allocateCommandBuffers(allocateInfo);

    // Allocate present barrier buffers
    allocateInfo.setCommandBufferCount(1);
    prePresentCmdBuffer = device.allocateCommandBuffers(allocateInfo)[0];
    postPresentCmdBuffer = device.allocateCommandBuffers(allocateInfo)[0];
}

void VkApp::InitDepthStencil()
{
    // Create the image
    depthFormat = GetDepthFormat();
    auto imageInfo = vk::ImageCreateInfo()
        .setImageType(vk::ImageType::e2D)
        .setFormat(depthFormat)
        .setExtent({ (uint32_t)clientWidth, (uint32_t)clientHeight, 1 })
        .setMipLevels(1)
        .setArrayLayers(1)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setTiling(vk::ImageTiling::eOptimal)
        .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransferSrc);
    depthStencil.image = device.createImage(imageInfo);

    // Allocate the memory
    auto memReqs = device.getImageMemoryRequirements(depthStencil.image);
    auto allocateInfo = vk::MemoryAllocateInfo()
        .setAllocationSize(memReqs.size)
        .setMemoryTypeIndex(GetMemoryType(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));
    depthStencil.mem = device.allocateMemory(allocateInfo);
    device.bindImageMemory(depthStencil.image, depthStencil.mem, 0);

    // Setup the image layout
    SetImageLayout(
        setupCmdBuffer,
        depthStencil.image,
        vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthStencilAttachmentOptimal
    );

    // Create the view
    auto viewInfo = vk::ImageViewCreateInfo()
        .setViewType(vk::ImageViewType::e2D)
        .setFormat(depthFormat)
        .setSubresourceRange(vk::ImageSubresourceRange()
            .setAspectMask(vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil)
            .setLevelCount(1)
            .setLayerCount(1))
        .setImage(depthStencil.image);
    depthStencil.view = device.createImageView(viewInfo);
}

void VkApp::InitRenderPass()
{
    vk::AttachmentDescription attachments[2] =
    {
        // Color attachment
        vk::AttachmentDescription
        {
            vk::AttachmentDescriptionFlags(),
            colorFormat,
            vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eDontCare,
            vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::eColorAttachmentOptimal,
        },
        // Depth attachment
        vk::AttachmentDescription
        {
            vk::AttachmentDescriptionFlags(),
            depthFormat,
            vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eDontCare,
            vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eDepthStencilAttachmentOptimal,
            vk::ImageLayout::eDepthStencilAttachmentOptimal,
        },
    };

    auto colorReference = vk::AttachmentReference()
        .setAttachment(0)
        .setLayout(vk::ImageLayout::eColorAttachmentOptimal);
    auto depthReference = vk::AttachmentReference()
        .setAttachment(1)
        .setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

    auto subpass = vk::SubpassDescription()
        .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
        .setColorAttachmentCount(1)
        .setPColorAttachments(&colorReference)
        .setPDepthStencilAttachment(&depthReference);

    auto renderPassInfo = vk::RenderPassCreateInfo()
        .setAttachmentCount(2)
        .setPAttachments(attachments)
        .setSubpassCount(1)
        .setPSubpasses(&subpass);

    renderPass = device.createRenderPass(renderPassInfo);
}

void VkApp::InitPipelineCache()
{
    pipelineCache = device.createPipelineCache(vk::PipelineCacheCreateInfo());
}

void VkApp::InitFrameBuffer()
{
    vk::ImageView attachments[2];
    attachments[1] = depthStencil.view;

    auto fbInfo = vk::FramebufferCreateInfo()
        .setRenderPass(renderPass)
        .setAttachmentCount(2)
        .setPAttachments(attachments)
        .setWidth(clientWidth)
        .setHeight(clientHeight)
        .setLayers(1);

    FreeFramebuffers();
    frameBuffers.resize(swapBuffers.size());
    for (uint32_t i = 0; i < frameBuffers.size(); ++i)
    {
        attachments[0] = swapBuffers[i].view;
        device.createFramebuffer(fbInfo);
    }
}

void VkApp::FreeCommandBuffers()
{
    device.freeCommandBuffers(commandPool, drawCmdBuffers);
    device.freeCommandBuffers(commandPool, prePresentCmdBuffer);
    device.freeCommandBuffers(commandPool, postPresentCmdBuffer);
}

void VkApp::FreeDepthStencil()
{
    if (!device || !depthStencil.view)
        return;

    device.destroyImageView(depthStencil.view);
    device.freeMemory(depthStencil.mem);
    device.destroyImage(depthStencil.image);
}

void VkApp::FreeFramebuffers()
{
    if (!device)
        return;

    for (auto &buffer : frameBuffers)
    {
        if (buffer)
        {
            device.destroyFramebuffer(buffer);
            buffer = nullptr;
        }
    }
}

uint32_t VkApp::FindQueue()
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

vk::Format VkApp::GetDepthFormat()
{
    const vk::Format depthFormats[] =
    {
        vk::Format::eD32SfloatS8Uint,
        //vk::Format::eD32Sfloat,
        vk::Format::eD24UnormS8Uint,
        vk::Format::eD16UnormS8Uint,
        //vk::Format::eD16Unorm,
    };

    for (auto &format : depthFormats)
    {
        auto formatProps = physicalDevice.getFormatProperties(format);
        if (formatProps.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
        {
            return format;
        }
    }

    throw std::runtime_error{ "Device doesn't support any acceptable depth-stencil formats" };
}

uint32_t VkApp::GetMemoryType(uint32_t typeBits, vk::MemoryPropertyFlags flags)
{
    auto memProps = physicalDevice.getMemoryProperties();
    for (int i = 0; i < 32; ++i)
    {
        if (typeBits & (1 << i))
        {
            if ((memProps.memoryTypes[i].propertyFlags & flags) == flags)
            {
                return i;
            }
        }
    }
    throw std::runtime_error{ "No suitable memory types" };
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
        device.allocateCommandBuffers(&allocateInfo, &setupCmdBuffer),
        "vk::Device::allocateCommandBuffers"
    );

    setupCmdBuffer.begin(vk::CommandBufferBeginInfo{});
}

void VkApp::FlushSetupCmd()
{
    if (!setupCmdBuffer)
        return;

    setupCmdBuffer.end();

    auto submitInfo = vk::SubmitInfo()
        .setCommandBufferCount(1)
        .setPCommandBuffers(&setupCmdBuffer);

    queue.submit(submitInfo, nullptr);
    queue.waitIdle();

    device.freeCommandBuffers(commandPool, setupCmdBuffer);
    setupCmdBuffer = nullptr;
}


