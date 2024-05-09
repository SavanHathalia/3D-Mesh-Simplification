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
    GLuint64 startTime, stopTime;
    unsigned int queryID[2];

    // generate two queries
    glGenQueries(2, queryID);
    glQueryCounter(queryID[0], GL_TIMESTAMP);

    for (unsigned int i = 0; i < meshes.size(); i++)
        meshes[i].Draw(shader);

    glQueryCounter(queryID[1], GL_TIMESTAMP);
    // get query results
    glGetQueryObjectui64v(queryID[0], GL_QUERY_RESULT, &startTime);
    glGetQueryObjectui64v(queryID[1], GL_QUERY_RESULT, &stopTime);

    timeTaken = (stopTime - startTime) / 1000.0;
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

    modelName = path.substr(path.find_last_of('\\') + 1, path.size() - path.find_last_of('\\') - 5);
    modelName = modelName.substr(path.find_last_of('/') + 1, path.size() - path.find_last_of('/') - 5);

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

// Hard coded model transforms
std::vector<glm::mat4> Model::calcModelMatrix()
{
    std::vector<glm::mat4> modelVec = {};
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 newModel = glm::mat4(1.0f);

    if (this->modelName == "bunny")
    {
        // Original transform
        model = glm::translate(model, glm::vec3(-0.5, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));

        // New model transform
        newModel = model;
        newModel = glm::translate(newModel, glm::vec3(3.5f, 0.0f, 0.0f));
    }
    else if (this->modelName == "armadillo")
    {
        model = glm::translate(model, glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));
        model = glm::rotate(model, glm::radians(180.f), glm::vec3(0, 1, 0));

        newModel = model;
        newModel = glm::translate(newModel, glm::vec3(3.f, 0.0f, 0.0f));
    }
    else if (this->modelName == "dragon")
    {
        model = glm::translate(model, glm::vec3(-0.5f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.01f, 0.01f, 0.01f));
        model = glm::rotate(model, glm::radians(180.f), glm::vec3(0, 1, 0));

        newModel = model;
        newModel = glm::translate(newModel, glm::vec3(-250.f, 0.0f, 0.0f));
    }
    else if (this->modelName == "happy")
    {
        model = glm::translate(model, glm::vec3(-0.7f, -1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(10.f, 10.f, 10.f));

        newModel = model;
        newModel = glm::translate(newModel, glm::vec3(0.2f, 0.0f, 0.0f));
    }
    else if (this->modelName == "lucy")
    {
        model = glm::translate(model, glm::vec3(0.5f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.001f, 0.001f, 0.001f));
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1, 0, 0));
        model = glm::rotate(model, glm::radians(-180.f), glm::vec3(0, 0, 1));

        newModel = model;
        newModel = glm::translate(newModel, glm::vec3(-1000.0f, 0.0f, 0.0f));
    }

    modelVec.push_back(model);
    modelVec.push_back(newModel);

    return modelVec;
}

void Model::addMesh(Mesh mesh)
{
    this->meshes.push_back(mesh);
    calcVertexCount();
}

// MY TRIAL IMPLEMENTATION OF THE QEM

void loadObj(const std::string& filename, Mesh& mesh) {

    // Initiate edges and faces
    mesh.etest = {};
    mesh.ftest = {};

    std::ifstream file(filename);
    std::string line;

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "v") {
            Vertextest vertex;
            iss >> vertex.Position.x >> vertex.Position.y >> vertex.Position.z;
            mesh.vtest.push_back(vertex);
        }
        else if (type == "vn") {
            Normaltest normal;
            iss >> normal.Normal.x >> normal.Normal.y >> normal.Normal.z;
            mesh.ntest.push_back(normal);
        }
        else if (type == "f") {
            unsigned int i1;
            unsigned int i2;
            unsigned int i3;

            unsigned int n1;
            unsigned int n2;
            unsigned int n3;

            char slash;
            iss >> i1 >> slash >> slash >> n1
                >> i2 >> slash >> slash >> n2
                >> i3 >> slash >> slash >> n3;

            i1--;
            i2--;
            i3--;
            mesh.itest.push_back(i1);
            mesh.itest.push_back(i2);
            mesh.itest.push_back(i3);
        }
    }

    for (int i = 0; i < mesh.vtest.size(); i++)
    {
        mesh.vtest[i].normal = &mesh.ntest[i];
        mesh.vtest[i].index = i;
    }
}



