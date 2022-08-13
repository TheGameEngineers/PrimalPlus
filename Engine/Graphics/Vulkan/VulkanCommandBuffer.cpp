// Copyright (c) Contributors of Primal+
// Distributed under the MIT license. See the LICENSE file in the project root for more information.
#include "VulkanCommandBuffer.h"

namespace primal::graphics::vulkan
{
vulkan_cmd_buffer
allocate_cmd_buffer(VkDevice device, VkCommandPool cmd_pool, bool primary)
{
    vulkan_cmd_buffer cmd_buffer{};

    VkCommandBufferAllocateInfo info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    info.commandPool = cmd_pool;
    info.level = primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY; // secondary buffers must be used with another buffer, not on it's own
    info.commandBufferCount = 1;
    info.pNext = nullptr;

    cmd_buffer.cmd_state = vulkan_cmd_buffer::CMD_NOT_ALLOCATED;
    VkResult result{ VK_SUCCESS };
    VkCall(result = vkAllocateCommandBuffers(device, &info, &cmd_buffer.cmd_buffer), "Failed to allocate command buffer...");
    cmd_buffer.cmd_state = vulkan_cmd_buffer::CMD_READY;

    MESSAGE("Command buffer allocated");

    return cmd_buffer;
}

void
free_cmd_buffer(VkDevice device, VkCommandPool cmd_pool, vulkan_cmd_buffer& cmd_buffer)
{
    vkFreeCommandBuffers(device, cmd_pool, 1, &cmd_buffer.cmd_buffer);

    cmd_buffer.cmd_buffer = nullptr;
    cmd_buffer.cmd_state = vulkan_cmd_buffer::CMD_NOT_ALLOCATED;
}

void
begin_cmd_buffer(vulkan_cmd_buffer& cmd_buffer, bool single_use, bool renderpass_continue, bool simultaneous_use)
{
    VkCommandBufferBeginInfo info{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    info.flags = 0;

    // NOTE: Single use cannot be used more than once, and will be reset and recorded again between each submission
    //		 Renderpass continue indicates it is a secondary buffer entirely inside a render pass. It is ignored for primary buffers
    //		 Simultaneous use can be resubmitted to a queue while it is in the pending state. and recorded into multiple primary command buffers
    if (single_use)
        info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (renderpass_continue)
        info.flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    if (simultaneous_use)
        info.flags |= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    VkResult result{ VK_SUCCESS };
    VkCall(result = vkBeginCommandBuffer(cmd_buffer.cmd_buffer, &info), "Failed to begin command buffer...");
    cmd_buffer.cmd_state = vulkan_cmd_buffer::CMD_RECORDING;
}

void
end_cmd_buffer(vulkan_cmd_buffer& cmd_buffer)
{
    VkResult result{ VK_SUCCESS };
    // TODO: Check to make sure command buffer is in a state where it can be ended prior to ending
    VkCall(result = vkEndCommandBuffer(cmd_buffer.cmd_buffer), "Failed to end command buffer...");
    cmd_buffer.cmd_state = vulkan_cmd_buffer::CMD_RECORDING_ENDED;
}

void
update_cmd_buffer_submitted(vulkan_cmd_buffer& cmd_buffer)
{
    cmd_buffer.cmd_state = vulkan_cmd_buffer::CMD_SUBMITTED;
}

void
reset_cmd_buffer(vulkan_cmd_buffer& cmd_buffer)
{
    cmd_buffer.cmd_state = vulkan_cmd_buffer::CMD_READY;
}

// This must be a single use, primary, command buffer
vulkan_cmd_buffer
allocate_cmd_buffer_begin_single_use(VkDevice device, VkCommandPool cmd_pool)
{
    vulkan_cmd_buffer cmd_buffer{};
    cmd_buffer = allocate_cmd_buffer(device, cmd_pool, true);
    begin_cmd_buffer(cmd_buffer, true, false, false);

    return cmd_buffer;
}

// This will end and submit a single use command buffer
// NOTE: will not use a fence
void
end_cmd_buffer_single_use(VkDevice device, VkCommandPool cmd_pool, vulkan_cmd_buffer& cmd_buffer, VkQueue queue)
{
    end_cmd_buffer(cmd_buffer);

    VkSubmitInfo info{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    info.commandBufferCount = 1;
    info.pCommandBuffers = &cmd_buffer.cmd_buffer;

    // Submit command buffer to the given queue
    VkResult result{ VK_SUCCESS };
    VkCall(result = vkQueueSubmit(queue, 1, &info, nullptr), "Failed to submit single use command buffer to queue...");

    // Wait for the queue to finish doing what it is doing
    VkCall(result = vkQueueWaitIdle(queue), "vkQueueWaitIdle failed in end_cmd_single_use()");

    // Then free the single use command buffer
    free_cmd_buffer(device, cmd_pool, cmd_buffer);
}
}