/*
 * CrabeLoader manual D3D11 depth event bridge.
 * Native COM objects are observed, never replaced by proxy objects.
 */
#pragma once
#include "hook_manager.hpp"
#include "d3d11/d3d11_impl_type_convert.hpp"
#include <memory>
#include <array>
#include <mutex>
#include <unordered_map>

namespace crabe_manual_depth {
using namespace reshade;
static std::recursive_mutex mutex;
static thread_local unsigned int paused = 0;
struct pause_scope { pause_scope() { ++paused; } ~pause_scope() { --paused; } };
struct tracked_resource { com_ptr<ID3D11Resource> object; uint64_t last_frame; };
struct session {
    d3d11::device_impl *device;
    d3d11::device_context_impl *immediate;
    uint64_t frame = 0;
    std::unordered_map<ID3D11DeviceContext *, std::unique_ptr<d3d11::device_context_impl>> deferred;
    std::unordered_map<ID3D11CommandList *, std::unique_ptr<d3d11::command_list_impl>> lists;
    std::unordered_map<ID3D11Resource *, tracked_resource> resources;
};
static std::unique_ptr<session> active;

template <size_t index, typename T, typename Object>
static T original(Object *object, T replacement) {
    return hooks::call(replacement);
}
static d3d11::device_context_impl *context_for(ID3D11DeviceContext *context) {
    if (!active) return nullptr;
    if (context == active->immediate->_orig) return active->immediate;
    com_ptr<ID3D11Device> device;
    context->GetDevice(&device);
    if (device.get() != active->device->_orig || context->GetType() != D3D11_DEVICE_CONTEXT_DEFERRED) return nullptr;
    auto &entry = active->deferred[context];
    if (!entry) {
        entry = std::make_unique<d3d11::device_context_impl>(active->device, context);
        invoke_addon_event<addon_event::init_command_list>(static_cast<api::command_list *>(entry.get()));
    }
    return entry.get();
}
static void observe(ID3D11DepthStencilView *view) {
    if (!view) return;
    com_ptr<ID3D11Resource> resource;
    view->GetResource(&resource);
    auto it = active->resources.find(resource.get());
    if (it == active->resources.end()) {
        const api::resource handle = {reinterpret_cast<uint64_t>(resource.get())};
        const auto desc = active->device->get_resource_desc(handle);
        active->resources.emplace(resource.get(), tracked_resource{resource, active->frame});
        invoke_addon_event<addon_event::init_resource>(active->device, desc, nullptr,
            api::resource_usage::depth_stencil, handle);
        log::message(log::level::info, "Crabe depth: observed %ux%u depth resource (samples=%u).",
            desc.texture.width, desc.texture.height, desc.texture.samples);
    } else it->second.last_frame = active->frame;
}
static void sync_binding(ID3D11DeviceContext *native, api::command_list *context) {
    com_ptr<ID3D11DepthStencilView> view;
    native->OMGetRenderTargets(0, nullptr, &view);
    observe(view.get());
    invoke_addon_event<addon_event::bind_render_targets_and_depth_stencil>(context, 0, nullptr,
        api::resource_view{reinterpret_cast<uint64_t>(view.get())});
    D3D11_VIEWPORT viewport{};
    UINT count = 1;
    native->RSGetViewports(&count, &viewport);
    if (count) invoke_addon_event<addon_event::bind_viewports>(context, 0, 1,
        reinterpret_cast<const api::viewport *>(&viewport));
}
template <typename Action>
static void dispatch(ID3D11DeviceContext *native, Action action, bool bindings = true) {
    if (paused) return;
    const std::lock_guard<std::recursive_mutex> lock(mutex);
    pause_scope guard;
    if (auto context = context_for(native)) {
        if (bindings) sync_binding(native, context);
        action(static_cast<api::command_list *>(context));
    }
}
static void refresh(ID3D11DeviceContext *context);
template <unsigned int slot> static void STDMETHODCALLTYPE draw(ID3D11DeviceContext *c, UINT count, UINT start) {
    dispatch(c, [&](api::command_list *list) { invoke_addon_event<addon_event::draw>(list, count, 1, start, 0); });
    { pause_scope native_call; original<13>(c, draw<slot>)(c, count, start); }
    refresh(c);
}
template <unsigned int slot> static void STDMETHODCALLTYPE draw_indexed(ID3D11DeviceContext *c, UINT count, UINT start, INT base) {
    dispatch(c, [&](api::command_list *list) { invoke_addon_event<addon_event::draw_indexed>(list, count, 1, start, base, 0); });
    { pause_scope native_call; original<12>(c, draw_indexed<slot>)(c, count, start, base); }
    refresh(c);
}
template <unsigned int slot> static void STDMETHODCALLTYPE draw_instanced(ID3D11DeviceContext *c, UINT count, UINT instances, UINT start, UINT first) {
    dispatch(c, [&](api::command_list *list) { invoke_addon_event<addon_event::draw>(list, count, instances, start, first); });
    { pause_scope native_call; original<21>(c, draw_instanced<slot>)(c, count, instances, start, first); }
    refresh(c);
}
template <unsigned int slot> static void STDMETHODCALLTYPE draw_indexed_instanced(ID3D11DeviceContext *c, UINT count, UINT instances, UINT start, INT base, UINT first) {
    dispatch(c, [&](api::command_list *list) { invoke_addon_event<addon_event::draw_indexed>(list, count, instances, start, base, first); });
    { pause_scope native_call; original<20>(c, draw_indexed_instanced<slot>)(c, count, instances, start, base, first); }
    refresh(c);
}
static void indirect(ID3D11DeviceContext *c, ID3D11Buffer *buffer, UINT offset, api::indirect_command type) {
    dispatch(c, [&](api::command_list *list) { invoke_addon_event<addon_event::draw_or_dispatch_indirect>(list,
        type, api::resource{reinterpret_cast<uint64_t>(buffer)}, static_cast<uint64_t>(offset), 1, 0); });
}
template <unsigned int slot> static void STDMETHODCALLTYPE draw_auto(ID3D11DeviceContext *c) {
    indirect(c, nullptr, 0, api::indirect_command::draw);
    { pause_scope native_call; original<38>(c, draw_auto<slot>)(c); }
    refresh(c);
}
template <unsigned int slot> static void STDMETHODCALLTYPE draw_indexed_indirect(ID3D11DeviceContext *c, ID3D11Buffer *buffer, UINT offset) {
    indirect(c, buffer, offset, api::indirect_command::draw_indexed);
    { pause_scope native_call; original<39>(c, draw_indexed_indirect<slot>)(c, buffer, offset); }
    refresh(c);
}
template <unsigned int slot> static void STDMETHODCALLTYPE draw_indirect(ID3D11DeviceContext *c, ID3D11Buffer *buffer, UINT offset) {
    indirect(c, buffer, offset, api::indirect_command::draw);
    { pause_scope native_call; original<40>(c, draw_indirect<slot>)(c, buffer, offset); }
    refresh(c);
}
template <unsigned int slot> static void STDMETHODCALLTYPE clear_depth(ID3D11DeviceContext *c, ID3D11DepthStencilView *view, UINT flags, FLOAT depth, UINT8 stencil) {
    dispatch(c, [&](api::command_list *list) {
        observe(view);
        invoke_addon_event<addon_event::clear_depth_stencil_view>(list,
            api::resource_view{reinterpret_cast<uint64_t>(view)}, (flags & D3D11_CLEAR_DEPTH) ? &depth : nullptr,
            (flags & D3D11_CLEAR_STENCIL) ? &stencil : nullptr, 0, nullptr);
    }, false);
    { pause_scope native_call; original<53>(c, clear_depth<slot>)(c, view, flags, depth, stencil); }
    refresh(c);
}
static ULONG STDMETHODCALLTYPE release_list(ID3D11CommandList *list) {
    const std::lock_guard<std::recursive_mutex> lock(mutex);
    const ULONG count = hooks::call(release_list, &hooks::vtable_from_instance(list)[2])(list);
    if (!count && active) {
        auto it = active->lists.find(list);
        if (it != active->lists.end()) {
            pause_scope guard;
            invoke_addon_event<addon_event::destroy_command_list>(it->second.get());
            active->lists.erase(it);
        }
    }
    return count;
}
template <unsigned int slot> static HRESULT STDMETHODCALLTYPE finish(ID3D11DeviceContext *c, BOOL restore, ID3D11CommandList **out) {
    HRESULT hr;
    { pause_scope native_call; hr = original<114>(c, finish<slot>)(c, restore, out); }
    if (SUCCEEDED(hr) && out && *out) dispatch(c, [&](api::command_list *context) {
        auto list = std::make_unique<d3d11::command_list_impl>(active->device, *out);
        invoke_addon_event<addon_event::init_command_list>(list.get());
        invoke_addon_event<addon_event::execute_secondary_command_list>(list.get(), context);
        active->lists[*out] = std::move(list);
        hooks::install("Crabe depth CommandList::Release", hooks::vtable_from_instance(*out), 2,
            reinterpret_cast<hook::address>(release_list));
        invoke_addon_event<addon_event::reset_command_list>(context);
    }, false);
    return hr;
}
template <unsigned int slot> static void STDMETHODCALLTYPE execute(ID3D11DeviceContext *c, ID3D11CommandList *native, BOOL restore) {
    dispatch(c, [&](api::command_list *context) {
        const auto it = active->lists.find(native);
        if (it != active->lists.end()) invoke_addon_event<addon_event::execute_secondary_command_list>(context, it->second.get());
    }, false);
    { pause_scope native_call; original<58>(c, execute<slot>)(c, native, restore); }
    refresh(c);
}
struct table_state { std::array<hook::address, 10> methods; bool ready; };
static std::unordered_map<hook::address *, table_state> installed_tables;
static bool install(ID3D11DeviceContext *context) {
    auto table = hooks::vtable_from_instance(context);
    const std::array<size_t, 10> indices{12,13,20,21,38,39,40,53,58,114};
    std::array<hook::address, 10> methods{};
    for (size_t i=0; i<indices.size(); ++i) methods[i]=table[indices[i]];
    const auto previous=installed_tables.find(table);
    if (previous!=installed_tables.end() && previous->second.methods==methods) return previous->second.ready;
    bool ok = true;
    const auto add = [&](size_t index, auto callbacks, const char *name) {
        if (hooks::is_hooked(table[index])) return;
        for (auto callback : callbacks)
            if (hooks::install(name, table[index], reinterpret_cast<hook::address>(callback))) return;
        ok = false;
    };
    add(12, std::array{draw_indexed<0>, draw_indexed<1>, draw_indexed<2>, draw_indexed<3>, draw_indexed<4>, draw_indexed<5>, draw_indexed<6>, draw_indexed<7>}, "Crabe depth draw_indexed");
    add(13, std::array{draw<0>, draw<1>, draw<2>, draw<3>, draw<4>, draw<5>, draw<6>, draw<7>}, "Crabe depth draw");
    add(20, std::array{draw_indexed_instanced<0>, draw_indexed_instanced<1>, draw_indexed_instanced<2>, draw_indexed_instanced<3>, draw_indexed_instanced<4>, draw_indexed_instanced<5>, draw_indexed_instanced<6>, draw_indexed_instanced<7>}, "Crabe depth draw_indexed_instanced");
    add(21, std::array{draw_instanced<0>, draw_instanced<1>, draw_instanced<2>, draw_instanced<3>, draw_instanced<4>, draw_instanced<5>, draw_instanced<6>, draw_instanced<7>}, "Crabe depth draw_instanced");
    add(38, std::array{draw_auto<0>, draw_auto<1>, draw_auto<2>, draw_auto<3>, draw_auto<4>, draw_auto<5>, draw_auto<6>, draw_auto<7>}, "Crabe depth draw_auto");
    add(39, std::array{draw_indexed_indirect<0>, draw_indexed_indirect<1>, draw_indexed_indirect<2>, draw_indexed_indirect<3>, draw_indexed_indirect<4>, draw_indexed_indirect<5>, draw_indexed_indirect<6>, draw_indexed_indirect<7>}, "Crabe depth draw_indexed_indirect");
    add(40, std::array{draw_indirect<0>, draw_indirect<1>, draw_indirect<2>, draw_indirect<3>, draw_indirect<4>, draw_indirect<5>, draw_indirect<6>, draw_indirect<7>}, "Crabe depth draw_indirect");
    add(53, std::array{clear_depth<0>, clear_depth<1>, clear_depth<2>, clear_depth<3>, clear_depth<4>, clear_depth<5>, clear_depth<6>, clear_depth<7>}, "Crabe depth clear_depth");
    add(58, std::array{execute<0>, execute<1>, execute<2>, execute<3>, execute<4>, execute<5>, execute<6>, execute<7>}, "Crabe depth execute");
    add(114, std::array{finish<0>, finish<1>, finish<2>, finish<3>, finish<4>, finish<5>, finish<6>, finish<7>}, "Crabe depth finish");
    installed_tables[table] = table_state{methods,ok};
    if (!ok) log::message(log::level::warning,"Crabe depth: a native method variant could not be hooked; capture may be incomplete.");
    return ok;
}
static void refresh(ID3D11DeviceContext *context) {
    if (paused) return;
    const std::lock_guard<std::recursive_mutex> lock(mutex);
    pause_scope guard;
    if (context_for(context)) install(context);
}
static void attach(d3d11::device_impl *device, d3d11::device_context_impl *context) {
    const std::lock_guard<std::recursive_mutex> lock(mutex);
    pause_scope guard;
    if (active) return;
    active = std::make_unique<session>();
    active->device = device;
    active->immediate = context;
    bool ok = install(context->_orig);
    com_ptr<ID3D11DeviceContext> deferred;
    if (SUCCEEDED(device->_orig->CreateDeferredContext(0, &deferred))) ok = install(deferred.get()) && ok;
    log::message(ok ? log::level::info : log::level::warning,
        "Crabe depth: native draw/clear event forwarding %s (COM identities preserved).", ok ? "enabled" : "partially installed");
}
static void next_frame() {
    const std::lock_guard<std::recursive_mutex> lock(mutex);
    if (!active) return;
    ++active->frame;
    for (auto it = active->resources.begin(); it != active->resources.end();) {
        if (active->frame - it->second.last_frame > 120) {
            invoke_addon_event<addon_event::destroy_resource>(active->device, api::resource{reinterpret_cast<uint64_t>(it->first)});
            it = active->resources.erase(it);
        } else ++it;
    }
}
static void detach() {
    const std::lock_guard<std::recursive_mutex> lock(mutex);
    pause_scope guard;
    if (!active) return;
    for (auto &entry : active->lists) invoke_addon_event<addon_event::destroy_command_list>(entry.second.get());
    for (auto &entry : active->deferred) invoke_addon_event<addon_event::destroy_command_list>(static_cast<api::command_list *>(entry.second.get()));
    for (auto &entry : active->resources) invoke_addon_event<addon_event::destroy_resource>(active->device,
        api::resource{reinterpret_cast<uint64_t>(entry.first)});
    active.reset();
}
}