void connectFace(Mesh& mesh, unsigned int v1, unsigned int v2, unsigned int v3)
{
    Hetest* he1 = new Hetest;
    Hetest* he2 = new Hetest;
    Hetest* he3 = new Hetest;

    // Setup vertex, the next edge and the face it belongs to
    he1->vertex = &mesh.vtest[v1];
    he2->vertex = &mesh.vtest[v2];
    he3->vertex = &mesh.vtest[v3];

    he1->next = he2;
    he2->next = he3;
    he3->next = he1;

    Facetest* face = new Facetest;
    face->halfEdge = he1;
    he1->face = face;
    he2->face = face;
    he3->face = face;

    face->halfEdge = he1;

    // Insert into edges map
    mesh.etest.insert({ {v1, v2}, he1 });
    mesh.etest.insert({ {v2, v3}, he2 });
    mesh.etest.insert({ {v3, v1}, he3 });

    // Insert into faces vector
    mesh.ftest.push_back(face);
}

void createHalfEdges(Mesh& mesh)
{
    int count = 0;
    printf("here");
    // Populate edges and faces
    for (int i = 0; i < mesh.itest.size() - 2; i += 3)
    {
        bool swap = false;
        for (Facetest* face : mesh.ftest)
        {
            if ((face->halfEdge->vertex->index == mesh.itest[i] && face->halfEdge->next->vertex->index == mesh.itest[i + 1]) ||
                (face->halfEdge->vertex->index == mesh.itest[i + 1] && face->halfEdge->next->vertex->index == mesh.itest[i + 2]) ||
                (face->halfEdge->vertex->index == mesh.itest[i + 2] && face->halfEdge->next->vertex->index == mesh.itest[i]))
            {
                swap = true;
                count++;
            }
        }
        if(swap) connectFace(mesh, mesh.itest[i + 1], mesh.itest[i], mesh.itest[i + 2]);
        else connectFace(mesh, mesh.itest[i], mesh.itest[i + 1], mesh.itest[i + 2]);
    }
    printf("conn: %u\n", count);

    count = 0;
    // Pair twins
    for (std::pair<TestFS, Hetest*> edge : mesh.etest)
    {
        if (!edge.second->twin) // Pair this edge and its twin
        {
            count++;
            edge.second->twin = mesh.etest[{edge.first.second, edge.first.first}];
            mesh.etest[{edge.first.second, edge.first.first}]->twin = edge.second;
        }
    }
    printf("pair: %u\n", count);
}

glm::mat4 calculateQuadric(Facetest* face)
{
    glm::vec3 pos1 = face->halfEdge->vertex->Position;
    glm::vec3 pos2 = face->halfEdge->next->vertex->Position;
    glm::vec3 pos3 = face->halfEdge->next->next->vertex->Position;

    glm::vec3 normal = glm::normalize(glm::cross(pos2 - pos1, pos3 - pos1)); // Find normal

    float d = -glm::dot(normal, pos1); // Calcalate d value in ax + by + cz + d = 0

    glm::vec4 planar = glm::vec4(normal, d); // Planar vector

    return(glm::dot(planar, planar));
}

float calculateCost(const Vertextest& v0, const Vertextest& v1)
{
    return(glm::dot(glm::vec4(v0.Position, 1), (v0.quadric + v1.quadric) * (glm::vec4(v0.Position, 1))));
}

