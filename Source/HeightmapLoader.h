#pragma once

#include "stb_image/stb_image.h"
#include "Renderer/Vertex.h"
#include "Renderer/Mesh.h"

struct MeshData;

namespace FLOOF {
	class HeightmapLoader {
	public:
		HeightmapLoader();


		bool readMap();

		void MakeBuffersFromData(const std::vector<MeshVertex>& meshData, const std::vector<uint32_t> indexData);

		
	private:
		const char* filepath { "Assets/TerrainTextures/Terrain_Tough/heightmap.png" };

		float zScale{ 1.0f };

		int height, width, channels;
		unsigned char* img { nullptr };

	public:
		MeshData getMeshData();
		std::vector<MeshVertex> mVertices;
		std::vector<uint32_t> mIndices;
	};
}