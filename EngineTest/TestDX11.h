#pragma once
#include "Test.h"
#include "Components/Entity.h"
#include "Components/Script.h"
#include "Components/Geometry.h"
#include "Content/ContentToEngine.h"
#include "Content/ContentLoader.h"
#include "EngineAPI/GameEntity.h"
#include "EngineAPI/Light.h"
#include "EngineAPI/ScriptComponent.h"
#include "EngineAPI/TransformComponent.h"
#include "Graphics/Renderer.h"
#include "Platform/PlatformTypes.h"
#include "Platform/Platform.h"
#include "Input/Input.h"
#include "Utilities/IOStream.h"

#include "../ContentTools/Geometry.h"

#include <filesystem>
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

using namespace primal;

struct camera_surface {
    game_entity::entity			entity{};
    graphics::camera			camera{};
    graphics::render_surface	surface{};
};

camera_surface				_surfaces[4];
time_it						timer;
bool resized{ false };

game_entity::entity create_one_game_entity(math::v3 position, math::v3 rotation, geometry::init_info* geometry, const char* script_name);
void remove_game_entity(game_entity::entity_id id);
bool read_file(std::filesystem::path, std::unique_ptr<u8[]>&, u64&);
void generate_lights();
void remove_lights();
void test_lights(f32 dt);
namespace {
id::id_type house_model_id{ id::invalid_id };
id::id_type plane_model_id{ id::invalid_id };
id::id_type robot_model_id{ id::invalid_id };
id::id_type sphere_model_id{ id::invalid_id };

game_entity::entity_id house_entity_id{ id::invalid_id };
game_entity::entity_id plane_entity_id{ id::invalid_id };
game_entity::entity_id robot_entity_id{ id::invalid_id };
game_entity::entity_id sphere_entity_ids[12]{};

struct texture_usage {
    enum usage : u32 {
        ambient_occlusion = 0,
        base_color,
        emissive,
        metal_rough,
        normal,

