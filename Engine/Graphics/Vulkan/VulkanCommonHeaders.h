// Copyright (c) Contributors of Primal+
// Distributed under the MIT license. See the LICENSE file in the project root for more information.
#pragma once

#include "CommonHeaders.h"
#include "Graphics/Renderer.h"

#include <vulkan/vulkan.h>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#ifndef WIN_32_LEAN_AND_MEAN
#define WIN_32_LEAN_AND_MEAN
#endif // WIN_32_LEAN_AND_MEAN
#include <Windows.h>
#include <vulkan/vulkan_win32.h>
#include <wrl.h>
#elif __linux__
#include <iostream>
#include <X11/Xlib.h>
#include <vulkan/vulkan_xlib.h>
#endif // _WIN32

#pragma comment(lib, "vulkan-1.lib")


namespace primal::graphics::vulkan {

constexpr u32 frame_buffer_count{ 3 };

struct vulkan_image
{
    VkImage			image;
    VkDeviceMemory	memory;
    VkImageView		view;
    u32				width;
    u32				height;
};

struct vulkan_renderpass
{
    enum state : u32 {
        READY,
        RECORDING,
        IN_RENDER_PASS,
        RECORDING_ENDED,
        SUBMITTED,
        NOT_ALLOCATED
    };

    VkRenderPass	render_pass;
    math::u32v4		render_area;
    math::v4		clear_color;
    f32				depth;
    u32				stencil;
};

struct vulkan_cmd_buffer
{
    enum state : u32 {
        CMD_READY,
        CMD_RECORDING,
        CMD_IN_RENDER_PASS,
        CMD_RECORDING_ENDED,
        CMD_SUBMITTED,
        CMD_NOT_ALLOCATED
    };

    VkCommandBuffer cmd_buffer;
    state           cmd_state;
};

struct vulkan_framebuffer
{
    VkFramebuffer				framebuffer;
    u32							attach_count;
    utl::vector<VkImageView>	attachments;
    vulkan_renderpass*			renderpass;
};

struct vulkan_fence
{
    VkFence	fence;
    bool	signaled;
};
}

#ifdef _DEBUG
#ifndef VkCall
#ifdef _WIN32
#define VkCall(x, msg)                              \
if(((VkResult)(x)) != VK_SUCCESS) {                 \
    char line_number[32];                           \
    sprintf_s(line_number, "%u", __LINE__);         \
    OutputDebugStringA("Error in: ");               \
    OutputDebugStringA(__FILE__);                   \
    OutputDebugStringA("\nLine: ");                 \
    OutputDebugStringA(line_number);                \
    OutputDebugStringA("\n");                       \
    OutputDebugStringA(msg);                        \
    OutputDebugStringA("\n");                       \
    __debugbreak();                                 \
}
#elif __linux__
#define VkCall(x, msg)                              \
if (((VkResult)(x)) != VK_SUCCESS) {                \
    std::cout << msg << std::endl;                  \
    __builtin_trap();                               \
}
#endif // _WIN32
#endif // !VkCall
#else
#ifndef VkCall
#define VkCall(x, msg) x
#endif // !VkCall
#endif // _DEBUG

#ifndef MESSAGE
#ifdef _WIN32
#define MESSAGE(x)      \
OutputDebugStringA(x);  \
OutputDebugStringA("\n")
#elif __linux__
#define MESSAGE(x) std::cout << x << std::endl
#endif // _WIN32
#endif // !MESSAGE

#ifndef ERROR_MSSG
#ifdef _WIN32
#define ERROR_MSSG(x)       \
OutputDebugStringA(x);      \
OutputDebugStringA("\n");   \
__debugbreak()
#elif __linux__
#define ERROR_MSSG(x)       \
std::cout << x << std::endl \
__builtin_trap()
#endif // _WIN32
#endif // !ERROR_MSSG

#ifndef GET_INSTANCE_PROC_ADDR
#define GET_INSTANCE_PROC_ADDR(inst, entry)											\
{																					\
    fp##entry = (PFN_vk##entry)vkGetInstanceProcAddr(inst, "vk"#entry);				\
    if (!fp##entry)																	\
        throw std::runtime_error("vkGetInstanceProcAddr failed to find vk"#entry);	\
}
#endif // !GET_INSTANCE_PROC_ADDR

#ifndef GET_DEVICE_PROC_ADDR
#define GET_DEVICE_PROC_ADDR(dev, entry)										    \
{																				    \
    fp##entry = (PFN_vk##entry)vkGetDeviceProcAddr(dev, "vk"#entry);			    \
    if (!fp##entry)																    \
        throw std::runtime_error("vkGetDeviceProcAddr failed to find vk"#entry);    \
}
#endif // !GET_DEVICE_PROC_ADDR