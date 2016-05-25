#pragma once

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#else
// TODO: Non-windows
#endif

#include <vulkan/vk_cpp.h>
#include <memory>
#include <vector>

class Window;

struct SwapChainBuffer
{
    vk::Image image;
    vk::ImageView view;
};

class VkApp
{
public:
    VkApp();
    ~VkApp();

    Window *GetWindow();

private:
    // Init routines
    void InitInstance();
    void InitDevice();
    void InitWindow();
    void InitCommandPool();
    void InitSwapChain();

    // Helpers
    size_t FindQueue();
    void SetImageLayout(
        vk::CommandBuffer commandBuffer,
        vk::Image image,
        vk::ImageAspectFlags aspectMask,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout
    );
    void InitSetupCmd();
    void FlushSetupCmd();

    std::unique_ptr<Window> window;
    vk::Instance instance;
    vk::PhysicalDevice physicalDevice;
    vk::Device device;
    vk::Queue queue;
    vk::SurfaceKHR surface;
    vk::Format colorFormat;
    vk::ColorSpaceKHR colorSpace;

    vk::CommandPool commandPool;
    vk::CommandBuffer setupCmd;

    vk::SwapchainKHR swapChain;
    std::vector<vk::Image> images;
    std::vector<SwapChainBuffer> buffers;
    size_t nodeIndex;

    int32_t clientWidth, clientHeight;
};
