#include "Model.h"

unsigned int TextureFromFile(const char* path, const std::string& directory, bool gamma)
{
    std::string filename = std::string(path);
    filename = directory + '/' + filename;

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3) // .jpg
            format = GL_RGB;
        else if (nrComponents == 4) // .png
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

Model::Model(const std::string& path, bool gamma) : gammaCorrection(gamma)
{
    loadModel(path);
    calcVertexCount();
}

void Model::Draw(Shader& shader)
{
    for (unsigned int i = 0; i < meshes.size(); i++)
        meshes[i].Draw(shader);
}

void Model::loadModel(const std::string& path)
{
    // read file via ASSIMP
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
    // check for errors
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
    {
        std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
        return;
    }
    // retrieve the directory path of the filepath
    directory = path.substr(0, path.find_last_of('/'));

    // process ASSIMP's root node recursively
    processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode* node, const aiScene* scene)
{
    // process each mesh located at the current node
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        // the node object only contains indices to index the actual objects in the scene. 
        // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene));
    }
    // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene);
    }
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
    // data to fill
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    // walk through each of the mesh's vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;
        glm::vec3 vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
        // positions
        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;
        vertex.Position = vector;
        // normals
        if (mesh->HasNormals())
        {
            vector.x = mesh->mNormals[i].x;
            vector.y = mesh->mNormals[i].y;
            vector.z = mesh->mNormals[i].z;
            vertex.Normal = vector;
        }
        // texture coordinates
        if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
        {
            glm::vec2 vec;
            // a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't 
            // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.TexCoords = vec;
        }
        else
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);

        vertices.push_back(vertex);
    }
    // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        // retrieve all indices of the face and store them in the indices vector
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }
    // process materials
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
    // we assume a convention for sampler names in the shaders. Each diffuse texture should be named
    // as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER. 
    // Same applies to other texture as the following list summarizes:
    // diffuse: texture_diffuseN
    // specular: texture_specularN
    // normal: texture_normalN

    // 1. diffuse maps
    std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
    // 2. specular maps
    std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

    /* If implementing normal maps and height maps */
    /*
    // 3. normal maps
    std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
    textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
    // 4. height maps
    std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
    textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
    */

    faceCount = mesh->mNumFaces;

    // return a mesh object created from the extracted mesh data
    return Mesh(vertices, indices, textures);
}

std::vector<Texture> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName)
{
    std::vector<Texture> textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);
        // check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
        bool skip = false;
        for (unsigned int j = 0; j < textures_loaded.size(); j++)
        {
            if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
            {
                textures.push_back(textures_loaded[j]);
                skip = true; // a texture with the same filepath has already been loaded, continue to next one. (optimization)
                break;
            }
        }
        if (!skip)
        {   // if texture hasn't been loaded already, load it
            Texture texture;
            texture.id = TextureFromFile(str.C_Str(), this->directory);
            texture.type = typeName;
            texture.path = str.C_Str();
            textures.push_back(texture);
            textures_loaded.push_back(texture);  // store it as texture loaded for entire model, to ensure we won't unnecessary load duplicate textures.
        }
    }
    return textures;
}

void Model::calcVertexCount()
{
    for (int i = 0; i < meshes.size(); i++)
    {
        vertexCount += meshes[i].vertices.size();
    }
    vertexCount = (vertexCount + 12) / 6;
}

void Model::addMesh(Mesh mesh)
{
    this->meshes.push_back(mesh);
    calcVertexCount();
}

