#include "OpenglCommonHeaders.h"
#include <wtypes.h>
#include "OpenglSurface.h"
namespace primal::graphics::opengl::core
{
    namespace {
        using surface_collection = utl::free_list<opengl_surface>;
        surface_collection				surfaces;
    }
	bool initialize(){
		return true;
	}
	void shutdown(){

	}
	surface create_surface(platform::window window){
		surface_id id{ surfaces.add(window) };
		surfaces[id].create_surface();
		return surface{ id };
	}
	void remove_surface(surface_id id){
        surfaces.remove(id);
	}
	void resize_surface(surface_id id, u32, u32){
        surfaces[id].resize();
	}
	u32 surface_width(surface_id id){
        return surfaces[id].width();
	}
	u32 surface_height(surface_id id){
        return surfaces[id].height();
	}
	void render_surface(surface_id id){

	}
}