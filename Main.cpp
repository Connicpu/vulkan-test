#include "VkApp.h"
#include "Window.h"
#include <iostream>

INT WINAPI WinMain(HINSTANCE hinstance, HINSTANCE, LPSTR, INT)
{
    // Initialize the instance
    VkApp app;

    // Get the first device
    auto physicalDevices = app.instance.enumeratePhysicalDevices();
    auto physicalDevice = physicalDevices[0];

    // Set up the queue info
    float queuePriorities[] = { 1.0f };
    auto devQueueInfo = vk::DeviceQueueCreateInfo()
        .setQueueCount(1)
        .setPQueuePriorities(queuePriorities);

    // Set up the device info
    auto devInfo = vk::DeviceCreateInfo()
        .setQueueCreateInfoCount(1)
        .setPQueueCreateInfos(&devQueueInfo);

    // Create the device!
    auto device = physicalDevice.createDevice(devInfo);

    // Do window stuff
    vk::SurfaceKHR surface;
    {
        Window window{ hinstance };
        auto hwnd = window.GetHandle();

        auto surfaceInfo = vk::Win32SurfaceCreateInfoKHR()
            .setHinstance(hinstance)
            .setHwnd(hwnd);

        // Create a surface for the window
        surface = app.instance.createWin32SurfaceKHR(surfaceInfo);

        // Get the color format to use
        vk::Format colorFormat;
        auto surfaceFormats = physicalDevice.getSurfaceFormatsKHR(surface);
        if (surfaceFormats.size() == 1 && surfaceFormats[0].format == vk::Format::eUndefined)
            colorFormat = vk::Format::eB8G8R8A8Unorm;
        else
            colorFormat = surfaceFormats[0].format;
        auto colorSpace = surfaceFormats[0].colorSpace;

        MSG msg;
        while (GetMessageW(&msg, window.GetHandle(), 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);

            if (window.Closed())
                break;
        }
    }

    app.instance.destroySurfaceKHR(surface);
    device.destroy();
    return 0;
}

