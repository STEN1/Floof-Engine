#pragma once
#include <string>
#include <vector>
#include "Vertex.h"

#include "Floof.h"

class ObjLoader {
public:
    ObjLoader() = delete;
    ObjLoader(const std::string& path);

    std::pair<std::vector<float>, uint32_t> GetVertexData();
    std::pair<std::vector<FLOOF::MeshVertex>, std::vector<uint32_t>> GetIndexedData();

    std::string Name;
private:
    struct F {
        int v{};
        int vt{};
        int vn{};
        bool operator==(const F& l) const;
    };
    int GetIndexFromF(const F& f, const std::vector<ObjLoader::F>& farr);

    std::vector<float> m_v;
    std::vector<float> m_vt;
    std::vector<float> m_vn;
    std::vector<F> m_f;
    std::vector<std::pair<F, int>> fIndexPair;
};