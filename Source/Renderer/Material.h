#pragma once
#include "Texture.h"

namespace FLOOF
{
	struct Material
	{
		bool HasTexture(TextureType type) const
		{
			return textures[type].IsValid();
		}

		void SetTexture(const Texture& texture)
		{
			if (!texture.IsValid()) return;

			textures[texture.GetTextureType()];
		}

		const Texture& GetTexture(TextureType type) const
		{
			return textures[type];
		}

	private:
		Texture textures[TextureType::Size];
	};
}