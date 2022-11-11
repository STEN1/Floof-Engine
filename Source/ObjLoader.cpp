#include "ObjLoader.h"

#include <fstream>
#include <sstream>
#include <array>
#include <vector>
#include <filesystem>



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


AssimpLoader::AssimpLoader(const std::string& path)
    :path(path)
{
    LoadModel(path);
}

bool AssimpLoader::LoadMesh(aiMesh* mesh, const aiScene* scene)
{
    AssimpLoader::AssimpMesh internalMesh;

    for (auto i = 0; i < mesh->mNumVertices; i++)
    {
        FLOOF::MeshVertex vertex;

        {	//Position and normals
            for (auto j = 0; j < 3; j++)
            {
                vertex.Pos[j] = mesh->mVertices[i][j];
            }

            if (mesh->HasNormals())
                for (auto j = 0; j < 3; j++)
                    vertex.Normal[j] = mesh->mNormals[i][j];
        }
        {	//Texture coords
            if (mesh->mTextureCoords[0])
            {
                for (auto j = 0; j < 2; j++)
                {
                    vertex.UV[j] = mesh->mTextureCoords[0][i][j];
                }
            }
        }
        {	//Tangents
            if (mesh->HasTangentsAndBitangents())
                for (auto j = 0; j < 3; j++)
                {
                    vertex.Tangent[j] = mesh->mTangents[i][j];
                    vertex.BitTangent[j] = mesh->mBitangents[i][j];
                }
        }
        internalMesh.vertices.emplace_back(vertex);
    }
    {	//Indices
        for (auto i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for (auto j = 0; j < face.mNumIndices; j++)
                internalMesh.indices.push_back(face.mIndices[j]);
        }
    }
    {	//Bonedata
        if (mesh->HasBones())
        {
            std::cerr << "Static mesh has bones, use skeletal mesh manager if you need bones! current path: " << path << std::endl;
        }
    }

    internalMesh.name = mesh->mName.data;
    std::cout << "Mesh name: " << internalMesh.name << std::endl;
    aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
    std::cout << "Material name: " << mat->GetName().C_Str() << std::endl;

    for (uint32_t i = 0; i <= AI_TEXTURE_TYPE_MAX; i++) {
        auto textureCount = mat->GetTextureCount(static_cast<aiTextureType>(i));
        if (textureCount > 0) {
            std::cout << "Has " << textureCount << " texture of type: " << i << std::endl;
        }
    }

    LoadMaterialTexture(mat, aiTextureType_DIFFUSE, "texture_diffuse");
    LoadMaterialTexture(mat, aiTextureType_NORMALS, "texture_normals");
    LoadMaterialTexture(mat, aiTextureType_SPECULAR, "texture_specular");

    staticMesh.meshes.emplace_back(internalMesh);
    return true;
}

void AssimpLoader::ProcessNode(aiNode* node, const aiScene* scene)
{
    for (auto i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        if (!LoadMesh(mesh, scene))
            std::cerr << "failed to load mesh: " << path << std::endl;
    }
    for (auto i = 0; i < node->mNumChildren; i++)
    {
        ProcessNode(node->mChildren[i], scene);
    }
};

void AssimpLoader::LoadModel(const std::string& path)
{
    Assimp::Importer importer;
    const aiScene* scene;

    std::cout << "Loading model from path: " << path << std::endl;

    if (!std::filesystem::exists(path))
    {
        std::cout << "Failed to load model from path: " << path << std::endl;
       // throw std::exception("Failed to find Assimp Model Path!");
    }

    scene = importer.ReadFile(path, aiFlags);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
    {
        std::cout << "Assimp failed to load " << importer.GetErrorString() << std::endl;
        return;
    }

    ProcessNode(scene->mRootNode, scene);
}

bool AssimpLoader::LoadMaterialTexture(aiMaterial* mat, aiTextureType type, std::string typeName)
{
    if (mat->GetTextureCount(type) == 0) {
        return false;
    }

    std::cout << typeName << " path:";
    for (uint32_t i = 0; i < mat->GetTextureCount(type); i++) {
        aiString path;
        mat->GetTexture(type, i, &path);
        std::cout << " " << path.C_Str();
    }
    std::cout << std::endl;
}
