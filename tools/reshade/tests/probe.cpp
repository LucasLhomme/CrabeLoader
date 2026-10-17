#include <reshade.hpp>
#include <filesystem>
#include <fstream>

static std::filesystem::path log_path;
static void record(const char *event) { std::ofstream(log_path, std::ios::app) << event << '\n'; }
static void init_device(reshade::api::device *) { record("init_device"); }
static void destroy_device(reshade::api::device *) { record("destroy_device"); }
static void init_runtime(reshade::api::effect_runtime *runtime)
{
    uint64_t value = 0;
    const GUID device_data = {0xe006e162, 0x33ac, 0x4b9f, {0xb1, 0x0f, 0x0e, 0x15, 0x33, 0x5c, 0x7b, 0xdb}};
    runtime->get_device()->get_private_data(reinterpret_cast<const uint8_t *>(&device_data), &value);
    record(value ? "generic_depth_device_ready" : "MISSING generic_depth_device");
    value = 0;
    const GUID runtime_data = {0x7c6363c7, 0xf94e, 0x437a, {0x91, 0x60, 0x14, 0x17, 0x82, 0xc4, 0x4a, 0x98}};
    runtime->get_private_data(reinterpret_cast<const uint8_t *>(&runtime_data), &value);
    record(value ? "generic_depth_runtime_ready" : "MISSING generic_depth_runtime");
    record("init_runtime");
}
static void destroy_runtime(reshade::api::effect_runtime *) { record("destroy_runtime"); }
static bool on_draw(reshade::api::command_list *, uint32_t, uint32_t, uint32_t, uint32_t) { record("native_draw"); return false; }
static void present(reshade::api::effect_runtime *) { record("frame"); }

extern "C" __declspec(dllexport) bool AddonInit(HMODULE self, HMODULE)
{
    wchar_t path[32768]{};
    GetModuleFileNameW(self, path, 32768);
    log_path = std::filesystem::path(path).parent_path() / "probe.log";
    if (!reshade::register_addon(self)) return false;
    record("loaded");
    reshade::register_event<reshade::addon_event::init_device>(init_device);
    reshade::register_event<reshade::addon_event::destroy_device>(destroy_device);
    reshade::register_event<reshade::addon_event::init_effect_runtime>(init_runtime);
    reshade::register_event<reshade::addon_event::destroy_effect_runtime>(destroy_runtime);
    reshade::register_event<reshade::addon_event::reshade_present>(present);
    reshade::register_event<reshade::addon_event::draw>(on_draw);
    return true;
}
extern "C" __declspec(dllexport) void AddonUninit(HMODULE self, HMODULE)
{
    record("unloaded");
    reshade::unregister_addon(self);
}