        count
    };
};

id::id_type texture_ids[texture_usage::count];

id::id_type vs_id{ id::invalid_id };
id::id_type ps_id{ id::invalid_id };
id::id_type textured_ps_id{ id::invalid_id };
id::id_type default_mtl_id{ id::invalid_id };
id::id_type robot_mtl_id{ id::invalid_id };

id::id_type pbr_mtl_ids[12]{};

utl::vector<id::id_type> render_item_id_cache;

std::mutex						mutex{};

constexpr u32 num_items{ 10 };

std::unordered_map<id::id_type, game_entity::entity_id> render_item_entity_map;

_NODISCARD id::id_type
load_asset(const char* path, content::asset_type::type type)
{
    std::unique_ptr<u8[]> buffer;
    u64 size{ 0 };
    read_file(path, buffer, size);

    const id::id_type asset_id{ content::create_resource(buffer.get(), type) };
    assert(id::is_valid(asset_id));
    return asset_id;
}

_NODISCARD id::id_type
load_model(const char* path)
{
    return load_asset(path, content::asset_type::mesh);
}

_NODISCARD id::id_type
load_texture(const char* path)
{
    return load_asset(path, content::asset_type::texture);
}

void
compile_shaders_vs()
{
    ID3DBlob* shader_blob{ nullptr };
    ID3DBlob* error_blob{ nullptr };

    UINT flags{ 0 };
#if _DEBUG
    flags |= D3DCOMPILE_DEBUG;
    flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

    std::string defines[]{ "1", "3" };
    utl::vector<u32> keys;
    keys.emplace_back(tools::elements::elements_type::static_normal);
    keys.emplace_back(tools::elements::elements_type::static_normal_texture);

    utl::vector<std::wstring> extra_args{};
    utl::vector<std::unique_ptr<u8[]>> vertex_shaders{};
    utl::vector<const u8*> vertex_shader_pointers{};

    for (u32 i{ 0 }; i < _countof(defines); ++i)
    {
        D3D_SHADER_MACRO define[2]{};//Last one must be NULL
        define[0].Name = "ELEMENTS_TYPE";
        define[0].Definition = defines[i].c_str();

        HRESULT hr = D3DCompileFromFile(L"..\\..\\Engine\\Graphics\\Direct3D11\\Shaders\\TestShader.hlsl", &define[0],
            D3D_COMPILE_STANDARD_FILE_INCLUDE, "TestShaderVS", "vs_5_0", flags, 0, &shader_blob, &error_blob);
        if (error_blob || FAILED(hr))
        {
            OutputDebugStringA((char*)error_blob->GetBufferPointer());
            return;
        }
        char hash[16]{ 0x092386 * i };//Random number as temporary hash

        const u64 buffer_size{ sizeof(u64) + content::compiled_shader::hash_length + shader_blob->GetBufferSize() };
        std::unique_ptr<u8[]> buffer{ std::make_unique<u8[]>(buffer_size) };
        utl::blob_stream_writer blob{ buffer.get(), buffer_size };
        blob.write(shader_blob->GetBufferSize());
        blob.write(hash, content::compiled_shader::hash_length);
        blob.write((u8*)shader_blob->GetBufferPointer(), shader_blob->GetBufferSize());

        assert(buffer_size == blob.offset());

        vertex_shaders.emplace_back(std::move(buffer));
        vertex_shader_pointers.emplace_back(vertex_shaders.back().get());

        shader_blob->Release();
    }

    vs_id = content::add_shader_group(vertex_shader_pointers.data(), (u32)vertex_shader_pointers.size(), keys.data());
}

void compile_shaders_ps()
{
    ID3DBlob* shader_blob{ nullptr };
    ID3DBlob* error_blob{ nullptr };
  
    //Must be used so that the Elements array doesn't contain empty structs... Though it really isn't used
    D3D_SHADER_MACRO define[2]{};//Last one must be NULL
    define[0].Name = "ELEMENTS_TYPE";
    define[0].Definition = "1";

    UINT flags{ 0 };
#if _DEBUG
    flags |= D3DCOMPILE_DEBUG;
    flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

    HRESULT hr = D3DCompileFromFile(L"..\\..\\Engine\\Graphics\\Direct3D11\\Shaders\\TestShader.hlsl", &define[0],
        D3D_COMPILE_STANDARD_FILE_INCLUDE, "TestShaderPS", "ps_5_0", flags, 0, &shader_blob, &error_blob);

    if (error_blob || FAILED(hr))
    {
        OutputDebugStringA((char*)error_blob->GetBufferPointer());
        return;
    }
    char hash[16]{ 0x01 };

    const u64 buffer_size{ sizeof(u64) + content::compiled_shader::hash_length + shader_blob->GetBufferSize() };
    std::unique_ptr<u8[]> buffer{ std::make_unique<u8[]>(buffer_size) };
    utl::blob_stream_writer blob{ buffer.get(), buffer_size };
    blob.write(shader_blob->GetBufferSize());
    blob.write(hash, content::compiled_shader::hash_length);
    blob.write((u8*)shader_blob->GetBufferPointer(), shader_blob->GetBufferSize());

    assert(buffer_size == blob.offset());

    u8* b[]{ buffer.get() };

    ps_id = content::add_shader_group(b, 1, &u32_invalid_id);
}

//My code is getting worse every time
void compile_shaders_textured_ps()
{
    ID3DBlob* shader_blob{ nullptr };
    ID3DBlob* error_blob{ nullptr };

    //Must be used so that the Elements array doesn't contain empty structs... Though it really isn't used
    D3D_SHADER_MACRO define[3]{};//Last one must be NULL
    define[0].Name = "ELEMENTS_TYPE";
    define[0].Definition = "1";
    define[1].Name = "TEXTURED_MTL";
    define[1].Definition = "1";

    UINT flags{ 0 };
#if _DEBUG
    flags |= D3DCOMPILE_DEBUG;
    flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

    HRESULT hr = D3DCompileFromFile(L"..\\..\\Engine\\Graphics\\Direct3D11\\Shaders\\TestShader.hlsl", &define[0],
        D3D_COMPILE_STANDARD_FILE_INCLUDE, "TestShaderPS", "ps_5_0", flags, 0, &shader_blob, &error_blob);
    if (error_blob || FAILED(hr))
    {
        OutputDebugStringA((char*)error_blob->GetBufferPointer());
        return;
    }
    char hash[16]{ 0x02 };

    const u64 buffer_size{ sizeof(u64) + content::compiled_shader::hash_length + shader_blob->GetBufferSize() };
    std::unique_ptr<u8[]> buffer{ std::make_unique<u8[]>(buffer_size) };
    utl::blob_stream_writer blob{ buffer.get(), buffer_size };
    blob.write(shader_blob->GetBufferSize());
    blob.write(hash, content::compiled_shader::hash_length);
    blob.write((u8*)shader_blob->GetBufferPointer(), shader_blob->GetBufferSize());

    assert(buffer_size == blob.offset());

    u8* b[]{ buffer.get() };

    textured_ps_id = content::add_shader_group(b, 1, &u32_invalid_id);
}

void
load_shaders()
{
    compile_shaders_vs();
    compile_shaders_ps();
    compile_shaders_textured_ps();
}

void
create_material()
{
    assert(id::is_valid(vs_id) && id::is_valid(ps_id) && id::is_valid(textured_ps_id));

    graphics::material_init_info info{};
    info.shader_ids[graphics::shader_type::vertex] = vs_id;
    info.shader_ids[graphics::shader_type::pixel] = ps_id;
    info.type = graphics::material_type::opaque;
    default_mtl_id = content::create_resource(&info, content::asset_type::material);

    memset(pbr_mtl_ids, 0xff, sizeof(pbr_mtl_ids));

    math::v2 metal_rough[_countof(pbr_mtl_ids)]
    {
        { 0.f, 0.f }, { 0.f, 0.2f }, { 0.f, 0.4f }, { 0.f, 0.6f }, { 0.f, 0.8f }, { 0.f, 1.f},
        { 1.f, 0.f }, { 1.f, 0.2f }, { 1.f, 0.4f }, { 1.f, 0.6f }, { 1.f, 0.8f }, { 1.f, 1.f},
    };

    graphics::material_surface& s{ info.surface };
    s.base_color = { 0.5f, 0.5f, 0.5f, 1.f };

    for (u32 i{ 0 }; i < _countof(pbr_mtl_ids); ++i)
    {
        s.metallic = metal_rough[i].x;
        s.roughness = metal_rough[i].y;
        pbr_mtl_ids[i] = content::create_resource(&info, content::asset_type::material);
    }

    info.shader_ids[graphics::shader_type::pixel] = textured_ps_id;
    info.texture_count = texture_usage::count;
    info.texture_ids = &texture_ids[0];

    robot_mtl_id = content::create_resource(&info, content::asset_type::material);
}

void
remove_item(game_entity::entity_id entity_id, id::id_type item_id, id::id_type model_id)
{
    if (id::is_valid(item_id))
    {
        graphics::remove_render_item(item_id);
        auto pair = render_item_entity_map.find(item_id);
        if (pair != render_item_entity_map.end())
        {
            remove_game_entity(pair->second);
        }

        if (id::is_valid(model_id))
        {
            content::destroy_resource(model_id, content::asset_type::mesh);
        }
    }
}

void
remove_model(id::id_type model_id)
{
    if (id::is_valid(model_id))
    {
        content::destroy_resource(model_id, content::asset_type::mesh);
    }
}

void
destroy_render_items()
{
    remove_game_entity(house_entity_id);
    remove_game_entity(plane_entity_id);
    remove_game_entity(robot_entity_id);

    for (u32 i{ 0 }; i < _countof(sphere_entity_ids); ++i)
    {
        remove_game_entity(sphere_entity_ids[i]);
    }

    remove_model(house_model_id);
    remove_model(plane_model_id);
    remove_model(robot_model_id);
    remove_model(sphere_model_id);

    if (id::is_valid(default_mtl_id))
    {
        content::destroy_resource(default_mtl_id, content::asset_type::material);
    }

    if (id::is_valid(robot_mtl_id))
    {
        content::destroy_resource(robot_mtl_id, content::asset_type::material);
    }

    for (id::id_type id : pbr_mtl_ids)
    {
        if (id::is_valid(id))
        {
            content::destroy_resource(id, content::asset_type::material);
        }
    }

    for (id::id_type id : texture_ids)
    {
        if (id::is_valid(id))
        {
            content::destroy_resource(id, content::asset_type::texture);
        }
    }

    if (id::is_valid(vs_id))
    {
        content::remove_shader_group(vs_id);
    }

    if (id::is_valid(ps_id))
    {
        content::remove_shader_group(ps_id);
    }

    if (id::is_valid(textured_ps_id))
    {
        content::remove_shader_group(textured_ps_id);
    }
}

void
create_camera_surface(camera_surface& surface, platform::window_init_info info)
{
    surface.surface.window = platform::create_window(&info);
    surface.surface.surface = graphics::create_surface(surface.surface.window);
    surface.entity = create_one_game_entity({ 11.f, 1.f, 0.f }, { 0.f, -3.14f / 2.f, 0.f }, nullptr, "camera_script");
    surface.camera = graphics::create_camera(graphics::perspective_camera_init_info(surface.entity.get_id()));
    surface.camera.aspect_ratio((f32)surface.surface.window.width() / surface.surface.window.height());
}

void
destroy_camera_surface(camera_surface& surface)
{
    camera_surface temp{ surface };
    surface = {};
    if (temp.surface.surface.is_valid()) graphics::remove_surface(temp.surface.surface.get_id());
    if (temp.surface.window.is_valid()) platform::remove_window(temp.surface.window.get_id());
    if (temp.camera.is_valid()) graphics::remove_camera(temp.camera.get_id());
    if (temp.entity.is_valid()) game_entity::remove(temp.entity.get_id());
}
}//anonymous namespace

