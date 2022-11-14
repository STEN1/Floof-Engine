#pragma once
#include <string>
#include <vector>
#include "Renderer/Vertex.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
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

class AssimpLoader
{
public:
    AssimpLoader() = delete;
    AssimpLoader(const std::string& path);
private:
    unsigned int aiFlags = aiProcess_Triangulate | aiProcess_GenUVCoords | aiProcess_FlipUVs;
    std::string path = "";

    struct AssimpMaterial
    {
        std::string DiffusePath;
        std::string NormalsPath;
        std::string MetallicPath;
        std::string RoughnessPath;
        std::string AOPath;
        std::string Name;
    };

    struct AssimpMesh
    {
        std::vector<FLOOF::MeshVertex> vertices;
        std::vector<uint32_t> indices;
        std::string name;

        AssimpMaterial material;
        aiMatrix4x4 Transform;
    };

    struct AssimpCollisionMesh
    {
        AssimpMesh mesh;
    } collisionMesh;

    struct AssimpStaticMesh
    {
        std::vector<AssimpMesh> meshes;
    } staticMesh;

    void ProcessNode(aiNode* node, const aiScene* scene, const aiMatrix4x4& nodeTransform);
    bool LoadMesh(aiMesh* mesh, const aiScene* scene, const aiMatrix4x4& nodeTransform);
    void LoadModel(const std::string& path);

    std::string LoadMaterialTexture(aiMaterial* mat, aiTextureType type);

public:
    const AssimpStaticMesh& GetAssimpStaticMesh() const
    {
        return staticMesh;
    }
};