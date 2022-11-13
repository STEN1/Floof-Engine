#include "Texture.h"

namespace FLOOF {
    Texture::Texture(const std::string& path)
        : Path(path)
    {
        VkTexture = TextureManager::GetTextureFromPath(path);
    }
    Texture::Texture(TextureColor color)
        : Color(color)
    {
        VkTexture = TextureManager::GetTextureFromColor(color);
    }
}