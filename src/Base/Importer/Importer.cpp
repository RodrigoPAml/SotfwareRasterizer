#include <Rasterizer.hpp>
#include "Importer.hpp"

namespace Rasterizer {
namespace Importer
{
    ObjectPtr Importer::Load(const std::string& path)
    {
        srand(time(NULL));

        Assimp::Importer importer;

        const aiScene* scene = importer.ReadFile(
            path, 
            aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_OptimizeMeshes | aiProcess_GenBoundingBoxes 
        );

        if (scene == nullptr)
            return nullptr;

        ObjectPtr object = ObjectPtr(new Object());

        object->basePath = Utils::Directory::RemovePartsFromPath(path, 1);

        if(scene == nullptr || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
            return nullptr;
        else
        {
            ProcessNode(scene->mRootNode, scene, object);
            ProcessMaterials(scene, object);
        }

        return object;
    }

    void Importer::ProcessNode(aiNode* node, const aiScene* scene, ObjectPtr object)
    {
        for(unsigned int i = 0; i < node->mNumMeshes; i++)
            ProcessMesh(scene->mMeshes[node->mMeshes[i]], scene, object);

        for(unsigned int i = 0; i < node->mNumChildren; i++)
           ProcessNode(node->mChildren[i], scene, object);
    }

    void Importer::ProcessMesh(aiMesh* mesh, const aiScene* scene, ObjectPtr object)
    {
        if(!mesh->HasPositions() || mesh->mNumVertices == 0 || mesh->mNumFaces == 0)
            return;

        Mesh m;

        if (!mesh->HasNormals())
            throw std::exception("need normals");

        bool haveUv = mesh->HasTextureCoords(0);

        float* vertices = new float[mesh->mNumFaces * 9];
        float* normals = new float[mesh->mNumFaces * 9];
        float* uvs = new float[mesh->mNumFaces * 9];

        unsigned int index = 0;

        for(int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
           
            if (face.mNumIndices == 3)
            {
                aiVector3D v1 = mesh->mVertices[face.mIndices[0]];
                aiVector3D v2 = mesh->mVertices[face.mIndices[1]];
                aiVector3D v3 = mesh->mVertices[face.mIndices[2]];

                aiVector3D n1 = mesh->mNormals[face.mIndices[0]];
                aiVector3D n2 = mesh->mNormals[face.mIndices[1]];
                aiVector3D n3 = mesh->mNormals[face.mIndices[2]];

                aiVector3D uv1 = { 0, 0, 0 };
                aiVector3D uv2 = { 1, 0, 0 };
                aiVector3D uv3 = { 0, 1, 0 };

                if (haveUv)
                {
                    uv1 = mesh->mTextureCoords[0][face.mIndices[0]];
                    uv2 = mesh->mTextureCoords[0][face.mIndices[1]];
                    uv3 = mesh->mTextureCoords[0][face.mIndices[2]];
                }

                vertices[index] = v1.x;
                normals[index] = n1.x;
                uvs[index] = uv1.x;

                index++;
                vertices[index] = v1.y;
                normals[index] = n1.y;
                uvs[index] = uv1.y;

                index++;
                vertices[index] = v1.z;
                normals[index] = n1.z;

                index++;
                vertices[index] = v2.x;
                normals[index] = n2.x;
                uvs[index] = uv2.x;

                index++;
                vertices[index] = v2.y;
                normals[index] = n2.y;
                uvs[index] = uv2.y;

                index++;
                vertices[index] = v2.z;
                normals[index] = n2.z;

                index++;
                vertices[index] = v3.x;
                normals[index] = n3.x;
                uvs[index] = uv3.x;

                index++;
                vertices[index] = v3.y;
                normals[index] = n3.y;
                uvs[index] = uv3.y;

                index++;
                vertices[index] = v3.z;
                normals[index] = n3.z;

                index++;
            }
            else throw std::exception("only faces with 3 vertices are supported");
        }

        m.vertices = vertices;
        m.normals = normals;
        m.uvs = uvs;
        m.maxBox = { mesh->mAABB.mMax.x,  mesh->mAABB.mMax.y,  mesh->mAABB.mMax.z };
        m.minBox = { mesh->mAABB.mMin.x,  mesh->mAABB.mMin.y,  mesh->mAABB.mMin.z };
        m.verticesSize = mesh->mNumFaces * 9;
        m.haveNormals = false;
        m.haveUVs = false;
        m.material = mesh->mMaterialIndex;

        object->meshes.push_back(m);
    }

    void Importer::ProcessMaterials(const aiScene* scene, ObjectPtr object)
    {
        using namespace Utils;

        aiString* path = new aiString("");
        for (int i = 0; i < scene->mNumMaterials; i++)
        {
            Material m;
            aiMaterial* material = scene->mMaterials[i];

            aiColor4D diffuseColor(1.0f, 1.0f, 1.0f, 1.0f); 

            if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor)) 
                m.color = Vec3<float>(diffuseColor.r, diffuseColor.g, diffuseColor.b);

            if (material->GetTexture(aiTextureType_DIFFUSE, 0, path) == AI_SUCCESS)
            {
                std::string texturePath = path->C_Str();
                m.image = Image(object->basePath + "//" + texturePath, true);
            }

            object->materials.push_back(m);
        }
    }
}}