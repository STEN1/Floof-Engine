#include "HeightmapLoader.h"
#include <iostream>
#include "glm/glm.hpp"

namespace FLOOF {

	HeightmapLoader::HeightmapLoader() {
		if (readFile()) {
			buildHeightmap();
		}
	}

	bool HeightmapLoader::readFile() {

		img = stbi_load(filepath, &width, &height, &channels, 0);
		if (img == 0) {
			std::cout << "Heightmap failed to read or could not find the file in the librariy";
			return false;
		}
		std::cout << "Loaded heigtmap image \n\twidth: " << width << "\tHeight: " << height << ", \tChannels: " << channels;
		buildHeightmap();

		return true;
	}

	bool HeightmapLoader::buildHeightmap() {
		
		for (int x = 0; x < height; x++) {
			for (int y = 0; y < width; y++) {
				float z = img[x * width + y] * zScale;
				mVertices.emplace_back(glm::vec3(x, y, z), glm::vec3(0.f, 0.f, 1.f), glm::vec2(0.f, 0.f));
			}
		}

		for (int x = 0; x < height - 1; x++) {
			for (int y = 0; y < width - 1; y++) {

				auto a{ x * width + y };
				auto b{ (x + 1) * width + y };
				auto c{ (x + 1) * width + (y + 1) };
				auto d{ x * width + (y + 1) };

				mIndices.emplace_back(a);
				mIndices.emplace_back(b);
				mIndices.emplace_back(c);

				mIndices.emplace_back(a);
				mIndices.emplace_back(c);
				mIndices.emplace_back(d);
			}
		}
		for (int x = 1; x < height - 1; x += 1) {
			for (int y = 1; y < width - 1; y += 1) {
				glm::vec3 a = mVertices[(x * width) + y].Pos;
				glm::vec3 b = mVertices[((x + 1) * width) + y].Pos;
				glm::vec3 c = mVertices[((x + 1) * width) + y + 1].Pos;
				glm::vec3 d = mVertices[(x * width) + y + 1].Pos;
				glm::vec3 e = mVertices[((x - 1) * width) + y].Pos;
				glm::vec3 f = mVertices[(x * width) + (y - 1)].Pos;
				glm::vec3 g = mVertices[((x - 1) * width) + (y - 1)].Pos;

				auto a0 = glm::cross(f - a, b - a);
				auto a1 = glm::cross(b - a, d - a);
				auto a2 = glm::cross(d - a, c - a);
				auto a3 = glm::cross(c - a, e - a);
				auto a4 = glm::cross(e - a, g - a);
				auto a5 = glm::cross(g - a, f - a);

				glm::vec3 normal = glm::normalize(a0 + a1 + a2 + a3 + a4 + a5);
				mVertices[(x * height) + y].Normal = normal;

				//uv
				auto UVx = (1.f / height) * y;
				auto UVy = (1.f / width) * x;
				mVertices[(x * width) + y].UV = glm::vec2(UVx,UVy);
			}
		}
		stbi_image_free(img);      //free image memory after bitmap generation
		return true;
	}
}