void createHalfEdges(Model& model)
{
    for (int i = 0; i < model.meshes.size(); i++)
    {
        Mesh& currMesh = model.meshes[i];
        currMesh.faces = {};
        for (int j = 0; j < currMesh.indices.size() - 2; j+=3)
        {
            // Set vertex and face of half edge
            currMesh.edges[{currMesh.indices[j], currMesh.indices[j+1]}] = new HalfEdge;
            currMesh.edges[{currMesh.indices[j], currMesh.indices[j + 1]}]->vertex = &currMesh.vertices[currMesh.indices[j]]; // Vertex
            currMesh.edges[{currMesh.indices[j], currMesh.indices[j + 1]}]->face->halfEdge = currMesh.edges[{j, j + 1}]; // Face

            currMesh.edges[{currMesh.indices[j+1], currMesh.indices[j + 2]}] = new HalfEdge;
            currMesh.edges[{currMesh.indices[j+1], currMesh.indices[j + 2]}]->vertex = &currMesh.vertices[currMesh.indices[j+1]]; // Vertex
            currMesh.edges[{currMesh.indices[j + 1], currMesh.indices[j + 2]}]->face->halfEdge = currMesh.edges[{j, j+1}]; // Face

            currMesh.edges[{currMesh.indices[j+2], currMesh.indices[j]}] = new HalfEdge;
            currMesh.edges[{currMesh.indices[j+2], currMesh.indices[j]}]->vertex = &currMesh.vertices[currMesh.indices[j+2]]; // Vertex
            currMesh.edges[{currMesh.indices[j+2], currMesh.indices[j]}]->face->halfEdge = currMesh.edges[{j, j + 1}]; // Face

            // Populate faces
            currMesh.faces[j / 3].halfEdge = currMesh.edges[{j, j + 1}];
        }
        for (int j = 0; j < currMesh.indices.size() - 2; j += 3)
        {
            // Set next half edge
            currMesh.edges[{currMesh.indices[j], currMesh.indices[j + 1]}]->next = currMesh.edges[{currMesh.indices[j + 1], currMesh.indices[j + 2]}];
            currMesh.edges[{currMesh.indices[j + 1], currMesh.indices[j + 2]}]->next = currMesh.edges[{currMesh.indices[j + 2], currMesh.indices[j]}];
            currMesh.edges[{currMesh.indices[j + 2], currMesh.indices[j]}]->next = currMesh.edges[{currMesh.indices[j], currMesh.indices[j + 1]}];

            // Set twin half edges
            if (currMesh.edges.find({ currMesh.indices[j + 1], currMesh.indices[j] }) != currMesh.edges.end())
            {
                currMesh.edges[{currMesh.indices[j], currMesh.indices[j + 1]}]->twin = currMesh.edges[{currMesh.indices[j + 1], currMesh.indices[j]}];
                currMesh.edges[{currMesh.indices[j+1], currMesh.indices[j]}]->twin = currMesh.edges[{currMesh.indices[j], currMesh.indices[j+1]}];
            }
            if (currMesh.edges.find({ currMesh.indices[j + 2], currMesh.indices[j+1] }) != currMesh.edges.end())
            {
                currMesh.edges[{currMesh.indices[j+1], currMesh.indices[j + 2]}]->twin = currMesh.edges[{currMesh.indices[j + 2], currMesh.indices[j+1]}];
                currMesh.edges[{currMesh.indices[j + 2], currMesh.indices[j+1]}]->twin = currMesh.edges[{currMesh.indices[j+1], currMesh.indices[j + 2]}];
            }
            if (currMesh.edges.find({ currMesh.indices[j], currMesh.indices[j+2] }) != currMesh.edges.end())
            {
                currMesh.edges[{currMesh.indices[j+2], currMesh.indices[j]}]->twin = currMesh.edges[{currMesh.indices[j], currMesh.indices[j+2]}];
                currMesh.edges[{currMesh.indices[j], currMesh.indices[j+2]}]->twin = currMesh.edges[{currMesh.indices[j+2], currMesh.indices[j]}];
            }
        }
    }
}

glm::mat4 calculateQuadric(const Face& face)
{
    glm::vec3 pos1 = face.halfEdge->vertex->Position;
    glm::vec3 pos2 = face.halfEdge->next->vertex->Position;
    glm::vec3 pos3 = face.halfEdge->next->next->vertex->Position;

    glm::vec3 normal = glm::normalize(glm::cross(pos2 - pos1, pos3 - pos1)); // Find normal

    float d = -glm::dot(normal, pos1); // Calcalate d value in ax + by + cz + d = 0

    glm::vec4 planar = glm::vec4(normal, d); // Planar vector

    return(glm::dot(planar, planar));
}

float calculateCost(const Vertex& v0, const Vertex& v1)
{
    return(glm::dot(glm::vec4(v0.Position, 1), (v0.quadric + v1.quadric) * (glm::vec4(v0.Position, 1))));
}

Model Model::simplifyModel(const Model& oldModel, const int vertThreshold)
{
    // Don't change the original model
    Model newModel = oldModel;

    // Create half-edge data structure
    createHalfEdges(newModel);

    // For each mesh in the model
    for (int i = 0; i < newModel.meshes.size(); i++)
    {
        Mesh& currMesh = newModel.meshes[i];

        // Calculate quadrics for each vertex
        for (int j = 0; j < currMesh.faces.size(); j++)
        {
            glm::mat4 quadric = calculateQuadric(currMesh.faces[j]);
            currMesh.faces[j].halfEdge->vertex->quadric += quadric;
            currMesh.faces[j].halfEdge->next->vertex->quadric += quadric;
            currMesh.faces[j].halfEdge->next->next->vertex->quadric += quadric;
        }

        HalfEdge* leastCostEdge;
        float leastCost = 9999999;
        // Calculate cost for each edge and store least cost
        for (std::map<std::pair<unsigned int, unsigned int>, HalfEdge*>::iterator j = currMesh.edges.begin(); j != currMesh.edges.end(); j++)
        {
            float cost = calculateCost(currMesh.vertices[j->first.first], currMesh.vertices[j->first.second]);
            currMesh.edges[{j->first.first, j->first.second}]->cost = cost;
            
            if (cost < leastCost)
            {
                leastCost = cost;
                leastCostEdge = j->second;
            }
        }

        // Collapse edge

    }
    
}