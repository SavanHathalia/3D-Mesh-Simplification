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
struct Hetest;

struct Vertex
{
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec2 TexCoords;

	unsigned int index;
	glm::mat4 quadric = glm::mat4(0.0f);
};

struct HalfEdge
{
	Vertex* vertex;

	HalfEdge* next;
	HalfEdge* twin = nullptr;
	Face* face;

	bool removed = false;

	float cost;
};

struct Face
{
	HalfEdge* halfEdge;

	bool removed = false;
};

struct TestFS
{
	unsigned int first;
	unsigned int second;

	bool operator<(const TestFS& fs) const noexcept
	{
		return this->first < fs.first;
	}
};

struct Normaltest {
	glm::vec3 Normal;
};

struct Vertextest {
	glm::vec3 Position;
	Normaltest* normal;

	unsigned int index;
	glm::mat4 quadric = glm::mat4(0.0f);
};

struct Facetest {
	Hetest* halfEdge;
	bool removed = false;
};

struct Hetest
{
	Vertextest* vertex;

	Hetest* next;
	Hetest* twin = nullptr;
	Facetest* face;

	bool removed = false;

	float cost;
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
	std::map<TestFS, HalfEdge*> edges;
	std::vector<Face*> faces;
	unsigned int VAO;

	// My trial of implementing the algorithm
	std::vector<Vertextest> vtest;
	std::vector<Normaltest> ntest;
	std::vector<unsigned int> itest;
	std::vector<Facetest*> ftest;
	std::map<TestFS, Hetest*> etest;

	Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures);

	void Draw(Shader& shader);

private:
	// render data
	unsigned int VBO, EBO;

	void setupMesh();
};


#endif