/*
** CrabeLoader
** File description:
** Declares the cache that turns image files on disk into Direct3D 11 textures for ImGui.
** Render thread only, because it creates GPU resources on the device RenderHook owns.
** Knows no Lua and no draw command; DrawBuffer asks it through a resolver function.
**
** Authors: @LucasLhomme
*/

#ifndef TEXTURE_CACHE_HPP_
#define TEXTURE_CACHE_HPP_

#include <d3d11.h>

#include <string>
#include <unordered_map>

namespace crabe::presentation {

// Decodes PNG, JPEG, BMP and the other formats Windows Imaging Component reads.
// A path is decoded once; one that failed is remembered so a missing file does
// not hit the disk every frame.
class TextureCache final {
    public:
        static TextureCache& get();

        // Binds the cache to the device textures are created on.
        void attach(ID3D11Device* device);

        // Releases every texture and forgets the device.
        void release();

        // The ImGui texture id for `path` (relative to the game folder, no ".."),
        // or nullptr when the file cannot be read or no device is attached.
        void* resolve(const std::string& path);

        // Same as resolve(), shaped for DrawBuffer::setImageResolver.
        static void* resolveForDrawBuffer(const std::string& path);

    private:
        TextureCache() = default;
        ~TextureCache() = default;
        TextureCache(const TextureCache&) = delete;
        TextureCache& operator=(const TextureCache&) = delete;

        ID3D11ShaderResourceView* load(const std::string& path) const;

        ID3D11Device* _device = nullptr;
        std::unordered_map<std::string, ID3D11ShaderResourceView*> _textures;
};

} // namespace crabe::presentation

#endif /* !TEXTURE_CACHE_HPP_ */
