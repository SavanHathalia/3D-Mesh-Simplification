#ifndef MYOPENMESH_H
#define MYOPENMESH_H

#include "GLFW/glfw3.h"

#include "OpenMesh/Core/IO/MeshIO.hh"
#include "OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh"
#include "OpenMesh/Tools/Decimater/DecimaterT.hh"
#include "OpenMesh/Tools/Decimater/ModQuadricT.hh"

#include <string>

class MyOpenMesh
{
public:
	// OpenMesh type
	typedef OpenMesh::TriMesh_ArrayKernelT<> oMesh;
	oMesh mesh;

	double timeTaken = 0.0f;

	MyOpenMesh() {};

	void loadMesh(const std::string& path);
	void simplifyMesh(const int& targetVertices);
	void writeMesh(const std::string& path);
};

#endif