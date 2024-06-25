#pragma once
#include "Test.h"
#include "Components/Entity.h"
#include "Components/Script.h"
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

game_entity::entity create_one_game_entity(math::v3 position, math::v3 rotation, const char* script_name);
void remove_game_entity(game_entity::entity_id id);
bool read_file(std::filesystem::path, std::unique_ptr<u8[]>&, u64&);
void generate_lights();
void remove_lights();
void test_lights(f32 dt);
namespace {
id::id_type house_model_id{ id::invalid_id };
id::id_type plane_model_id{ id::invalid_id };
id::id_type robot_model_id{ id::invalid_id };

id::id_type house_item_id{ id::invalid_id };
id::id_type plane_item_id{ id::invalid_id };
id::id_type robot_item_id{ id::invalid_id };

game_entity::entity_id house_entity_id{ id::invalid_id };
game_entity::entity_id plane_entity_id{ id::invalid_id };
game_entity::entity_id robot_entity_id{ id::invalid_id };

id::id_type vs_id{ id::invalid_id };
id::id_type ps_id{ id::invalid_id };
id::id_type mtl_id{ id::invalid_id };

utl::vector<game_entity::entity_id>			entity_ids;
utl::vector<id::id_type>					item_ids;

std::mutex						mutex{};

constexpr u32 num_items{ 10 };

std::unordered_map<id::id_type, game_entity::entity_id> render_item_entity_map;

_NODISCARD id::id_type
load_model(const char* path)
{
    std::unique_ptr<u8[]> model;
    u64 size{ 0 };
    read_file(path, model, size);

    const id::id_type model_id{ content::create_resource(model.get(), content::asset_type::mesh) };
    assert(id::is_valid(model_id));
    return model_id;
}

void compile_shaders_vs()
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
            D3D_COMPILE_STANDARD_FILE_INCLUDE, "TestShaderVS", "vs_5_0", 0, 0, &shader_blob, &error_blob);
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

    utl::vector<u32> keys;
    keys.emplace_back(tools::elements::elements_type::static_normal_texture);

    //Must be used so that the Elements array doesn't contain empty structs... Though it really isn't used
    D3D_SHADER_MACRO define[2]{};//Last one must be NULL
    define[0].Name = "ELEMENTS_TYPE";
    define[0].Definition = "1";

    HRESULT hr = D3DCompileFromFile(L"..\\..\\Engine\\Graphics\\Direct3D11\\Shaders\\TestShader.hlsl", &define[0],
        D3D_COMPILE_STANDARD_FILE_INCLUDE, "TestShaderPS", "ps_5_0", 0, 0, &shader_blob, &error_blob);
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

void
load_shaders()
{
    compile_shaders_vs();
    compile_shaders_ps();
}

void
create_material()
{
    assert(id::is_valid(vs_id) && id::is_valid(ps_id));
    graphics::material_init_info info{};
    info.shader_ids[graphics::shader_type::vertex] = vs_id;
    info.shader_ids[graphics::shader_type::pixel] = ps_id;
    info.type = graphics::material_type::opaque;
    mtl_id = content::create_resource(&info, content::asset_type::material);
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

f32
random_pos()
{
    return (f32)(rand() % 100) - 50.f;
}


void
remove_items()
{
    for (u32 i{ 0 }; i < item_ids.size(); ++i)
    {
        id::id_type item{ item_ids[i] };

        if (id::is_valid(item))
        {
            graphics::remove_render_item(item);
            auto pair = render_item_entity_map.find(item);
            if (pair != render_item_entity_map.end())
            {
                remove_game_entity(pair->second);
            }
        }
    }
}

void
get_render_items(id::id_type* items, [[maybe_unused]] u32 count)
{
    items[0] = house_item_id;
    items[1] = plane_item_id;
    items[2] = robot_item_id;
}

void
create_camera_surface(camera_surface& surface, platform::window_init_info info)
{
    surface.surface.window = platform::create_window(&info);
    surface.surface.surface = graphics::create_surface(surface.surface.window);
    surface.entity = create_one_game_entity({ 11.f, 1.f, 0.f }, { 0.f, -3.14f / 2.f, 0.f }, "camera_script");
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
            auto _1 = std::thread{ [] { house_model_id = load_model("..\\..\\x64\\house_model.model"); } };
            auto _2 = std::thread{ [] { plane_model_id = load_model("..\\..\\x64\\wood_model.model"); } };
            auto _3 = std::thread{ [] { robot_model_id = load_model("..\\..\\x64\\robot_model.model"); } };
            auto _4 = std::thread{ [] { load_shaders(); } };

            house_entity_id = create_one_game_entity({}, {}, nullptr).get_id();
            plane_entity_id = create_one_game_entity({ 0.f, 10.f, 0.f }, {}, "rotator_script").get_id();
            robot_entity_id = create_one_game_entity({ 0.f, 0.1f, 1.f }, {}, "rotator_script").get_id();

            _1.join();
            _2.join();
            _3.join();
            _4.join();

            create_material();
            id::id_type materials[]{ mtl_id };

            house_item_id = { graphics::add_render_item(house_entity_id, house_model_id, _countof(materials), &materials[0]) };
            plane_item_id = { graphics::add_render_item(plane_entity_id, plane_model_id, _countof(materials), &materials[0]) };
            robot_item_id = { graphics::add_render_item(robot_entity_id, robot_model_id, _countof(materials), &materials[0]) };

            render_item_entity_map[house_item_id] = house_entity_id;
            render_item_entity_map[plane_item_id] = plane_entity_id;
            render_item_entity_map[robot_item_id] = robot_entity_id;
        }

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
                f32 thresholds[3]{ 0.f };

                id::id_type render_items[3]{};
                get_render_items(&render_items[0], 3);

                graphics::frame_info info{};
                info.render_item_ids = &render_items[0];
                info.render_item_count = 3;
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
        remove_items();

        {
            remove_item(house_entity_id, house_item_id, house_model_id);
            remove_item(plane_entity_id, plane_item_id, plane_model_id);
            remove_item(robot_entity_id, robot_item_id, robot_model_id);

            if (id::is_valid(mtl_id))
            {
                content::destroy_resource(mtl_id, content::asset_type::material);
            }
            if (id::is_valid(vs_id))
            {
                content::remove_shader_group(vs_id);
            }
            if (id::is_valid(ps_id))
            {
                content::remove_shader_group(ps_id);
            }
        }

        remove_lights();

        for (u32 i{ 0 }; i < _countof(_surfaces); ++i)
            destroy_camera_surface(_surfaces[i]);

        graphics::shutdown();
    }

private:
    void create_material()
    {
        assert(id::is_valid(vs_id) && id::is_valid(ps_id));
        graphics::material_init_info info{};
        info.shader_ids[graphics::shader_type::vertex] = vs_id;
        info.shader_ids[graphics::shader_type::pixel] = ps_id;
        info.type = graphics::material_type::opaque;
        mtl_id = content::create_resource(&info, content::asset_type::material);
    }

    constexpr math::v3 rgb_to_color(u8 r, u8 g, u8 b) { return{ r / 255.f, g / 255.f, b / 255.f }; }

    f32 random(f32 min = 0.f) { return std::max(min, rand() * inv_rand_max); }

    const f32 inv_rand_max{ 1.f / RAND_MAX };

    utl::vector<graphics::light> lights;
    const u64 left_set{ 0 };
    const u64 right_set{ 1 };
};