Hetest* calcLeastCostEdge(Mesh& mesh)
{
    Hetest* leastCostEdge = nullptr;
    float leastCost = 9999999;
    // Calculate cost for each edge and store least cost
    for (std::pair<TestFS, Hetest*> edge : mesh.etest)
    {
        float cost = calculateCost(mesh.vtest[edge.first.first], mesh.vtest[edge.first.second]);
        mesh.etest[{edge.first.first, edge.first.second}]->cost = cost;

        if (cost < leastCost)
        {
            leastCost = cost;
            leastCostEdge = edge.second;
        }
    }
    return leastCostEdge;
}

Hetest* findLeastCostEdge(Mesh& mesh)
{
    Hetest* leastCostEdge = nullptr;
    float leastCost = 9999999;
    // Calculate cost for each edge and store least cost
    for (std::pair<TestFS, Hetest*> edge : mesh.etest)
    {
        if (edge.second->cost < leastCost)
        {
            leastCost = edge.second->cost;
            leastCostEdge = edge.second;
        }
    }
    return leastCostEdge;
}


void collapseEdge(Hetest* edge, Mesh& mesh)
{
    // Get the halfedge to collapse and its twin
    Hetest* e1 = edge;
    Hetest* e2 = edge->twin;

    // Update v1 and v2 to be in the midpoint of both
    e1->vertex->Position = (e1->vertex->Position + e2->vertex->Position) * 0.5f;

    // Update quadric of v1 by adding both
    e1->vertex->quadric += e2->vertex->quadric;

    // Mark edges to be removed edges connected to v2 which includes the edge to collapse
    for (std::pair<TestFS, Hetest*> anEdge : mesh.etest)
    {
        if (anEdge.second->vertex == e2->vertex || anEdge.second->next->vertex == e2->vertex) anEdge.second->removed = true;
    }

    // Update the half edge connectivity
    // e1 face
    Hetest* currEdge = e1->next->twin->next;

    // Update faces
    e1->face->removed = true;
    currEdge->face->removed = true;

    e1->next = currEdge->next;
    e1->next->next = currEdge->next->next->twin->next;
    e1->next->next->next = e1;

    Facetest* newFace = new Facetest;
    newFace->halfEdge = e1;
    mesh.ftest.push_back(newFace); // Add new face

    e1->face = newFace;
    e1->next->face = e1->face;
    e1->next->next->face = e1->face;

    // Add edge
    mesh.etest.insert({ {e1->vertex->index, e1->next->vertex->index}, e1 });

    // Calc new cost
    float cost = calculateCost(mesh.vtest[e1->vertex->index], mesh.vtest[e1->next->vertex->index]);
    e1->cost = cost;

    // e2 face
    currEdge = e2->next->next->twin->next;
    Hetest* nextEdge = currEdge->next->twin->next;

    // Mark faces for removal
    e2->face->removed = true;
    currEdge->face->removed = true;

    // Create new edge
    Hetest* newHalfEdge = new Hetest;

    newHalfEdge->vertex = currEdge->next->vertex;
    newHalfEdge->next = e2->next;
    e2->next->next = currEdge;
    currEdge->next = newHalfEdge;

    // Create new face
    newFace = new Facetest;
    newFace->halfEdge = e2;
    mesh.ftest.push_back(newFace); // Add new face

    currEdge->face = e2->next->face;
    newHalfEdge->face = e2->next->face;

    // Add edges
    mesh.etest.insert({ {newHalfEdge->vertex->index, e1->vertex->index}, newHalfEdge });

    // Calc new cost
    cost = calculateCost(mesh.vtest[newHalfEdge->vertex->index], mesh.vtest[e1->vertex->index]);
    currEdge->cost = cost;

    while (nextEdge != e1->next && nextEdge->next->twin)
    {
        currEdge = nextEdge;
        nextEdge = nextEdge->next->twin->next;

        // Remove face
        currEdge->face->removed = true;

        // Create half edges
        newHalfEdge = new Hetest;
        Hetest* newHE2 = new Hetest;

        newHalfEdge->vertex = currEdge->next->vertex;
        newHalfEdge->next = newHE2;
        newHE2->vertex = e1->vertex;
        newHE2->next = currEdge;
        currEdge->next = newHalfEdge;

        // New face
        newFace = new Facetest;
        newFace->halfEdge = currEdge;
        mesh.ftest.push_back(newFace);

        // Set face
        currEdge->face = newFace;
        newHalfEdge->face = currEdge->face;
        newHE2->face = currEdge->face;

        // Add edges
        mesh.etest.insert({ {newHalfEdge->vertex->index, newHE2->vertex->index}, newHalfEdge });
        mesh.etest.insert({ {newHE2->vertex->index, currEdge->vertex->index}, newHE2 });

        // Calculate new costs
        cost = calculateCost(mesh.vtest[newHalfEdge->vertex->index], mesh.vtest[newHE2->vertex->index]);
        newHalfEdge->cost = cost;
        cost = calculateCost(mesh.vtest[newHE2->vertex->index], mesh.vtest[currEdge->vertex->index]);
        newHE2->cost = cost;
    }

    // Pair twins
    for (std::pair<TestFS, Hetest*> edge : mesh.etest)
    {
        if (!edge.second->twin) // Pair this edge and its twin
        {
            edge.second->twin = mesh.etest[{edge.first.second, edge.first.first}];
            mesh.etest[{edge.first.second, edge.first.first}]->twin = edge.second;
        }
    }
}

