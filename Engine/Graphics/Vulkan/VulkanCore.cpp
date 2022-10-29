// Copyright (c) Contributors of Primal+
// Distributed under the MIT license. See the LICENSE file in the project root for more information.
#define VOLK_IMPLEMENTATION 

#include "VulkanCore.h"
#include "VulkanValidation.h"
#include "VulkanSurface.h"
#include "VulkanRenderPass.h"
#include "VulkanCommandBuffer.h"
#include "VulkanResources.h"
#include "VulkanHelpers.h"
#include <set>

namespace primal::graphics::vulkan::core
{
namespace
{
class vulkan_command
{
public:
    vulkan_command() = default;
    DISABLE_COPY_AND_MOVE(vulkan_command);
    explicit vulkan_command(VkDevice device, u32 queue_family_idx, u32 swapchain_image_count)
    {
        VkResult result{ VK_SUCCESS };
        utl::vector<VkCommandBuffer> cmd_buffers;
        _swapchain_image_count = swapchain_image_count;

        // Command pool

        VkCommandPoolCreateInfo info{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        info.queueFamilyIndex = queue_family_idx;							// Queue family type that buffers from this command pool will use
        info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;		// Allows a command buffer created in this pool to implicitly reset itself to initial state

        VkCall(result = vkCreateCommandPool(device, &info, nullptr, &_cmd_pool), "Failed to create command pool...");
        if (result != VK_SUCCESS) goto _error;
        MESSAGE("Created command pool");

        // NOTE: we will only be using 1 queue for any queue family, so queueIndex is always set to 0
        // TODO: add compute queue in the future
        vkGetDeviceQueue(core::logical_device(), core::graphics_family_queue_index(), 0, &_graphics_queue);
        MESSAGE("Found graphics queue");
        vkGetDeviceQueue(core::logical_device(), core::presentation_family_queue_index(), 0, &_presentation_queue);
        MESSAGE("Found presentation queue");

        // Command buffers
        create_command_buffers(device, queue_family_idx);

        // Semaphores & Fences
        {
            _image_available.resize(frame_buffer_count);
            _render_finished.resize(frame_buffer_count);
            _draw_fences.resize(frame_buffer_count);
            VkSemaphoreCreateInfo s_info{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

            // NOTE: fences here are created in an already signaled state, which indicates to Vulkan API that the frame
            //		 has already been rendered. This will prevent them from waiting indefinitely to render first frame.
            for (u32 i{ 0 }; i < (frame_buffer_count); ++i)
            {
                if (vkCreateSemaphore(device, &s_info, nullptr, &_image_available[i]) != VK_SUCCESS ||
                    vkCreateSemaphore(device, &s_info, nullptr, &_render_finished[i]) != VK_SUCCESS ||
                    !create_fence(device, true, _draw_fences[i]))
                {
                    goto _error;
                }
            }

            // NOTE: The in flight fences should not exist yet, so we start with a clear list. They are stored as pointers
            //		 as their initial state should be 0, and will be 0 when they are not in use. Actual fences are not stored
            //		 in this list.
            _fences_in_flight = (vulkan_fence**)malloc(sizeof(vulkan_fence) * _swapchain_image_count);
            for (u32 i{ 0 }; i < _swapchain_image_count; ++i)
            {
                _fences_in_flight[i] = 0;
            }
        }

        return;

    _error:
        release();
    }

    bool begin_frame(vulkan_surface* surface)
    {
        // Are we currently recreating the swapchain?
        if (surface->is_recreating())
        {
            VkResult result{ vkDeviceWaitIdle(core::logical_device()) };
            if (!vulkan_success(result))
            {
                MESSAGE("begin_frame() [1] vkDeviceWaitIdle failed...");
                return false;
            }
            MESSAGE("Resizing swapchain");
            return false;
        }

        // Did the window resize?
        if (surface->is_resized())
        {
            VkResult result{ vkDeviceWaitIdle(core::logical_device()) };
            if (!vulkan_success(result))
            {
                MESSAGE("begin_frame() [2] vkDeviceWaitIdle failed...");
                return false;
            }

            if (!surface->recreate_swapchain())
                return false;

            MESSAGE("Resized");
            return false;
        }

        u32 frame{ surface->current_frame() };

        // Wait for fence signaling the current frame is complete, so this frame can continue
        if (!wait_for_fence(core::logical_device(), _draw_fences[frame], std::numeric_limits<u64>::max()))
        {
            MESSAGE("Draw fence wait failere...");
            return false;
        }

        // Get next swapchain image
        if (!surface->next_image_index(_image_available[frame], nullptr, std::numeric_limits<u64>::max()))
            return false;

        // Begin recording commands
        vulkan_cmd_buffer& cmd_buffer{ _cmd_buffers[frame] };
        reset_cmd_buffer(cmd_buffer);
        begin_cmd_buffer(cmd_buffer, false, false, false);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (f32)surface->width();
        viewport.height = (f32)surface->height();
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        scissor.extent.width = surface->width();
        scissor.extent.height = surface->height();

        vkCmdSetViewport(cmd_buffer.cmd_buffer, 0, 1, &viewport);
        vkCmdSetScissor(cmd_buffer.cmd_buffer, 0, 1, &scissor);

        surface->set_renderpass_render_area({ 0, 0, surface->width(), surface->height() });
        surface->set_renderpass_clear_color({ 0.0f, 0.0f, 0.0f, 0.0f });
        renderpass::begin_renderpass(cmd_buffer.cmd_buffer, cmd_buffer.cmd_state, surface->renderpass(), surface->current_framebuffer());

        return true;
    }

    bool end_frame(vulkan_surface* surface)
    {
        u32 frame{ surface->current_frame() };
        vulkan_cmd_buffer& cmd_buffer{ _cmd_buffers[frame] };

        renderpass::end_renderpass(cmd_buffer.cmd_buffer, cmd_buffer.cmd_state, surface->renderpass());
        end_cmd_buffer(cmd_buffer);

        // Make sure the previous frame is not using this image
        if (_fences_in_flight[frame] != VK_NULL_HANDLE)
            wait_for_fence(core::logical_device(), *_fences_in_flight[frame], std::numeric_limits<u64>::max());

        // Mark the fence as in use by this frame
        _fences_in_flight[frame] = &_draw_fences[frame];

        // Reset the femce for use in next frame
        reset_fence(core::logical_device(), _draw_fences[frame]);

        VkSubmitInfo info{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
        info.commandBufferCount = 1;
        info.pCommandBuffers = &cmd_buffer.cmd_buffer;
        info.signalSemaphoreCount = 1;
        info.pSignalSemaphores = &_render_finished[frame];
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &_image_available[frame];
        VkPipelineStageFlags flags[1]{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        info.pWaitDstStageMask = flags;

        VkResult result{ VK_SUCCESS };
        VkCall(result = vkQueueSubmit(_graphics_queue, 1, &info, _draw_fences[frame].fence), "Failed to submit queue...");
        if (result != VK_SUCCESS) return false;

        update_cmd_buffer_submitted(cmd_buffer);

        surface->present(_image_available[frame], _render_finished[frame], nullptr, _presentation_queue);

        return true;
    }

    void release()
    {
        vkDeviceWaitIdle(core::logical_device());

        for (u32 i{ 0 }; i < (frame_buffer_count); ++i)
        {
            vkDestroySemaphore(core::logical_device(), _render_finished[i], nullptr);
            vkDestroySemaphore(core::logical_device(), _image_available[i], nullptr);
            destroy_fence(core::logical_device(), _draw_fences[i]);
        }
        free(_fences_in_flight);
        for (u32 i{ 0 }; i < _swapchain_image_count; ++i)
        {
            free_cmd_buffer(core::logical_device(), _cmd_pool, _cmd_buffers[i]);
        }
        vkDestroyCommandPool(core::logical_device(), _cmd_pool, nullptr);
    }

    [[nodiscard]] constexpr VkCommandPool const command_pool() const { return _cmd_pool; }
    //[[nodiscard]] constexpr u32 frame_index() const { return _frame_index; }

private:
    void create_command_buffers(VkDevice device, u32 queue_family_idx)
    {
        _cmd_buffers.resize(_swapchain_image_count);

        for (u32 i{ 0 }; i < _swapchain_image_count; ++i)
        {
            if (_cmd_buffers[i].cmd_buffer)
                free_cmd_buffer(device, _cmd_pool, _cmd_buffers[i]);

            // TODO: hardcoded to true for now... 
            _cmd_buffers[i] = allocate_cmd_buffer(device, _cmd_pool, true);
        }
    }

    bool create_fence(VkDevice device, bool signaled, vulkan_fence& fence)
    {
        fence.signaled = signaled;

        VkFenceCreateInfo f_info{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        if (signaled)
            f_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;	// this ensures the fence starts signaled as open

        VkResult result{ VK_SUCCESS };
        VkCall(result = vkCreateFence(device, &f_info, nullptr, &fence.fence), "Failed to create fence");
        if (result != VK_SUCCESS) return false;

        return true;
    }

    void destroy_fence(VkDevice device, vulkan_fence& fence)
    {
        vkDestroyFence(device, fence.fence, nullptr);
        fence.fence = nullptr;
        fence.signaled = false;
    }

    bool wait_for_fence(VkDevice device, vulkan_fence& fence, u64 timeout)
    {
        if (!fence.signaled)
        {
            VkResult result{ vkWaitForFences(device, 1, &fence.fence, true, timeout) };
            switch (result)
            {
            case VK_SUCCESS:
                fence.signaled = true;
                return true;
                break;
            case VK_TIMEOUT:
                MESSAGE("Fence timed out...");
                break;
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                MESSAGE("Out of host memory error on fence wait...");
                break;
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                MESSAGE("Out of host device error on fence wait...");
                break;
            case VK_ERROR_DEVICE_LOST:
                MESSAGE("Device lost error on fence wait...");
                break;
            default:
                MESSAGE("Unknown error on fence wait...");
                break;
            }
        }
        else
        {
            return true;
        }

        return false;
    }

    void reset_fence(VkDevice device, vulkan_fence& fence)
    {
        if (fence.signaled)
        {
            VkCall(vkResetFences(device, 1, &fence.fence), "Failed to reset fence...");
            fence.signaled = false;
        }
    }

    VkCommandPool					_cmd_pool{ nullptr };
    VkQueue							_graphics_queue{ nullptr };
    VkQueue							_presentation_queue{ nullptr };
    utl::vector<vulkan_cmd_buffer>	_cmd_buffers;
    utl::vector<vulkan_fence>		_draw_fences;
    vulkan_fence**					_fences_in_flight;
    utl::vector<VkSemaphore>		_image_available;
    utl::vector<VkSemaphore>		_render_finished;
    u32								_swapchain_image_count{ 0 };
};

// Indices (locations) of Queue Families (if they exist at all)
struct queue_family_indices
{
    u32 graphics_family{ u32_invalid_id };			// Location of Graphics Queue Family
    u32 presentation_family{ u32_invalid_id };		// Location of Presentation Queue Family

    // Check if queue families are valid
    bool is_valid() { return graphics_family >= 0 && presentation_family >= 0; }
} queue_family_indices;

struct device_group
{
    VkPhysicalDevice physical_device;
    VkDevice logical_device;
} device_group;

using surface_collection = utl::free_list<vulkan_surface>;

const utl::vector<const char*>	device_extensions{ 1, VK_KHR_SWAPCHAIN_EXTENSION_NAME };
VkInstance						instance{ nullptr };
VkFormat						device_depth_format{ VK_FORMAT_UNDEFINED };
vulkan_command					gfx_command;
VkDebugUtilsMessengerEXT		debug_messenger{ 0 };
surface_collection				surfaces;

bool
check_instance_ext_support(utl::vector<const char*>* check_ext)
{
    // Need to get number of extensions to create array of correct size to hold extensions
    u32 extension_count{ 0 };
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);

    // No need to go further if there are no extensions supported
    if (extension_count == 0) return false;

    // Create a list of vkExtensionProperties using extensionCount
    utl::vector<VkExtensionProperties> extensions(extension_count);
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());

    // Check if given extensions are in the list of available extensions
    for (const auto& check : *check_ext)
    {
        bool has_extension = false;
        for (const auto& ext : extensions)
        {
            if (strcmp(check, ext.extensionName) == 0)
            {
                has_extension = true;
                break;
            }
        }

        if (!has_extension) return false;
    }

    return true;
}

void
set_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& info)
{
    info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    info.pfnUserCallback = debug_callback;
    info.pUserData = nullptr;
}

void
get_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    // Get all Queue Family Property info for the given device
    u32 queue_family_count{ 0 };
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

    utl::vector<VkQueueFamilyProperties> queue_family_list(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_family_list.data());

    // Go through each queue family and check if it has at least one of the required tupe of queue
    u32 i{ 0 };
    for (const auto& queue_family : queue_family_list)
    {
        // First check if queue family has at least one queue in that family (could have none)
        // Queue can be multiple types defined through a bitfield. Here we check if the graphics bit is set.
        if (queue_family.queueCount > 0 && (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT))
        {
            queue_family_indices.graphics_family = i;		// If queue family is valid, then get index
        }

        // Check if queue family supports presentation
        VkBool32 presentation_support{ false };
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentation_support);
        // Check if queue is presentation type (can be both graphics and presentation)
        if (queue_family.queueCount > 0 && presentation_support)
            queue_family_indices.presentation_family = i;

        if (queue_family_indices.is_valid()) break;		// If queue family indices are in a valid state, break loop

        i++;
    }
}

bool
check_device_extension_support(VkPhysicalDevice device)
{
    // Need to get number of extensions to create array of correct size to hold extensions
    u32 extension_count{ 0 };
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

    // No need to go further if there are no extensions supported
    if (extension_count == 0) return false;

    // Create a list of vkExtensionProperties using extensionCount
    utl::vector<VkExtensionProperties> extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, extensions.data());