LRESULT win_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    bool toggle_fullscreen{ false };

    switch (msg)
    {
    case WM_DESTROY:
    {
        bool all_closed{ true };
        for (u32 i{ 0 }; i < _countof(_surfaces); ++i)
        {
            if (_surfaces[i].surface.window.is_valid())
            {
                if (_surfaces[i].surface.window.is_closed())
                {
                    destroy_camera_surface(_surfaces[i]);
                }
                else
                {
                    all_closed = false;
                }
            }
        }
        if (all_closed)
        {
            PostQuitMessage(0);
            return 0;
        }
        break;
    }
    case WM_KEYDOWN:
        if (wparam == VK_ESCAPE)
        {
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            return 0;
        }
        break;
    case WM_SIZE:
        resized = (wparam != SIZE_MINIMIZED);
        break;
    case WM_SYSCHAR:
        toggle_fullscreen = (wparam == VK_RETURN && (HIWORD(lparam) & KF_ALTDOWN));
        break;
    }

    if ((resized && GetKeyState(VK_LBUTTON) >= 0) || toggle_fullscreen)
    {
        platform::window win{ platform::window_id{(id::id_type)GetWindowLongPtr(hwnd, GWLP_USERDATA)} };
        for (u32 i{ 0 }; i < _countof(_surfaces); ++i)
        {
            if (win.get_id() == _surfaces[i].surface.window.get_id())
            {
                if (toggle_fullscreen)
                {
                    win.set_fullscreen(!win.is_fullscreen());
                    return 0;
                }
                else
                {
                    _surfaces[i].surface.surface.resize(win.width(), win.height());
                    _surfaces[i].camera.aspect_ratio((f32)win.width() / win.height());
                    resized = false;
                }
                break;
            }
        }
        Sleep(10);
    }

    return DefWindowProc(hwnd, msg, wparam, lparam);
}

