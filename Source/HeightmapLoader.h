#pragma once

#include "stb_image/stb_image.h"
#include "Renderer/Vertex.h"
#include "Renderer/Mesh.h"

struct MeshData;

namespace FLOOF {
	class HeightmapLoader {
	public:
		HeightmapLoader(const char* mapstr);


		bool readMap();

		void MakeBuffersFromData(const std::vector<MeshVertex>& meshData, const std::vector<uint32_t> indexData);

        int height, width;
	private:
		const char* filepath;

		float zScale{ 1.0f };

        int channels;
		unsigned char* img { nullptr };

	public:
		MeshData getMeshData();
		std::vector<MeshVertex> mVertices;
		std::vector<uint32_t> mIndices;
	};
}