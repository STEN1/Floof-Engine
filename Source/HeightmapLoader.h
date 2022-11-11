#pragma once

#include "stb_image/stb_image.h"
#include "Renderer/Vertex.h"

namespace FLOOF {
	class HeightmapLoader {
	public:
		HeightmapLoader();

		bool readFile();
		bool buildHeightmap();
	private:
		const char* filepath { "Assets/TerrainTextures/Terrain_Tough/heightmap.png" };

		std::vector<MeshVertex> mVertices;
		std::vector<int> mIndices;

		float zScale{ 1.0f };

		int height, width, channels;
		unsigned char* img{ nullptr };
	};
}