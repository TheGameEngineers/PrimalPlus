#pragma once

namespace primal::graphics {
struct platform_interface;

namespace d3d11 {
void get_platform_interface(platform_interface& pi);
}
}