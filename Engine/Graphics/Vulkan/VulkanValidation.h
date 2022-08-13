// Copyright (c) Contributors of Primal+
// Distributed under the MIT license. See the LICENSE file in the project root for more information.
#pragma once

#include "VulkanCommonHeaders.h"

namespace primal::graphics::vulkan
{
// List of validation layers to use
// VK_LAYER_KHRONOS_validation = All standard validation layers
constexpr const char* validation_layers[]{ "VK_LAYER_KHRONOS_validation" };

// Only enable validation layers if in debug mode
#ifdef _DEBUG
constexpr bool enable_validation_layers{ true };
#else
constexpr bool enable_validation_layers{ false };
#endif

inline bool
validation_layer_supported()
{
    uint32_t layer_count{ 0 };
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

    utl::vector<VkLayerProperties> available_layers(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

    for (const auto& layer_name : validation_layers)
    {
        bool layer_found{ false };
        for (const auto& layer_properties : available_layers)
        {
            if (strcmp(layer_name, layer_properties.layerName) == 0)
            {
                layer_found = true;
                break;
            }
        }

        if (!layer_found) return false;
    }

    return true;
}

// Callback function for validation debugging (will be called when validation information record)
inline VKAPI_ATTR VkBool32 VKAPI_CALL
debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data)
{
    if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        // Message is important enough to show
        MESSAGE("Validation layer:");
        MESSAGE(callback_data->pMessage);
    }

    // VK_FALSE indicates that the program should not terminate
    return VK_FALSE;
}

inline VkResult
create_debug_utils_messenger_ext(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* create_info,
    const VkAllocationCallbacks* allocator, VkDebugUtilsMessengerEXT* debug_messenger)
{
    // vkGetInstanceProcAddr returns a function pointer to the requested function in the requested instance
    // Resulting function is cast as a function pointer with the header of "vkCreateDebugUtilsMessengerEXT"
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    // If function was found, executre if with given data and return result, otherwise, return error
    if (func != nullptr)
    {
        return func(instance, create_info, allocator, debug_messenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

inline void
destroy_debug_utils_messenger_ext(VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger, const VkAllocationCallbacks* allocator)
{
    // Get function pointer to requested function, then cast to function pointer for vkDestroyDebugUtilsMessengerEXT
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    // If function found, execute
    if (func != nullptr)
    {
        func(instance, debug_messenger, allocator);
    }
}
}