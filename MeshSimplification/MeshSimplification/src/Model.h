#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <chrono>

#include "Mesh.h"
#include "Shader.h"

unsigned int TextureFromFile(const char* path, const std::string& directory, bool gamma = false);

struct HalfEdge;

class Model
{
public:
	std::vector<Texture> textures_loaded; // Stores all textures loaded so far, optimization to make sure texures aren't loaded more than once
	std::vector<Mesh> meshes;
	std::string directory;
	bool gammaCorrection;
	int faceCount = 0;
	int vertexCount = 0;

	// Constructor, expects a filepath to the 3D model
	Model(const std::string& path, bool gamma = false);

	Model() : gammaCorrection(false) {};

	void Draw(Shader& shader);

	void addMesh(Mesh mesh);

	Model simplifyModel(const Model& oldModel, const int vertThreshold);

private:
	void loadModel(const std::string& path);

	void processNode(aiNode* node, const aiScene* scene);

	Mesh processMesh(aiMesh* mesh, const aiScene* scene);

	std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);

	void calcVertexCount();
};

#endif