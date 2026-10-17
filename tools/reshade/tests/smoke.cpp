#include <reshade.hpp>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <cmath>
#include <cstring>
#include <wrl/client.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

using Microsoft::WRL::ComPtr;
static void require(bool ok, const char *message)
{
    if (!ok) { std::cerr << "FAIL: " << message << " (Windows error " << GetLastError() << ")\n"; ExitProcess(1); }
}

struct Scene {
    ComPtr<ID3D11Texture2D> depth;
    ComPtr<ID3D11DepthStencilView> dsv;
    ComPtr<ID3D11RenderTargetView> rtv;
    ComPtr<ID3D11VertexShader> near_vs, far_vs;
    ComPtr<ID3D11PixelShader> ps;
    ComPtr<ID3D11RasterizerState> raster;
    ComPtr<ID3D11DepthStencilState> state;
    Scene(ID3D11Device *device, IDXGISwapChain *chain) {
        ComPtr<ID3D11Texture2D> buffer;
        require(SUCCEEDED(chain->GetBuffer(0, IID_PPV_ARGS(&buffer))), "get back buffer");
        D3D11_TEXTURE2D_DESC desc{}; buffer->GetDesc(&desc);
        require(SUCCEEDED(device->CreateRenderTargetView(buffer.Get(), nullptr, &rtv)), "create render target");
        desc.Format = DXGI_FORMAT_D32_FLOAT;
        desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        require(SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &depth)), "create typed depth target");
        require(SUCCEEDED(device->CreateDepthStencilView(depth.Get(), nullptr, &dsv)), "create depth view");
        const auto compile = [&](const char *text, const char *entry, const char *profile) {
            ComPtr<ID3DBlob> code, error;
            require(SUCCEEDED(D3DCompile(text, strlen(text), nullptr, nullptr, nullptr, entry, profile,
                0, 0, &code, &error)), "compile scene shader");
            return code;
        };
        const char *shader = "float4 nearVS(uint id:SV_VertexID):SV_Position { id%=3; return float4(id==2?3:-1,id==1?3:-1,0.25,1); } "
            "float4 farVS(uint id:SV_VertexID):SV_Position { float4 p=nearVS(id); p.z=0.75; return p; } "
            "float4 pixel():SV_Target { return float4(0.1,0.2,0.3,1); }";
        auto code = compile(shader, "nearVS", "vs_5_0");
        require(SUCCEEDED(device->CreateVertexShader(code->GetBufferPointer(), code->GetBufferSize(), nullptr, &near_vs)), "create near shader");
        code = compile(shader, "farVS", "vs_5_0");
        require(SUCCEEDED(device->CreateVertexShader(code->GetBufferPointer(), code->GetBufferSize(), nullptr, &far_vs)), "create use_deferred shader");
        code = compile(shader, "pixel", "ps_5_0");
        require(SUCCEEDED(device->CreatePixelShader(code->GetBufferPointer(), code->GetBufferSize(), nullptr, &ps)), "create pixel shader");
        D3D11_RASTERIZER_DESC rs{}; rs.FillMode=D3D11_FILL_SOLID; rs.CullMode=D3D11_CULL_NONE; rs.DepthClipEnable=TRUE;
        require(SUCCEEDED(device->CreateRasterizerState(&rs,&raster)), "create raster state");
        D3D11_DEPTH_STENCIL_DESC ds{}; ds.DepthEnable=TRUE; ds.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ALL; ds.DepthFunc=D3D11_COMPARISON_LESS;
        require(SUCCEEDED(device->CreateDepthStencilState(&ds,&state)), "create depth state");
    }
    void draw(ID3D11DeviceContext *context, bool use_deferred) {
        D3D11_TEXTURE2D_DESC desc{}; depth->GetDesc(&desc);
        D3D11_VIEWPORT viewport{0,0,float(desc.Width),float(desc.Height),0,1};
        auto target=rtv.Get(); context->OMSetRenderTargets(1,&target,dsv.Get());
        context->RSSetViewports(1,&viewport); context->RSSetState(raster.Get());
        context->OMSetDepthStencilState(state.Get(),0);
        context->ClearDepthStencilView(dsv.Get(),D3D11_CLEAR_DEPTH,1,0);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->VSSetShader(use_deferred?far_vs.Get():near_vs.Get(),nullptr,0); context->PSSetShader(ps.Get(),nullptr,0);
        for (int draw=0; draw<9; ++draw) context->Draw(6,0);
        context->ClearDepthStencilView(dsv.Get(),D3D11_CLEAR_DEPTH,1,0);
        context->OMSetRenderTargets(0,nullptr,nullptr);
    }
};
static float sampled_depth(reshade::api::effect_runtime *runtime, ID3D11Device *device, ID3D11DeviceContext *context) {
    auto variable=runtime->find_texture_variable("depth_probe.fx","DepthTex");
    if (variable == 0) return -1;
    reshade::api::resource_view view{};
    runtime->get_texture_binding(variable,&view,nullptr);
    if (!view.handle) return -1;
    ComPtr<ID3D11Resource> resource;
    reinterpret_cast<ID3D11ShaderResourceView *>(view.handle)->GetResource(&resource);
    ComPtr<ID3D11Texture2D> texture;
    require(SUCCEEDED(resource.As(&texture)), "depth input is texture");
    D3D11_TEXTURE2D_DESC desc{}; texture->GetDesc(&desc);
    desc.Usage=D3D11_USAGE_STAGING; desc.BindFlags=0; desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ; desc.MiscFlags=0;
    ComPtr<ID3D11Texture2D> staging;
    require(SUCCEEDED(device->CreateTexture2D(&desc,nullptr,&staging)), "create depth readback");
    context->CopyResource(staging.Get(),texture.Get());
    D3D11_MAPPED_SUBRESOURCE mapped{};
    require(SUCCEEDED(context->Map(staging.Get(),0,D3D11_MAP_READ,0,&mapped)), "read depth input");
    float depth=0;
    memcpy(&depth,static_cast<const char *>(mapped.pData)+(desc.Height/2)*mapped.RowPitch+(desc.Width/2)*sizeof(float),sizeof(float));
    context->Unmap(staging.Get(),0);
    return depth;
}
int wmain(int argc, wchar_t **argv)
{
    require(argc == 3 || (argc == 4 && std::wstring(argv[3]) == L"--hardware"), "usage: manual_addon_smoke <patched DLL> <probe.addon32> [--hardware]");
    const auto temp = std::filesystem::temp_directory_path() /
        (L"crabe-reshade-smoke-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64()));
    require(std::filesystem::create_directory(temp), "create isolated test directory");
    std::filesystem::copy_file(std::filesystem::absolute(argv[2]), temp / "manual_addon_probe.addon32");
    std::ofstream(temp / "ReShade.ini") << "[GENERAL]\nNoReloadOnInit=0\nEffectSearchPaths=.\\\n[DEPTH]\nDepthCopyBeforeClears=1\n[ADDON]\nAddonPath=.\\\n";
    std::ofstream(temp / "depth_probe.fx") << R"FX(
texture DepthTex : DEPTH;
sampler DepthSampler { Texture = DepthTex; };
void vs(uint id : SV_VertexID, out float4 pos : SV_Position, out float2 uv : TEXCOORD) {
    uv=float2((id << 1) & 2, id & 2); pos=float4(uv*float2(2,-2)+float2(-1,1),0,1);
}
float4 ps(float4 pos:SV_Position,float2 uv:TEXCOORD):SV_Target { return tex2D(DepthSampler,uv).rrrr; }
technique DepthProbe { pass { VertexShader=vs; PixelShader=ps; } }
)FX";
    SetEnvironmentVariableW(L"RESHADE_DISABLE_GRAPHICS_HOOK", L"1");
    SetEnvironmentVariableW(L"RESHADE_BASE_PATH_OVERRIDE", temp.c_str());
    const auto dll = std::filesystem::absolute(argv[1]);
    const HMODULE module = LoadLibraryExW(dll.c_str(), nullptr,
        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    require(module != nullptr, "load manual ReShade DLL");
    using Capability = unsigned int(__cdecl *)();
    auto capability = reinterpret_cast<Capability>(GetProcAddress(module, "CrabeReShadeManualAddonSupport"));
    require(capability && capability() == 2, "manual add-on capability export");
    using Create = bool(__cdecl *)(reshade::api::device_api, void *, void *, void *, const char *, reshade::api::effect_runtime **);
    using Destroy = void(__cdecl *)(reshade::api::effect_runtime *);
    using Present = void(__cdecl *)(reshade::api::effect_runtime *);
    auto create = reinterpret_cast<Create>(GetProcAddress(module, "ReShadeCreateEffectRuntime"));
    auto destroy = reinterpret_cast<Destroy>(GetProcAddress(module, "ReShadeDestroyEffectRuntime"));
    auto present = reinterpret_cast<Present>(GetProcAddress(module, "ReShadeUpdateAndPresentEffectRuntime"));
    require(create && destroy && present, "manual runtime exports");
    WNDCLASSW wc{};
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"CrabeReShadeSmoke";
    require(RegisterClassW(&wc) != 0, "register hidden window");
    HWND window = CreateWindowW(wc.lpszClassName, L"Crabe ReShade smoke", WS_OVERLAPPEDWINDOW,
        0, 0, 640, 360, nullptr, nullptr, wc.hInstance, nullptr);
    require(window != nullptr, "create hidden window");
    DXGI_SWAP_CHAIN_DESC desc{};
    desc.BufferDesc.Width = 640;
    desc.BufferDesc.Height = 360;
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 1;
    desc.OutputWindow = window;
    desc.Windowed = TRUE;
    ComPtr<IDXGISwapChain> chain;
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    require(SUCCEEDED(D3D11CreateDeviceAndSwapChain(nullptr, argc == 4 ? D3D_DRIVER_TYPE_HARDWARE : D3D_DRIVER_TYPE_WARP, nullptr, 0,
        nullptr, 0, D3D11_SDK_VERSION, &desc, &chain, &device, nullptr, &context)), "create native D3D11 device");
    auto config = (temp / "ReShade.ini").u8string();
    reshade::api::effect_runtime *runtime = nullptr;
    require(!create(reshade::api::device_api::d3d11, device.Get(), context.Get(), nullptr,
        config.c_str(), &runtime) && runtime == nullptr, "reject invalid swapchain");
    ComPtr<ID3D11DeviceContext> deferred;
    require(SUCCEEDED(device->CreateDeferredContext(0, &deferred)), "create deferred context before runtime");
    int total_frames=0, depth_checks=0;
    for (int cycle = 0; cycle < 3; ++cycle)
    {
        require(create(reshade::api::device_api::d3d11, device.Get(), context.Get(), chain.Get(),
            config.c_str(), &runtime) && runtime, "create runtime with add-ons");
        {
            Scene scene(device.Get(),chain.Get());
            for (int frame = 0; frame < 50; ++frame) {
                const bool use_deferred=(frame%2)!=0;
                scene.draw(use_deferred?deferred.Get():context.Get(),use_deferred);
                if (use_deferred) {
                    ComPtr<ID3D11CommandList> list;
                    require(SUCCEEDED(deferred->FinishCommandList(FALSE,&list)), "finish native deferred commands");
                    context->ExecuteCommandList(list.Get(),FALSE);
                }
                present(runtime); ++total_frames;
                auto technique=runtime->find_technique("depth_probe.fx","DepthProbe");
                if (technique != 0) runtime->set_technique_state(technique,true);
                float depth=sampled_depth(runtime,device.Get(),context.Get());
                if (frame >= 10 && depth >= 0) {
                    require(std::abs(depth-(use_deferred?0.75f:0.25f)) < 0.001f, "rendered immediate/deferred depth survives end-of-frame clear");
                    ++depth_checks;
                }
                Sleep(5);
            }
            context->ClearState(); deferred->ClearState();
        }
        destroy(runtime);
        runtime = nullptr;
        context->ClearState();
        context->Flush();
        require(SUCCEEDED(chain->ResizeBuffers(1, 640 + cycle * 16, 360, DXGI_FORMAT_UNKNOWN, 0)),
            "resize after runtime destruction");
    }
    destroy(nullptr);
    std::map<std::string, int> counts;
    std::ifstream log(temp / "probe.log");
    for (std::string line; std::getline(log, line); ) ++counts[line];
    for (const auto event : {"loaded", "unloaded", "init_device", "destroy_device", "init_runtime",
        "destroy_runtime", "generic_depth_device_ready", "generic_depth_runtime_ready"})
        require(counts[event] == 3, event);
    require(counts["frame"] == total_frames, "external add-on frame callbacks");
    require(counts["native_draw"] == total_frames*9, "native draws counted once; ReShade draws excluded");
    require(depth_checks >= 60, "depth checks across immediate/deferred frames and resize cycles");
    std::cout << "PASS: built-in Generic Depth initialized; external addon loaded/unloaded 3 times; "
                 "rendered immediate/deferred depth preserved before clears; " << depth_checks << " GPU depth checks; 3 successful resizes.\nLogs: " << temp.u8string() << '\n';
    DestroyWindow(window);
    return 0;
}
