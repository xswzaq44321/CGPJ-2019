
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cstdlib>
#include <vector>
#include <ctime>
#define GLM_FORCE_RADIAN
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fmt/format.h>
#include <imgui.h>
#include <memory>
#include "imgui_impl_opengl3.h"
#include "imgui_impl_glfw.h"
#include "WenCheng.h"
#define BALL_AMOUNT 100
#define PI 3.1415926f
#define RAD2DEG (180.0f / PI)
#define DEG2RAD (PI / 180.0f)

// IzsKon eat grass everyday!

static void error_callback(int error, const char *description)
{
    std::cerr << fmt::format("Error: {0}\n", description);
}

int main(void)
{
    GLFWwindow *window;
    glfwSetErrorCallback(error_callback);
    if (!glfwInit())
        exit(EXIT_FAILURE);
    // Good bye Mac OS X
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(1280, 720, "Simple example", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(window);
    // glfwSwapInterval(0);
    if (!gladLoadGL())
    {
        exit(EXIT_FAILURE);
    }
    // Setup Dear ImGui binding
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Setup style
    ImGui::StyleColorsDark();

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    auto text = Texture2D::LoadFromFile("../resource/face.png");
    auto mesh = StaticMesh::LoadMesh("../resource/sphere.obj");
    //auto uselessProg = Program::LoadFromFile("../resource/vs.vert", "../resource/fs.frag");

    std::vector<Program> prog;
    std::vector<glm::vec3> position;
    std::vector<glm::vec3> velocity(BALL_AMOUNT, glm::vec3(0));
    for (int temp = 0; temp < BALL_AMOUNT; ++temp)
    {
        prog.push_back(Program::LoadFromFile("../resource/vs.vert", "../resource/fs.frag"));
        position.push_back(glm::vec3(rand() % 100 / 10.0 - 5, rand() % 100 / 10.0, rand() % 10 - 10));
    }

    // Do not remove {}, why???
    {
        // text and mesh, shader => garbage collector
        auto g1 = Protect(text);
        auto g2 = Protect(mesh);
        auto foo = Protect(prog[0]);

        std::vector<decltype(foo)> g3;

        for (int temp = 0; temp < prog.size(); ++temp)
        {
            g3.push_back(Protect(prog[temp]));
        }
        //auto g3 = Protect(prog);

        if (!mesh.hasNormal() || !mesh.hasUV())
        {
            std::cerr << "Mesh does not have normal or uv\n";
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        float degree = 0.0f;
        glm::vec3 object_color{1.0f};
        srand(1145141919810);
        while (!glfwWindowShouldClose(window))
        {
            degree += 1.0f;
            glfwPollEvents();

            int display_w, display_h;
            glfwGetFramebufferSize(window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glEnable(GL_DEPTH_TEST);
            for (int temp = 0; temp < prog.size(); ++temp)
            {
                if(position[temp].y > -3) velocity[temp].y += 0.0000098f;
                else velocity[temp] = {0,0,0};
                position[temp] -= velocity[temp];
                
                prog[temp]["vp"] = glm::perspective(45 / 180.0f * 3.1415926f, 1280.0f / 720.0f, 0.1f, 10000.0f) *
                                   glm::lookAt(glm::vec3{0, 0, 10}, glm::vec3{0, 0, 0}, glm::vec3{0, 1, 0});
                prog[temp]["model"] = glm::translate(glm::mat4(1.0f), position[temp]) * glm::rotate(glm::mat4(1.0f), degree * 3.1415926f / 180.0f, glm::vec3(0, 1, 0)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.3f));
                prog[temp]["object_color"] = object_color;
                text.bindToChannel(0);
                prog[temp].use();
                mesh.draw();
                //Hello World
            }
            glDisable(GL_DEPTH_TEST);

            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            bool new_shader = false;
            // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
            {
                static int counter = 0;

                ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and append into it.

                ImGui::SliderFloat("degree", &degree, 0.0f, 360.0f);             // Edit 1 float using a slider from 0.0f to 1.0f
                ImGui::ColorEdit3("clear color", (float *)&clear_color);         // Edit 3 floats representing a color
                ImGui::ColorEdit3("object color", glm::value_ptr(object_color)); // Edit 3 floats representing a color

                if (ImGui::Button("Button")) // Buttons return true when clicked (most widgets return true when edited/activated)
                    counter++;
                ImGui::SameLine();
                ImGui::Text("counter = %d", counter);
                ImGui::Image(reinterpret_cast<ImTextureID>(text.id()), ImVec2{128, 128});
                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
                // if(ImGui::Button("Reload Shader")) {
                //     auto new_prog = Program::LoadFromFile("../resource/vs.vert", "../resource/fs.frag");
                //     // because we would like to switch value of prog
                //     // we need to release object on our own.
                //     prog.release();
                //     prog = new_prog;

                // }
                ImGui::End();
            }

            // Rendering
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(window);
        }
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}