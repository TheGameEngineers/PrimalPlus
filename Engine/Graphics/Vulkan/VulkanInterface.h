#pragma once

namespace primal::graphics
{
	struct platform_interface;

	namespace vulkan
	{
		void get_platform_interface(platform_interface& pi);
	}
}