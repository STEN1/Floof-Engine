#pragma once

#include "stb_image/stb_image.h"
#include "Renderer/Vertex.h"

namespace FLOOF {
	class HeightmapLoader {
	public:
		HeightmapLoader();

		bool readFile();
		bool buildMesh();
	private:
		const char* filepath { "Assets/TerrainTextures/Terrain_Tough/heightmap.png" };

		std::vector<ColorVertex> PointData;
		std::vector<MeshVertex> VertexData;
		std::vector<ColorNormalVertex> ColorNormalVertexData;
		std::vector<uint32_t> IndexData;
		std::vector<MeshVertex> mVertexsData;

		int height, width, channels;
		unsigned char* img{ nullptr };
	};
}