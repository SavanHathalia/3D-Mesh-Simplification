#ifndef MYIMGUI_H
#define MYIMGUI_H

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <file_browser/ImGuiFileDialog.h>

#include "Model.h"
#include "MyOpenMesh.h"

class MyImGui
{
public:
	// ImGui default settings
	bool bShowControls = true;
	bool bPolygonMode = false;

	std::string filePath;
	std::string filePathName;

	int vertCount = 0;

	MyImGui(std::string& originalModelPath);
	~MyImGui();

	void setup(GLFWwindow* window);
	void newFrame();
	void showControlsWindow();
	void showOptionsWindow();
	void showMeshInfoWindow(const Model& originalModel, const Model& newModel);
	void showImportWindow(Model& originalModel, Model& newModel);
	void toggleWireframe();
	void render();
};

#endif