class engine_test : public test
{
public:
    bool initialize() override
    {
        if(!graphics::initialize(graphics::graphics_platform::direct3d11))
            return false;
        {
            platform::window_init_info info[]
            {
                {&win_proc, nullptr, L"Window 1", 100, 100, 200, 400},
                {&win_proc, nullptr, L"Window 2", 150, 150, 400, 200},
                {&win_proc, nullptr, L"Window 3", 200, 200, 200, 200},
                {&win_proc, nullptr, L"Window 4", 250, 250, 400, 300},
            };
            static_assert(_countof(_surfaces) == _countof(info));

            for (u32 i{ 0 }; i < _countof(_surfaces); ++i)
            {
                create_camera_surface(_surfaces[i], info[i]);
            }
        }
        {
            input::input_source source{};
            source.binding = std::hash<std::string>()("move");
            source.source_type = input::input_source::keyboard;

            source.code = input::input_code::key_a;
            source.multiplier = 1.f;
            source.axis = input::axis::x;
            input::bind(source);

            source.code = input::input_code::key_d;
            source.multiplier = -1.f;
            input::bind(source);

            source.code = input::input_code::key_w;
            source.multiplier = 1.f;
            source.axis = input::axis::z;
            input::bind(source);

            source.code = input::input_code::key_s;
            source.multiplier = -1.f;
            input::bind(source);

            source.code = input::input_code::key_e;
            source.multiplier = 1.f;
            source.axis = input::axis::y;
            input::bind(source);

            source.code = input::input_code::key_q;
            source.multiplier = -1.f;
            input::bind(source);
        }

        generate_lights();

        {
            assert(std::filesystem::exists("..\\..\\x64\\house_model.model"));
            assert(std::filesystem::exists("..\\..\\x64\\wood_model.model"));
            assert(std::filesystem::exists("..\\..\\x64\\robot_model.model"));
            assert(std::filesystem::exists("..\\..\\x64\\ao.texture"));

            memset(&texture_ids[0], 0xff, sizeof(id::id_type) * _countof(texture_ids));

            std::thread threads[]{
                std::thread{ [] { texture_ids[texture_usage::ambient_occlusion] = load_texture("..\\..\\x64\\ao.texture"); }},
                std::thread{ [] { texture_ids[texture_usage::base_color] = load_texture("..\\..\\x64\\albedo.texture"); }},
                std::thread{ [] { texture_ids[texture_usage::emissive] = load_texture("..\\..\\x64\\emissive.texture"); }},
                std::thread{ [] { texture_ids[texture_usage::metal_rough] = load_texture("..\\..\\x64\\metalrough.texture"); }},
                std::thread{ [] { texture_ids[texture_usage::normal] = load_texture("..\\..\\x64\\normal.texture"); }},

                std::thread{ [] { house_model_id = load_model("..\\..\\x64\\house_model.model"); } },
                std::thread{ [] { plane_model_id = load_model("..\\..\\x64\\wood_model.model"); } },
                std::thread{ [] { robot_model_id = load_model("..\\..\\x64\\robot_model.model"); } },
                std::thread{ [] { sphere_model_id = load_model("..\\..\\x64\\sphere_model.model"); } },
                std::thread{ [] { load_shaders(); } },
            };

            for (auto& t : threads) t.join();

            create_material();
            id::id_type materials[]{ default_mtl_id };
            id::id_type robot_materials[]{ robot_mtl_id };

            geometry::init_info geometry_info{};
            geometry_info.material_count = _countof(materials);
            geometry_info.material_ids = &materials[0];

            geometry_info.geometry_content_id = house_model_id;
            house_entity_id = create_one_game_entity({}, {}, &geometry_info, nullptr).get_id();

            geometry_info.geometry_content_id = plane_model_id;
            plane_entity_id = create_one_game_entity({ 0.f, 10.f, 0.f }, {}, &geometry_info, "rotator_script").get_id();

            geometry_info.geometry_content_id = robot_model_id;
            geometry_info.material_count = _countof(robot_materials);
            geometry_info.material_ids = &robot_materials[0];

            robot_entity_id = create_one_game_entity({ 0.f, 0.1f, -1.f }, { 0.f, math::half_pi, 0.f }, &geometry_info, nullptr).get_id();

            geometry_info.geometry_content_id = sphere_model_id;
            geometry_info.material_count = 1;

            for (u32 i{ 0 }; i < _countof(sphere_entity_ids); ++i)
            {
                id::id_type id{ pbr_mtl_ids[i] };
                id::id_type sphere_mtls[]{ id };
                geometry_info.material_ids = &sphere_mtls[0];
                const f32 x{ (-6.f + i % 6) * 1.5f };
                const f32 y{ (i < 6) ? 8.f : 5.5f };
                const f32 z{ x };
                sphere_entity_ids[i] = create_one_game_entity({ x, y, z }, {}, &geometry_info, nullptr).get_id();
            }
        }

        render_item_id_cache.resize(3 + 12);
        geometry::get_render_item_ids(render_item_id_cache.data(), (u32)render_item_id_cache.size());

        return true;
    }

