#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <file_browser/ImGuiFileDialog.h>

#include <iostream>
#include <map>

#include "Camera.h"
#include "Shader.h"
#include "Model.h"
#include "MyOpenMesh.h"
#include "MyImGui.h"

// Window resize
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

// Keyboard/Mouse input
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void processInput(GLFWwindow* window, int key, int scancode, int action, int mods);

// Settings
unsigned int windowWidth = 1920;
unsigned int windowHeight = 1080;

// Camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = windowWidth / 2.0f;
float lastY = windowHeight / 2.0f;
bool firstMouse = true;
bool windowActive = false;

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

// Setup ImGui
std::string originalModelPath = "res/models/bunny/bunny.obj"; // Path to inital bunny model loaded in
MyImGui myImGui(originalModelPath);

int main(void)
{
    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    // Window parameters setup
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(windowWidth, windowHeight, "3D Mesh Simplification", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // V-Sync is on.

    // Window resize
    glfwSetFramebufferSizeCallback(window, &framebuffer_size_callback); // Account for resizing window
    // Input handling
    glfwSetKeyCallback(window, &processInput);
    glfwSetCursorPosCallback(window, &mouse_callback);
    glfwSetScrollCallback(window, &scroll_callback);

    // Initialise GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Output relevant information
    std::printf("RENDERER %s\n", glGetString(GL_RENDERER));
    std::printf("VENDOR %s\n", glGetString(GL_VENDOR));
    std::printf("VERSION %s\n", glGetString(GL_VERSION));
    std::printf("SHADING_LANGUAGE_VERSION %s\n\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

    // Global GL setup goes here
    //glEnable(GL_FRAMEBUFFER_SRGB);
    //glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    Shader shaderProgram("res/shaders/default.vert", "res/shaders/default.frag");
    Shader lightProgram("res/shaders/light.vert", "res/shaders/light.frag");

    float lightVertices[] = {
        // positions        
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,

        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,

        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,

         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,

        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f,

        -0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f
    };

    // positions of the point lights
    glm::vec3 pointLightPositions[] = {
        glm::vec3(0.7f,  0.2f,  2.0f),
        glm::vec3(0.0f,  0.0f, -3.0f)
    };

    unsigned int lightCubeVAO, lightVBO;
    glGenVertexArrays(1, &lightCubeVAO);
    glGenBuffers(1, &lightVBO);

    glBindVertexArray(lightCubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lightVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lightVertices), lightVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // lighting
    glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

    // Load obj
    Model originalModel(originalModelPath);

    // Placeholder for simplified mesh
    Model newModel(originalModelPath);

    // ImGui setup
    myImGui.setup(window);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        processInput(window);
        // Per-frame time logic
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        /* Render here */
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shaderProgram.Activate();

        // directional light
        shaderProgram.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
        shaderProgram.setVec3("dirLight.ambient", 0.05f, 0.05f, 0.05f);
        shaderProgram.setVec3("dirLight.diffuse", 0.4f, 0.4f, 0.4f);
        shaderProgram.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);
        // point light 1
        shaderProgram.setVec3("pointLights[0].position", pointLightPositions[0]);
        shaderProgram.setVec3("pointLights[0].ambient", 0.05f, 0.05f, 0.05f);
        shaderProgram.setVec3("pointLights[0].diffuse", 0.8f, 0.8f, 0.8f);
        shaderProgram.setVec3("pointLights[0].specular", 1.0f, 1.0f, 1.0f);
        shaderProgram.setFloat("pointLights[0].constant", 1.0f);
        shaderProgram.setFloat("pointLights[0].linear", 0.09f);
        shaderProgram.setFloat("pointLights[0].quadratic", 0.032f);
        // point light 2
        shaderProgram.setVec3("pointLights[1].position", pointLightPositions[1]);
        shaderProgram.setVec3("pointLights[1].ambient", 0.05f, 0.05f, 0.05f);
        shaderProgram.setVec3("pointLights[1].diffuse", 0.8f, 0.8f, 0.8f);
        shaderProgram.setVec3("pointLights[1].specular", 1.0f, 1.0f, 1.0f);
        shaderProgram.setFloat("pointLights[1].constant", 1.0f);
        shaderProgram.setFloat("pointLights[1].linear", 0.09f);
        shaderProgram.setFloat("pointLights[1].quadratic", 0.032f);

        // Material
        shaderProgram.setFloat("material.shininess", 32.0f);

        // pass projection matrix to shader (note that in this case it could change every frame)
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)windowWidth / (float)windowHeight, 0.1f, 100.0f);
        shaderProgram.setMat4("projection", projection);

        // camera/view transformation
        glm::mat4 view = camera.GetViewMatrix();
        shaderProgram.setMat4("view", view);

        // Model output
        std::vector<glm::mat4> modelMats = originalModel.calcModelMatrix();
        shaderProgram.setMat4("model", modelMats[0]);
        originalModel.Draw(shaderProgram);

        shaderProgram.setMat4("model", modelMats[1]);
        newModel.Draw(shaderProgram);

        // Output light itself using the light shaders
        lightProgram.Activate();
        lightProgram.setMat4("projection", projection);
        lightProgram.setMat4("view", view);

        glBindVertexArray(lightCubeVAO);
        for (unsigned int i = 0; i < 2; i++)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, pointLightPositions[i]);
            model = glm::scale(model, glm::vec3(0.2f)); // Make it a smaller cube
            lightProgram.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, sizeof(lightVertices) / 3 * sizeof(float));
        }

        // Load ImGui menus
        myImGui.newFrame();
        myImGui.showControlsWindow();
        myImGui.showOptionsWindow();
        myImGui.showMeshInfoWindow(originalModel, newModel);
        myImGui.showImportWindow(originalModel, newModel);
        myImGui.toggleWireframe();
        myImGui.render();

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    windowWidth = width;
    windowHeight = height;
    glViewport(0, 0, width, height);
}

// Processes every frame
void processInput(GLFWwindow* window)
{
    if (windowActive)
    {
        // tell GLFW to capture our mouse
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.ProcessKeyboard(FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.ProcessKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.ProcessKeyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.ProcessKeyboard(RIGHT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            camera.ProcessKeyboard(UP, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            camera.ProcessKeyboard(DOWN, deltaTime);
    }
    else
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

// Processes only on press
void processInput(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if(key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
        glfwSetCursorPos(window, windowWidth / 2, windowHeight / 2);
        if (windowActive)windowActive = false;
        else windowActive = true;
    }
    
    if (windowActive)
    {
        if (key == GLFW_KEY_C && action == GLFW_PRESS)
        {
            myImGui.bShowControls = !myImGui.bShowControls;
        }

        if (key == GLFW_KEY_P && action == GLFW_PRESS)
        {
            myImGui.bPolygonMode = !myImGui.bPolygonMode;
        }

        if (key == GLFW_KEY_M && action == GLFW_PRESS)
        {
            myImGui.bShowImportMenu = !myImGui.bShowImportMenu;
        }
    }
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    if (windowActive)
    {
        float xpos = static_cast<float>(xposIn);
        float ypos = static_cast<float>(yposIn);

        if (firstMouse)
        {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

        lastX = xpos;
        lastY = ypos;

        camera.ProcessMouseMovement(xoffset, yoffset);
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if(windowActive) camera.ProcessMouseScroll(static_cast<float>(yoffset));
}