void deleteEdgesFaces(Mesh& mesh)
{
    // Remove and delete faces
    for (int i = 0; i < mesh.ftest.size(); i++)
    {
        if (mesh.ftest[i]->removed == true)
        {
            mesh.ftest.erase(mesh.ftest.begin() + i);
        }
    }

    // Remove and delete edges
    for (std::pair<TestFS, Hetest*> edge : mesh.etest)
    {
        if (edge.second->removed == true)
        {
            mesh.etest.erase({ edge.first.first, edge.first.second });
        }
    }
}

Mesh extractIndices(Mesh& mesh)
{
    std::vector<unsigned int> indices = {};

    // Add face index data
    for (int i = 0; i < mesh.ftest.size(); i++)
    {
        indices.push_back(mesh.ftest[i]->halfEdge->vertex->index);
        indices.push_back(mesh.ftest[i]->halfEdge->next->vertex->index);
        indices.push_back(mesh.ftest[i]->halfEdge->next->next->vertex->index);
    }

    return { mesh.vertices, indices, {} };;
}

Model Model::simplifyModel(const Model& oldModel, const int vertThreshold)
 {
    // Don't change the original model
    Model newModel = oldModel;

    // For each mesh in the model
    for (Mesh mesh : newModel.meshes)
    {
        loadObj("res/models/bunny/bunny.obj", mesh);

        // Create half-edge data structure
        createHalfEdges(mesh);

        // Calculate quadrics for each vertex
        for (int i = 0; i < mesh.ftest.size(); i++)
        {
            glm::mat4 quadric = calculateQuadric(mesh.ftest[i]);
            mesh.ftest[i]->halfEdge->vertex->quadric += quadric;
            mesh.ftest[i]->halfEdge->next->vertex->quadric += quadric;
            mesh.ftest[i]->halfEdge->next->next->vertex->quadric += quadric;
        }

        // Find least cost edge
        Hetest* leastCostEdge = calcLeastCostEdge(mesh);

        // Collapse edge
        collapseEdge(leastCostEdge, mesh);

        // Remove/delete redundant faces and edges
        deleteEdgesFaces(mesh);

        newModel.vertexCount--;

        while (newModel.vertexCount != vertThreshold)
        {
            leastCostEdge = findLeastCostEdge(mesh);

            collapseEdge(leastCostEdge, mesh);
            newModel.vertexCount--;

            printf("%d\n", newModel.vertexCount);
        }

        // Extract the new indices and create mesh out of it
        mesh = extractIndices(mesh);

        newModel.faceCount = mesh.faces.size();
    }

    return newModel;
}

