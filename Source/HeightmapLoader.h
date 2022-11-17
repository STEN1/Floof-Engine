#pragma once

#include "stb_image/stb_image.h"
#include "Renderer/Vertex.h"

namespace FLOOF {
	class HeightmapLoader {
	public:
		HeightmapLoader();

		void MakeBuffersFromData(const std::vector<MeshVertex>& meshData, const std::vector<uint32_t> indexData);
		bool readMap();
	private:
		const char* filepath { "Assets/TerrainTextures/Terrain_Tough/heightmap.png" };

		std::vector<MeshVertex> mVertices;
		std::vector<int> mIndices;

		float zScale{ 1.0f };

		int height, width, channels;
		unsigned char* img { nullptr };
	};
}