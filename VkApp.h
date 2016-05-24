#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vk_cpp.h>

class VkApp
{
public:
    VkApp();
    ~VkApp();

    vk::Instance instance;
};
