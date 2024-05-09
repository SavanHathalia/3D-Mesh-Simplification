#include "MyImGui.h"

MyImGui::MyImGui(std::string& originalModelPath)
{
    filePathName = originalModelPath;
}

MyImGui::~MyImGui()
{
    // End ImGui functions
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void MyImGui::setup(GLFWwindow* window)
{
    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 430");
}

void MyImGui::newFrame()
{
    // ImGui setup
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void MyImGui::showControlsWindow()
{
    // Show controls
    if (bShowControls)
    {
        ImGui::SetNextWindowSize(ImVec2(220, 210));
        ImGui::Begin("Controls:");
        ImGui::Text("Space: Toggle window active");
        ImGui::Text("Mouse: Camera rotation");
        ImGui::Text("W: Forward");
        ImGui::Text("S: Backward");
        ImGui::Text("A: Left");
        ImGui::Text("D: Right");
        ImGui::Text("E: Up");
        ImGui::Text("Q: Down");
        ImGui::Text("C: Toggle controls");
        ImGui::Text("P: Toggle polygon mode");
        ImGui::Text("M: Show import menu");
        ImGui::End();
    }
}

void MyImGui::showOptionsWindow()
{
    // Options window
    ImGui::SetNextWindowSize(ImVec2(250, 100));
    ImGui::Begin("Options:");
    ImGui::Checkbox("C: Toggle controls", &bShowControls);
    ImGui::Checkbox("P: Toggle wireframe mode", &bPolygonMode);
    ImGui::Checkbox("M: Show import menu", &bShowImportMenu);
    ImGui::End();
}

void MyImGui::showMeshInfoWindow(const Model& originalModel, const Model& newModel)
{
    // Mesh info window
    ImGui::SetNextWindowSize(ImVec2(250, 250));
    ImGui::Begin("Mesh Info:");
    ImGui::Text("Original mesh:\nVertex count: %i", originalModel.vertexCount);
    ImGui::Text("Face count: %i", originalModel.faceCount);
    ImGui::Text("Time taken to load: %.1f us", originalModel.timeTaken);
    ImGui::Text("\nSimplified mesh:\nVertex count: %i", newModel.vertexCount);
    ImGui::Text("Face count: %i", newModel.faceCount);
    ImGui::Text("Time taken to load: %.1f us", newModel.timeTaken);
    ImGui::Text("\nSimplification percent: %.1f%%", ((float)newModel.vertexCount / (float)originalModel.vertexCount) * 100.f);
    ImGui::Text("Time taken to simplify: %.1f ms", timeTaken);
    ImGui::End();
}

void MyImGui::showImportWindow(Model& originalModel, Model& newModel)
{
    if(bShowImportMenu)
    {
        // OBJ import window
        ImGui::SetNextWindowSize(ImVec2(900, 220));
        ImGui::Begin("Mesh importer:");
        ImGui::Text("Loaded obj:\n%s\n----------", filePathName.c_str());
        ImGui::Text("Load obj:");
        // open Dialog Simple
        if (ImGui::Button("Browse files...")) {
            IGFD::FileDialogConfig config;
            config.path = ".";
            ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".obj", config);
        }
        // display
        if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey")) {
            if (ImGuiFileDialog::Instance()->IsOk()) { // action if OK
                filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
                filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
                printf("Loaded OBJ file located at: %s\n\n", filePathName.c_str());
                // action
                originalModel = Model(filePathName);
                newModel = Model(filePathName);
                vertCount = 0;
                ImGui::Text("Loaded OBJ file located at: %s", filePathName.c_str());
            }

            // close
            ImGuiFileDialog::Instance()->Close();
        }

        // simplify mesh slider
        ImGui::Text("----------\nDesired vertex count:");
        ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - 80);
        if(ImGui::IsMouseReleased(ImGui::SliderInt("vertices", &vertCount, 0, originalModel.vertexCount)))
        {
            printf("Stared simplification...\n");
            MyOpenMesh simpMesh;
            simpMesh.loadMesh(filePathName);
            simpMesh.simplifyMesh(vertCount);
            simpMesh.writeMesh("res/models/simplified_mesh.obj");
            timeTaken = simpMesh.timeTaken;
            newModel = Model("res/models/simplified_mesh.obj");
        }
        ImGui::Text("Simplification percent: %.1f%%", ((float)vertCount / (float)originalModel.vertexCount) * 100.f);
        ImGui::End();
    }
}

void MyImGui::toggleWireframe()
{
    // Wireframe mode
    if (bPolygonMode) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void MyImGui::render()
{
    // Render ImGui menus
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
