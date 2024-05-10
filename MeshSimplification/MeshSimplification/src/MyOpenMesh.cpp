#include "MyOpenMesh.h"

void MyOpenMesh::loadMesh(const std::string& path)
{
    if (!OpenMesh::IO::read_mesh(mesh, path)) {
        std::cerr << "Error loading mesh: " << path << std::endl;
    }
    printf("Openmesh mesh load complete.\n");
}

void MyOpenMesh::simplifyMesh(const int& targetVertices)
{
    printf("Simplifying mesh...\n");

    const clock_t begin_time = clock();

    // Define the decimater type
    typedef OpenMesh::Decimater::DecimaterT<oMesh> Decimater;

    // Define the quadric module type
    typedef OpenMesh::Decimater::ModQuadricT<oMesh>::Handle QuadricModule;

    // Initialize the decimater
    Decimater decimater(mesh);

    // Add the quadric module to the decimater
    QuadricModule quadric_module;
    decimater.add(quadric_module);

    // Initialize the decimater
    decimater.initialize();

    double startTime = glfwGetTime();
    // Simplify the mesh to the target number of vertices
    decimater.decimate_to_faces(0Ui64, targetVertices*3);
    timeTaken = glfwGetTime() - startTime;

    // Clean up unused vertices
    mesh.garbage_collection();

    timeTaken = (1000 * float(clock() - begin_time)) / CLOCKS_PER_SEC;

    printf("Simplification complete.\n");
}

void MyOpenMesh::writeMesh(const std::string& path)
{
    printf("Saving to file...\n");
    OpenMesh::IO::write_mesh(mesh, path);
    printf("Saved!\n\n");
}
