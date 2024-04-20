#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <string>
#include <vector>
#include <map>

#include "Shader.h"

struct Face;

struct Vertex
{
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec2 TexCoords;
	glm::mat4 quadric = glm::mat4(0.0f);
};

struct HalfEdge
{
	HalfEdge* twin;
	HalfEdge* next;
	Vertex* vertex;
	Face* face;
	float cost;
};

struct Face
{
	HalfEdge* halfEdge;
};

struct Texture
{
	unsigned int id;
	std::string type;
	std::string path;
};

class Mesh
{
public:
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	std::vector<Texture> textures;
	std::map<std::pair<unsigned int, unsigned int>, HalfEdge*> edges;
	std::vector<Face> faces;
	unsigned int VAO;

	Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures);

	void Draw(Shader& shader);

private:
	// render data
	unsigned int VBO, EBO;

	void setupMesh();
};


#endif