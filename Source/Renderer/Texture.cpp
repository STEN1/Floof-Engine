#include "Texture.h"

namespace FLOOF {
    Texture::Texture(const std::string& path, bool flip, bool glNormal)
        : Path(path)
    {
        VkTexture = TextureManager::GetTextureFromPath(path, flip, glNormal);
    }
    Texture::Texture(TextureColor color)
        : Color(color)
    {
        VkTexture = TextureManager::GetTextureFromColor(color);
    }
}