    // Check if given extensions are in the list of available extensions
    for (const auto& device_ext : device_extensions)
    {
        bool has_ext = false;
        for (const auto& extension : extensions)
        {
            if (strcmp(device_ext, extension.extensionName) == 0)
            {
                has_ext = true;
                break;
            }
        }

        if (!has_ext) return false;
    }

    return true;
}

bool
check_device_suitable(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    get_queue_families(device, surface);

    bool extensions_supported{ check_device_extension_support(device) };



    bool swapchain_valid{ false };
    if (extensions_supported)
    {
        swapchain_details swapchain_details = get_swapchain_details(device, surface);
        swapchain_valid = !swapchain_details.formats.empty() && !swapchain_details.presentation_modes.empty();
    }

    return queue_family_indices.is_valid() && extensions_supported && swapchain_valid;
}

bool
get_physical_device(VkSurfaceKHR surface)
{
    u32 device_count{ 0 };
    VkResult result{ VK_SUCCESS };

    // Get number of physical devices (GPUs, etc...) the instance can access
    VkCall(result = vkEnumeratePhysicalDevices(instance, &device_count, nullptr), "Failed to get physical device count...");
    if (result != VK_SUCCESS) return false;
    assert(device_count > 0);

    // Make sure there is at least one device that supports Vulkan
    if (device_count == 0)
    {
        MESSAGE("Can't find a GPU that supports Vulkan instance...");
        return false;
    }

    // Create a list of physical devices the instance can access
    utl::vector<VkPhysicalDevice> devices(device_count);
    VkCall(result = vkEnumeratePhysicalDevices(instance, &device_count, devices.data()), "Failed to get a list of physical devices...");
    if (result != VK_SUCCESS) return false;

    // Choose first physical device that meets requirements
    for (const auto& d : devices)
    {
        if (check_device_suitable(d, surface))
        {
            device_group.physical_device = d;
            break;
        }
    }

    assert(device_group.physical_device);
    if (!device_group.physical_device)
    {
        MESSAGE("Failed to find suitable physical device...");
        return false;
    }

    MESSAGE("Physical device found successfully");

    return true;
}

bool
create_logical_device()
{
    // Vector for queue creation information, and set for family indices
    utl::vector<VkDeviceQueueCreateInfo> infos{};
    std::set<u32> indices{ queue_family_indices.graphics_family, queue_family_indices.presentation_family };

    // Queues the logical device needs to create and info to do so
    for (u32 queue_family_index : indices)
    {
        VkDeviceQueueCreateInfo info{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
        info.queueFamilyIndex = queue_family_index;				// the index of the family to create a queue from
        info.queueCount = 1;										// Number of queues to create
        f32 priority{ 1.0f };
        info.pQueuePriorities = &priority;						// Vulkan needs to know how to handle multiple queues (1 highest priority, 0 lowest)

        infos.push_back(info);
    }

    // Information to create logical device (sometimes called "device" for short)
    VkDeviceCreateInfo info{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    info.queueCreateInfoCount = (u32)infos.size();		// number of queue create infos
    info.pQueueCreateInfos = infos.data();				// List of queue create infos so device can create required queues
    info.enabledExtensionCount = (u32)device_extensions.size();	// NUmber of enabled logical device extensions
    info.ppEnabledExtensionNames = device_extensions.data();		// List of enabled logical device extensions

    // Physical device features the logical device will be using
    VkPhysicalDeviceFeatures device_features{};

    info.pEnabledFeatures = &device_features;					// Physical device features logical device will use

    VkResult result{ VK_SUCCESS };
    VkCall(result = vkCreateDevice(device_group.physical_device, &info, nullptr, &device_group.logical_device), "Failed to create a logical device...");
    if (result != VK_SUCCESS) return false;

    MESSAGE("Logical Device created successfully");

    return true;
}

bool
failed_init()
{
    shutdown();
    return false;
}
} // anonymous namespace

// Function Pointers
// NOTE: some of these will move into VulkanSurface
PFN_vkGetPhysicalDeviceSurfaceSupportKHR		fpGetPhysicalDeviceSurfaceSupportKHR;
PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR	fpGetPhysicalDeviceSurfaceCapabilitiesKHR;
PFN_vkGetPhysicalDeviceSurfaceFormatsKHR		fpGetPhysicalDeviceSurfaceFormatsKHR;
PFN_vkGetPhysicalDeviceSurfacePresentModesKHR	fpGetPhysicalDeviceSurfacePresentModesKHR;

bool
initialize()
{
    if (instance) shutdown();

    if (volkInitialize() != VK_SUCCESS)
    {
        return failed_init();
    }

    if (enable_validation_layers && !validation_layer_supported())
    {
        MESSAGE("Validation layers requested, but not available...");
        shutdown();
    }

    // Information about the application itself
    // TODO: Hardcoded for now... handle properly in the future
    VkApplicationInfo app_info{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
    app_info.pApplicationName = "Application Name";				// custom name of app
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);		// custom version of app
    app_info.pEngineName = "Primal";							//custom name of engine
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);			// custom version of engine
    app_info.apiVersion = VK_API_VERSION_1_2;					// Version of Vulkan API

    // List of instance extensions we need to have available
    utl::vector<const char*> instance_ext{ 1, VK_KHR_SURFACE_EXTENSION_NAME };

    // Add appropriate OS specific surface extension to the list.
    // For now, only Windows and Linux XLib are supported.
#ifdef _WIN32
    instance_ext.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif __linux__
    instance_ext.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
#endif // _WIN32

    // If validation enabled, add extension to report debug info
    if (enable_validation_layers)
        instance_ext.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    // Check to see if instance extensions are supported
    if (!check_instance_ext_support(&instance_ext))
    {
        MESSAGE("VKInstance does not support required extensions...");
        shutdown();
    }

    // Additional debug messenger that will be used during instance creation and instance destruction
    VkDebugUtilsMessengerCreateInfoEXT debug_create_info{ VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };

    // Create info for Vulkan instance
    VkInstanceCreateInfo info{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    info.pApplicationInfo = &app_info;
    info.enabledExtensionCount = (u32)instance_ext.size();
    info.ppEnabledExtensionNames = instance_ext.data();

    if (enable_validation_layers)
    {
        set_debug_messenger_create_info(debug_create_info);
        info.enabledLayerCount = _countof(validation_layers);
        info.ppEnabledLayerNames = &validation_layers[0];
        info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_create_info;
    }
    else
    {
        info.enabledLayerCount = 0;
        info.ppEnabledLayerNames = nullptr;
        info.pNext = nullptr;
    }

    VkResult result{ VK_SUCCESS };
    VkCall(result = vkCreateInstance(&info, nullptr, &instance), "Failed to create a Vulkan instance...");
    if (result != VK_SUCCESS) return failed_init();

    MESSAGE("Vulkan instance created");

    volkLoadInstance(instance);

    // Now that we have an instance, if enable_validation_layers, we can create the debug messenger
    if (enable_validation_layers)
    {
        VkDebugUtilsMessengerCreateInfoEXT create_info{ VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
        set_debug_messenger_create_info(create_info);

        VkCall(result = create_debug_utils_messenger_ext(instance, &create_info, nullptr, &debug_messenger), "Failed to set up debug messenger...");
        if (result != VK_SUCCESS) return failed_init();

        MESSAGE("Vulkan validation layer created");
    }

    GET_INSTANCE_PROC_ADDR(instance, GetPhysicalDeviceSurfaceSupportKHR);
    GET_INSTANCE_PROC_ADDR(instance, GetPhysicalDeviceSurfaceCapabilitiesKHR);
    GET_INSTANCE_PROC_ADDR(instance, GetPhysicalDeviceSurfaceFormatsKHR);
    GET_INSTANCE_PROC_ADDR(instance, GetPhysicalDeviceSurfacePresentModesKHR);

    MESSAGE("Vulkan initialized successfully");

    return true;
}

void
shutdown()
{
    gfx_command.release();
    vkDestroyDevice(device_group.logical_device, nullptr);

    if (enable_validation_layers)
        destroy_debug_utils_messenger_ext(instance, debug_messenger, nullptr);

    vkDestroyInstance(instance, nullptr);
}

bool
create_device(VkSurfaceKHR surface)
{
    // These should only be created once
    if (device_group.logical_device || device_group.physical_device) return true;
    assert(!device_group.logical_device && !device_group.physical_device);

    return (get_physical_device(surface) && create_logical_device());
}

bool
create_graphics_command(u32 swapchain_image_count)
{
    // This should only be created once
    if (gfx_command.command_pool()) return true;
    assert(!gfx_command.command_pool());

    new (&gfx_command) vulkan_command(device_group.logical_device, queue_family_indices.graphics_family, swapchain_image_count);
    if (!gfx_command.command_pool()) return false;

    return true;
}

bool
detect_depth_format(VkPhysicalDevice physical_device)
{
    if (device_depth_format != VK_FORMAT_UNDEFINED) return true;

    const u32 count{ 3 };
    VkFormat formats[3]{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
    u32 flags{ VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT };

    for (u32 i{ 0 }; i < count; ++i)
    {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(physical_device, formats[i], &properties);

        if ((properties.linearTilingFeatures & flags) == flags)
        {
            device_depth_format = formats[i];
            return true;
        }
        else if ((properties.optimalTilingFeatures & flags) == flags)
        {
            device_depth_format = formats[i];
            return true;
        }
    }

    return false;
}

s32
find_memory_index(u32 type, u32 flags)
{
    VkPhysicalDeviceMemoryProperties properties;
    vkGetPhysicalDeviceMemoryProperties(device_group.physical_device, &properties);

    for (u32 i{ 0 }; i < properties.memoryTypeCount; ++i)
    {
        if (type & (1 << i) && (properties.memoryTypes[i].propertyFlags & flags) == flags)
            return i;
    }

    MESSAGE("Cannot find memory type...");
    return -1;
}

u32
graphics_family_queue_index()
{
    return queue_family_indices.graphics_family;
}

u32
presentation_family_queue_index()
{
    return queue_family_indices.presentation_family;
}

VkPhysicalDevice
physical_device()
{
    return device_group.physical_device;
}

VkDevice
logical_device()
{
    return device_group.logical_device;
}

VkInstance
get_instance()
{
    return instance;
}

VkFormat
depth_format()
{
    return device_depth_format;
}

surface
create_surface(platform::window window)
{
    surface_id id{ surfaces.add(window) };
    surfaces[id].create(instance);
    return surface{ id };
}

void
remove_surface(surface_id id)
{
    surfaces.remove(id);
}

void
resize_surface(surface_id id, u32, u32)
{
    surfaces[id].resize();
}

u32
surface_width(surface_id id)
{
    return surfaces[id].width();
}

u32
surface_height(surface_id id)
{
    return surfaces[id].height();
}

void
render_surface(surface_id id, frame_info info)
{
    if (gfx_command.begin_frame(&surfaces[id]))
    {
        //
        // ....
        //

        gfx_command.end_frame(&surfaces[id]);
    }
}
}