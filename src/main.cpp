
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
#include "lodepng.h"
#include <thread>
#define BALL_AMOUNT 100
#define PI 3.1415926f
#define RAD2DEG (180.0f / PI)
#define DEG2RAD (PI / 180.0f)
#define GRAVITY 0.98f

static void error_callback(int error, const char *description)
{
	std::cerr << fmt::format("Error: {0}\n", description);
}

void assignValue(std::vector<glm::vec3> &pos, int particleNum, float *position)
{
	for (int i = 0; i < particleNum; ++i)
	{
		pos[i].x = position[i * 3 + 0];
		pos[i].y = position[i * 3 + 1];
		pos[i].z = position[i * 3 + 2];
	}
}

void outputPng(const char *filename, std::vector<unsigned char> raw_data, int widht, int height)
{
	lodepng::encode(filename, raw_data, widht, height);
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
	auto prog = Program::LoadFromFile(
		"../resource/vs.vert",
		"../resource/gs.geom",
		"../resource/fs.frag");

	std::vector<glm::vec3> position;
	std::vector<glm::vec3> velocity(BALL_AMOUNT, glm::vec3(0));
	srand(1145141919810);
	for (int temp = 0; temp < BALL_AMOUNT; ++temp)
	{
		position.push_back(glm::vec3(rand() % 100 / 10.0 - 5, rand() % 100 / 10.0, rand() % 10 - 10));
	}

	// Do not remove {}, why???
	{
		// text and mesh, shader => garbage collector
		auto g1 = Protect(text);
		auto g2 = Protect(mesh);
		auto g3 = Protect(prog);

		if (!mesh.hasNormal() || !mesh.hasUV())
		{
			std::cerr << "Mesh does not have normal or uv\n";
			glfwSetWindowShouldClose(window, GLFW_TRUE);
		}

		float degree = 0.0f;
		glm::vec3 object_color{1.0f};
		static clock_t clockCount;
		bool flat_shading = false;
		glm::vec3 light_pos;
		while (!glfwWindowShouldClose(window))
		{
			double deltaTime = (double)(clock() - clockCount) / CLOCKS_PER_SEC;
			clockCount = clock();
			// std::cout << deltaTime << std::endl;
			degree += 360.0f * deltaTime;

			int display_w, display_h;
			glfwGetFramebufferSize(window, &display_w, &display_h);
			glViewport(0, 0, display_w, display_h);
			glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glEnable(GL_DEPTH_TEST);
			prog["vp"] = glm::perspective(45 / 180.0f * 3.1415926f, 16.0f / 9.0f, 0.1f, 10000.0f) *
						 glm::lookAt(glm::vec3{0, 0, 10}, glm::vec3{0, 0, 0}, glm::vec3{0, 1, 0});
			prog["model"] = glm::rotate(glm::mat4(1.0f), degree * 3.1415926f / 180.0f, glm::vec3(0, 1, 0));
			prog["object_color"] = object_color;
			prog["light_pos"] = light_pos;
			prog["eye_pos"] = glm::vec3{0, 0, 10};
			prog["text"] = 0;
			text.bindToChannel(0);
			prog.use();
			prog["flat_shading"] = static_cast<int>(flat_shading);
			for (int temp = 0; temp < position.size(); ++temp)
			{
				if (position[temp].y > -3)
					velocity[temp].y += GRAVITY * deltaTime;
				else
					velocity[temp] = {0, 0, 0};
				glm::vec3 deltaVelocity = velocity[temp];
				deltaVelocity *= deltaTime;
				position[temp] -= deltaVelocity;

				prog["model"] = glm::translate(glm::mat4(1.0f), position[temp]) * glm::rotate(glm::mat4(1.0f), degree * 3.1415926f / 180.0f, glm::vec3(0, 1, 0)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.3f));
				mesh.draw();
				//Hello World
			}
			glDisable(GL_DEPTH_TEST);

			{ // drawing normal map

				static bool once = true;
				static double timePassed = 0;
				static std::vector<std::shared_ptr<std::thread>> threadVec;
				timePassed += deltaTime;
				if (timePassed > 0.1)
				{
					timePassed = 0;
					GLuint fbo;
					glGenFramebuffers(1, &fbo);
					glBindFramebuffer(GL_FRAMEBUFFER, fbo);

					GLuint texture;
					glGenTextures(1, &texture);
					glBindTexture(GL_TEXTURE_2D, texture);
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, display_w, display_h, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glBindTexture(GL_TEXTURE_2D, 0);
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

					GLuint rbo;
					glGenRenderbuffers(1, &rbo);
					glBindRenderbuffer(GL_RENDERBUFFER, rbo);
					glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, display_w, display_h);
					glBindRenderbuffer(GL_RENDERBUFFER, 0);
					glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

					if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
						std::cout << "YES" << std::endl;
					else
						std::cout << "CRAP" << std::endl;

					glViewport(0, 0, display_w, display_h);
					glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
					glEnable(GL_DEPTH_TEST);
					auto prog_normal = Program::LoadFromFile(
						"../resource/vs.vert",
						"../resource/gs.geom",
						"../resource/fs_normal.frag");
					auto gg = Protect(prog_normal);

					prog_normal["vp"] = glm::perspective(45 / 180.0f * 3.1415926f, 16.0f / 9.0f, 0.1f, 10000.0f) *
										glm::lookAt(glm::vec3{0, 0, 10}, glm::vec3{0, 0, 0}, glm::vec3{0, 1, 0});
					prog_normal["model"] = glm::rotate(glm::mat4(1.0f), degree * 3.1415926f / 180.0f, glm::vec3(0, 1, 0));
					prog_normal["object_color"] = object_color;
					prog_normal["light_pos"] = light_pos;
					prog_normal["eye_pos"] = glm::vec3{0, 0, 10};
					prog_normal["text"] = 0;
					text.bindToChannel(0);
					prog_normal.use();
					prog_normal["flat_shading"] = static_cast<int>(flat_shading);
					for (int temp = 0; temp < position.size(); ++temp)
					{
						if (position[temp].y > -3)
							velocity[temp].y += GRAVITY * deltaTime;
						else
							velocity[temp] = {0, 0, 0};
						glm::vec3 deltaVelocity = velocity[temp];
						deltaVelocity *= deltaTime;
						position[temp] -= deltaVelocity;

						prog_normal["model"] = glm::translate(glm::mat4(1.0f), position[temp]) * glm::rotate(glm::mat4(1.0f), degree * 3.1415926f / 180.0f, glm::vec3(0, 1, 0)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.3f));
						mesh.draw();
						//Hello World
					}

					std::vector<unsigned char> raw_data(display_w * display_h * 4);
					glReadPixels(0, 0, display_w, display_h, GL_RGBA, GL_UNSIGNED_BYTE, raw_data.data());
					char temp[100];
					static int fileCount = 0;
					sprintf(temp, "normal%d.png", fileCount++);

					threadVec.push_back(std::shared_ptr<std::thread>(new std::thread(outputPng, temp, raw_data, display_w, display_h),
																	 [](std::thread *ptr) {
																		 ptr->join();
																		 std::cout << "Release thread\n";
																		 delete ptr;
																	 }));
					if(threadVec.size() > 10){
						threadVec.erase(threadVec.begin());
					}
					// outputPng(temp, raw_data, display_w, display_h);

					glDisable(GL_DEPTH_TEST);

					glBindFramebuffer(GL_FRAMEBUFFER, 0);
					glDeleteFramebuffers(1, &fbo);
					once = false;
				}
			}

			// Start the Dear ImGui frame
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			bool new_shader = false;
			// 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
			{
				static int counter = 0;

				ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and append into it.

				ImGui::SliderFloat("degree", &degree, 0.0f, 360.0f);			 // Edit 1 float using a slider from 0.0f to 1.0f
				ImGui::ColorEdit3("clear color", (float *)&clear_color);		 // Edit 3 floats representing a color
				ImGui::ColorEdit3("object color", glm::value_ptr(object_color)); // Edit 3 floats representing a color

				if (ImGui::Button("Button")) // Buttons return true when clicked (most widgets return true when edited/activated)
					counter++;
				ImGui::SameLine();
				ImGui::Text("counter = %d", counter);
				ImGui::Checkbox("Flat Shading", &flat_shading);
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
			glfwPollEvents();
		}
	}
	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}
