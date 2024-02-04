// Copyright (c) Contributors of Primal+
// Distributed under the MIT license. See the LICENSE file in the project root for more information.
#include "GraphicsPlatformInterface.h"
#include "Direct3D12\D3D12Interface.h"
#include "Direct3D11\D3D11Interface.h"
#include "Vulkan\VulkanInterface.h"

namespace primal::graphics {
#include "GraphicsPlatform.h"

bool
set_platform_interface(graphics_platform platform, platform_interface& pi)
{
    switch (platform)
    {
    case graphics_platform::direct3d12:
        #if defined (_WIN64)
        d3d12::get_platform_interface(pi);
        #else
        return false;
        #endif
        break;
    case graphics_platform::direct3d11:
#if defined (_WIN64)
		d3d11::get_platform_interface(pi);
#else
        return false;
#endif
		break;
    case graphics_platform::vulkan_1:
        vulkan::get_platform_interface(pi);
        break;
    default:
        return false;
    }

    assert(pi.platform == platform);
    return true;
}

}