    void run() override
    {
        timer.begin();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        const f32 dt_avg{ timer.dt_avg() };
        script::update(dt_avg);
        //test_lights(dt_avg);

        for (u32 i{ 0 }; i < _countof(_surfaces); ++i)
        {
            if (_surfaces[i].surface.surface.is_valid())
            {
                f32 thresholds[3 + 12]{};

                graphics::frame_info info{};
                info.render_item_ids = render_item_id_cache.data();
                info.render_item_count = 3 + 12;
                info.light_set_key = left_set;
                info.average_frame_time = timer.dt_avg();
                info.thresholds = &thresholds[0];
                info.camera_id = _surfaces[i].camera.get_id();

                _surfaces[i].surface.surface.render(info);
            }
        }

        timer.end();
    }

    void shutdown() override
    {
        destroy_render_items();
        remove_lights();

        for (u32 i{ 0 }; i < _countof(_surfaces); ++i)
            destroy_camera_surface(_surfaces[i]);

        graphics::shutdown();
    }

private:
    constexpr math::v3 rgb_to_color(u8 r, u8 g, u8 b) { return{ r / 255.f, g / 255.f, b / 255.f }; }

    f32 random(f32 min = 0.f) { return std::max(min, rand() * inv_rand_max); }

    const f32 inv_rand_max{ 1.f / RAND_MAX };

    utl::vector<graphics::light> lights;
    const u64 left_set{ 0 };
    const u64 right_set{ 1 };
};
