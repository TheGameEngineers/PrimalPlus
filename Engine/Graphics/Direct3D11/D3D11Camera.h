#pragma once
#include "D3D11CommonHeaders.h"

#if PRIMAL_BUILD_D3D11

namespace primal::graphics::d3d11::camera {

class d3d11_camera
{
public:
	explicit d3d11_camera(camera_init_info info);

	void update();
	void up(math::v3 up);
	constexpr void field_of_view(f32 fov);
	constexpr void aspect_ratio(f32 aspect_ratio);
	constexpr void view_width(f32 width);
	constexpr void view_height(f32 height);
	constexpr void near_z(f32 near_z);
	constexpr void far_z(f32 far_z);

	_NODISCARD constexpr DirectX::XMMATRIX view() const { return _view; }
	_NODISCARD constexpr DirectX::XMMATRIX projection() const { return _projection; }
	_NODISCARD constexpr DirectX::XMMATRIX inverse_projection() const { return _inverse_projection; }
	_NODISCARD constexpr DirectX::XMMATRIX view_projection() const { return _view_projection; }
	_NODISCARD constexpr DirectX::XMMATRIX inverse_view_projection() const { return _inverse_view_projection; }
	_NODISCARD constexpr DirectX::XMVECTOR position() const { return _position; }
	_NODISCARD constexpr DirectX::XMVECTOR direction() const { return _direction; }
	_NODISCARD constexpr DirectX::XMVECTOR up() const { return _up; }
	_NODISCARD constexpr f32 near_z() const { return _near_z; }
	_NODISCARD constexpr f32 far_z() const { return _far_z; }
	_NODISCARD constexpr f32 field_of_view() const { return _field_of_view; }
	_NODISCARD constexpr f32 aspect_ratio() const { return _aspect_ratio; }
	_NODISCARD constexpr f32 view_width() const { return _view_width; }
	_NODISCARD constexpr f32 view_height() const { return _view_height; }
	_NODISCARD constexpr graphics::camera::type projection_type() const { return _projection_type; }
	_NODISCARD constexpr id::id_type entity_id() const { return _entity_id; }

private:
	DirectX::XMMATRIX		_view;
	DirectX::XMMATRIX		_projection;
	DirectX::XMMATRIX		_inverse_projection;
	DirectX::XMMATRIX		_view_projection;
	DirectX::XMMATRIX		_inverse_view_projection;
	DirectX::XMVECTOR		_position{};
	DirectX::XMVECTOR		_direction{};
	DirectX::XMVECTOR		_up;
	f32						_near_z;
	f32						_far_z;
	union {
		f32					_field_of_view;
		f32					_view_width;
	};
	union {
		f32					_aspect_ratio;
		f32					_view_height;
	};
	graphics::camera::type	_projection_type;
	id::id_type				_entity_id;
	bool					_is_dirty;
};

graphics::camera create(camera_init_info info);
void remove(camera_id id);
void set_parameter(camera_id id, camera_parameter::parameter parameter, const void* const data, u32 size);
void get_parameter(camera_id id, camera_parameter::parameter parameter, void* const data, u32 size);
_NODISCARD d3d11_camera& get(camera_id id);
}

#endif
