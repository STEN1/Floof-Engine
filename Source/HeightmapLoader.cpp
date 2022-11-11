#include "HeightmapLoader.h"
#include <iostream>

namespace FLOOF {

	HeightmapLoader::HeightmapLoader() {
		readFile();
	}

	bool HeightmapLoader::readFile() {

		img = stbi_load(filepath, &width, &height, &channels, 0);
		if (img == nullptr) {
			std::cout << "Heightmap failed to read or could not find the file in the librariy";
			return false;
		}
		std::cout << "Loaded heigtmap image \n\twidth: " << width << "\tHeight: " << height << ", Channels: " << channels;
		return true;
	}

	bool HeightmapLoader::buildMesh() {
		
		return true;
	}
}