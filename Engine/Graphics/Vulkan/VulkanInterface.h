// Copyright (c) Contributors of Primal+
// Distributed under the MIT license. See the LICENSE file in the project root for more information.
#pragma once

namespace primal::graphics {

struct platform_interface;

namespace vulkan {
	
void get_platform_interface(platform_interface& pi);

} // vulkan namespace

}