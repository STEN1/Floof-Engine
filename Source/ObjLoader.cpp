#include "ObjLoader.h"

#include <fstream>
#include <sstream>
#include <array>
#include <vector>



/*
# = kommentar
o = objektnavn
v = vertex
vt = uv-koordinat
vn = normal
f = hvilken av v og vn og vt utgj�r hvilken av linjene som er en vertex
Og det er tre slike samlinger i hver f-linje, som da utgj�r en trekant.
*/



ObjLoader::ObjLoader(const std::string& path) {
    std::ifstream objFile(path);

    if (!objFile.is_open()) {
        std::cout << "Cant open file: " << path << std::endl;
        return;
    }

    std::string s{};
    while (std::getline(objFile, s)) {
        std::istringstream ss{ s };
        std::array<char, 3> dataType{};
        ss.getline(dataType.data(), 3, ' ');
        std::string type{ dataType.data() };
        if (type == "#") // Comment
        {
            //std::string comment;
            //while (ss >> s)
            //{
            //	comment += s;
            //	comment += " ";
            //}
            //LOG("Obj file comment: " + comment);
        }
        if (type == "o") // obj name
        {
            std::array<char, 256> nameData;
            ss.getline(nameData.data(), nameData.size());
            Name = nameData.data();
        }
        if (type == "v")
            while (ss >> s)
                m_v.emplace_back(std::stof(s));
        if (type == "vt")
            while (ss >> s)
                m_vt.emplace_back(std::stof(s));
        if (type == "vn")
            while (ss >> s)
                m_vn.emplace_back(std::stof(s));
        if (type == "f") {
            while (ss >> s) {
                std::array<char, 6> charArray{};
                std::istringstream ss{ s };
                std::vector<std::string> fStrings;
                while (ss.getline(charArray.data(), charArray.size(), '/')) {
                    std::string fstring{ charArray.data() };
                    fStrings.push_back(fstring);
                }
                m_f.emplace_back(F{
                    std::stoi(fStrings[0]) - 1,
                    std::stoi(fStrings[1]) - 1,
                    std::stoi(fStrings[2]) - 1
                    });
            }
        }
    }

    objFile.close();
    //LOG("Obj loaded: " + Name);
}

std::pair<std::vector<float>, uint32_t> ObjLoader::GetVertexData() {
    std::vector<float> vertexData;
    for (int i = 0; i < m_f.size(); i++) {
        // pos
        vertexData.push_back(m_v[m_f[i].v * 3]);
        vertexData.push_back(m_v[m_f[i].v * 3 + 1]);
        vertexData.push_back(m_v[m_f[i].v * 3 + 2]);
        // color
        //vertexData.push_back(1.f);
        //vertexData.push_back(1.f);
        //vertexData.push_back(1.f);
        // uv
        vertexData.push_back(m_vt[m_f[i].vt * 2]);
        vertexData.push_back(m_vt[m_f[i].vt * 2 + 1]);
        // normal
        vertexData.push_back(m_vn[m_f[i].vn * 3]);
        vertexData.push_back(m_vn[m_f[i].vn * 3 + 1]);
        vertexData.push_back(m_vn[m_f[i].vn * 3 + 2]);
    }
    return { vertexData, m_f.size() };
}


std::pair<std::vector<FLOOF::MeshVertex>, std::vector<uint32_t>> ObjLoader::GetIndexedData() {
    std::vector<ObjLoader::F> builtF;
    int largestIndex{};
    std::vector<FLOOF::MeshVertex> vertexData;
    std::vector<uint32_t> indexData;

    for (int i = 0; i < m_f.size(); i++) {
        int index = GetIndexFromF(m_f[i], builtF);
        if (index == -1) // build vertex data and add index
        {
            FLOOF::MeshVertex vertex;
            vertex.Pos = glm::vec3(m_v[m_f[i].v * 3], m_v[m_f[i].v * 3 + 1], m_v[m_f[i].v * 3 + 2]);
            vertex.Normal = glm::vec3(m_vn[m_f[i].vn * 3], m_vn[m_f[i].vn * 3 + 1], m_vn[m_f[i].vn * 3 + 2]);
            vertex.UV = glm::vec2(m_vt[m_f[i].vt * 2], m_vt[m_f[i].vt * 2 + 1]);
            vertexData.push_back(vertex);

            builtF.push_back(m_f[i]);
            indexData.push_back(largestIndex++);
        } else // add index
        {
            indexData.push_back(index);
        }
    }

    return { vertexData, indexData };
}

int ObjLoader::GetIndexFromF(const F& f, const std::vector<ObjLoader::F>& farr) {
    for (int i = 0; i < farr.size(); i++)
        if (f == farr[i])
            return i;
    return -1;
}

bool ObjLoader::F::operator==(const F& l) const {
    return v == l.v && vt == l.vt && vn == l.vn;
}
