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

struct DepthStencilBuffer
{
    vk::Image image;
    vk::ImageView view;
    vk::DeviceMemory mem;
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
    void InitCommandBuffers();
    void InitDepthStencil();
    void InitRenderPass();
    void InitPipelineCache();
    void InitFrameBuffer();

    // Free helpers
    void FreeCommandBuffers();
    void FreeDepthStencil();
    void FreeFramebuffers();

    // Helpers
    uint32_t FindQueue();
    vk::Format GetDepthFormat();
    uint32_t GetMemoryType(uint32_t typeBits, vk::MemoryPropertyFlags flags);
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
    vk::Format depthFormat;
    uint32_t queueIndex;

    vk::CommandPool commandPool;
    vk::CommandBuffer setupCmdBuffer;
    vk::CommandBuffer prePresentCmdBuffer;
    vk::CommandBuffer postPresentCmdBuffer;
    std::vector<vk::CommandBuffer> drawCmdBuffers;
    vk::PipelineCache pipelineCache;

    vk::SwapchainKHR swapChain;
    std::vector<SwapChainBuffer> swapBuffers;
    DepthStencilBuffer depthStencil;
    vk::RenderPass renderPass;
    std::vector<vk::Framebuffer> frameBuffers;

    int32_t clientWidth, clientHeight;
};
