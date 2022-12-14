#pragma once
#include "../Floof.h"

#include <unordered_map>
#include <string>
#include "VulkanRenderer.h"
#include "../Math.h"

namespace FLOOF {
    enum class TextureColor {
        None = 0,
        Red,
        Green,
        Blue,
        White,
        LightGrey,
        DarkGrey,
        Black,
        FlatNormal,
        Size,
    };

    class TextureManager {
        friend class Texture;
        friend class Application;

        TextureManager() = delete;

    public:
        static void DestroyAll();
        static VulkanTexture GetBRDFLut();
    private:
        static VulkanTexture GetTextureFromPath(const std::string& path, bool flip = false, bool glNormal = false);
        static VulkanTexture GetTextureFromColor(TextureColor color);

    private:
        inline static std::unordered_map<std::string, VulkanTexture> s_TextureCache;
        inline static std::unordered_map<TextureColor, VulkanTexture> s_ColorTextureCache;
        inline static VulkanTexture s_BRDFLut;
    };
}