/*
** CrabeLoader
** File description:
** Decodes an image file with Windows Imaging Component into an immutable RGBA texture.
** Paths stay in the game folder: absolute paths and ".." are refused, since a mod names the file.
** Caches failures too, so a missing file costs one disk hit instead of one per frame.
**
** Authors: @LucasLhomme
*/

#include "presentation/texture_cache.hpp"
#include "shared/logger.hpp"

#include <wincodec.h>
#include <windows.h>
#include <wrl/client.h>

#include <cstdint>
#include <vector>

#pragma comment(lib, "windowscodecs.lib")

namespace crabe::presentation {

namespace {

    using Microsoft::WRL::ComPtr;

    bool isConfined(const std::string& path)
    {
        if (path.empty() || path.find(':') != std::string::npos)
            return false;
        if (path.front() == '/' || path.front() == '\\')
            return false;
        return path.find("..") == std::string::npos;
    }

    std::wstring widen(const std::string& text)
    {
        const int length = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
        if (length <= 1)
            return {};
        std::wstring wide(static_cast<std::size_t>(length - 1), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, wide.data(), length);
        return wide;
    }

    ComPtr<IWICImagingFactory> createFactory()
    {
        // The render thread may or may not already be in a COM apartment; either
        // way WIC works, and RPC_E_CHANGED_MODE only means one exists.
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);

        ComPtr<IWICImagingFactory> factory;
        CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
        return factory;
    }

    // Straight-alpha RGBA8, which is what ImGui's blend state expects.
    bool decode(const std::wstring& path, std::vector<std::uint8_t>& pixels, UINT& width, UINT& height)
    {
        ComPtr<IWICImagingFactory> factory = createFactory();
        if (!factory)
            return false;

        ComPtr<IWICBitmapDecoder> decoder;
        if (FAILED(factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ,
                                                      WICDecodeMetadataCacheOnDemand, &decoder)))
            return false;

        ComPtr<IWICBitmapFrameDecode> frame;
        ComPtr<IWICFormatConverter> converter;
        if (FAILED(decoder->GetFrame(0, &frame)) || FAILED(factory->CreateFormatConverter(&converter)))
            return false;
        if (FAILED(converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone,
                                         nullptr, 0.0, WICBitmapPaletteTypeCustom)))
            return false;
        if (FAILED(converter->GetSize(&width, &height)) || width == 0 || height == 0)
            return false;

        pixels.resize(static_cast<std::size_t>(width) * height * 4);
        return SUCCEEDED(converter->CopyPixels(nullptr, width * 4, static_cast<UINT>(pixels.size()), pixels.data()));
    }

}

TextureCache& TextureCache::get()
{
    static TextureCache instance;
    return instance;
}

void TextureCache::attach(ID3D11Device* device)
{
    _device = device;
}

void TextureCache::release()
{
    for (auto& entry : _textures) {
        if (entry.second)
            entry.second->Release();
    }
    _textures.clear();
    _device = nullptr;
}

void* TextureCache::resolve(const std::string& path)
{
    if (!_device)
        return nullptr;

    auto found = _textures.find(path);
    if (found == _textures.end())
        found = _textures.emplace(path, load(path)).first;
    return found->second;
}

void* TextureCache::resolveForDrawBuffer(const std::string& path)
{
    return get().resolve(path);
}

ID3D11ShaderResourceView* TextureCache::load(const std::string& path) const
{
    auto& log = crabe::shared::Logger::getInstance();

    std::vector<std::uint8_t> pixels;
    UINT width = 0;
    UINT height = 0;
    if (!isConfined(path) || !decode(widen(path), pixels, width, height)) {
        log.warning("TextureCache: cannot load image '{}'.", path);
        return nullptr;
    }

    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA data{};
    data.pSysMem = pixels.data();
    data.SysMemPitch = width * 4;

    ComPtr<ID3D11Texture2D> texture;
    ID3D11ShaderResourceView* view = nullptr;
    if (FAILED(_device->CreateTexture2D(&desc, &data, &texture))
        || FAILED(_device->CreateShaderResourceView(texture.Get(), nullptr, &view))) {
        log.warning("TextureCache: cannot create a texture for '{}'.", path);
        return nullptr;
    }

    log.debug("TextureCache: loaded '{}' ({}x{}).", path, width, height);
    return view;
}

} // namespace crabe::presentation
