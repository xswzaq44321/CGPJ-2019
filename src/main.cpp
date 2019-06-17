
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
#include <future>
#include "SPHSimulation_2.h"
#include "particlePlacements.h"
#include <opencv2/opencv.hpp>
#define BALL_AMOUNT s.particleNum
#define PI 3.1415926f
#define RAD2DEG (180.0f / PI)
#define DEG2RAD (PI / 180.0f)
#define GRAVITY 0.98f
#define CREATE_PNG 0

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

void outputPng(const char *filename, const std::vector<unsigned char> &raw_data, int width, int height)
{
	std::vector<unsigned char> raw_data2(width * height * 4);
	for (int i = 0; i < height; ++i) {
		std::copy(raw_data.begin() + i * width * 4, raw_data.begin() + (i + 1) * width * 4, raw_data2.begin() + (height - 1 - i) * width * 4);
	}
	lodepng::encode(filename, raw_data2, width, height);
}

template <typename T>
bool future_is_ready(std::future<T> &t)
{
	return t.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

int main(void)
{
    cv::Mat src2 = cv::imread("maps/normal6.png");
    cv::Mat dst2;
    cv::bilateralFilter(src2, dst2, 5, 250, 250);
    cv::imshow("origin2", src2);
    cv::imshow("bilateralBlur", dst2);

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
	glfwSwapInterval(0);
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
	ImVec4 transparent_color = ImVec4(1.0f, 1.0f, 1.0f, 0.0f);
	float myNear = 0.1f, myFar = 100.0f;
	auto text = Texture2D::LoadFromFile("../resource/face.png");
	auto mesh = StaticMesh::LoadMesh("../resource/sphere.obj");
	auto prog = Program::LoadFromFile(
		"../resource/vs_rect.vert",
		"../resource/fs_rect.frag");
	auto progLight = Program::LoadFromFile(
		"../resource/vs.vert",
		"../resource/gs.geom",
		"../resource/fs_light.frag");
	auto prog_normal = Program::LoadFromFile(
		"../resource/vs_instanced_viewspace.vert",
		"../resource/gs.geom",
		"../resource/fs_normal.frag");
	auto prog_depth = Program::LoadFromFile(
		"../resource/vs_instanced_viewspace.vert",
		"../resource/gs.geom",
		"../resource/fs_depth.frag");
	GLuint fbo;
	SystemState s;
	setInitial(centerBox, &s);  // On here, all ball position saved in
	getAcceleration(&s);
	leapFrogStart(&s, Params::timeStep);

	std::vector<glm::vec3> position(BALL_AMOUNT, glm::vec3(0));

	assignValue(position, s.particleNum, s.position);

	std::cout << BALL_AMOUNT << std::endl;

	// Do not remove {}, why???
	{
		// text and mesh, shader => garbage collector
		auto g1 = Protect(text);
		auto g2 = Protect(mesh);
		auto g3 = Protect(prog);
		auto gn = Protect(prog_normal);
		auto gd = Protect(prog_depth);

		if (!mesh.hasNormal() || !mesh.hasUV())
		{
			std::cerr << "Mesh does not have normal or uv\n";
			glfwSetWindowShouldClose(window, GLFW_TRUE);
		}

		float degree = 0.0f;
		glm::vec3 object_color{ 1.0f };
		static clock_t clockCount;
		bool flat_shading = false;
		bool bling_phong = false;
		glm::vec3 light_pos = { 0.0f, 10.0f, 0.0f };

		{
			glGenFramebuffers(1, &fbo);
			glBindFramebuffer(GL_FRAMEBUFFER, fbo);

			GLuint texture;
			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1280, 720, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glBindTexture(GL_TEXTURE_2D, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

			GLuint rbo;
			glGenRenderbuffers(1, &rbo);
			glBindRenderbuffer(GL_RENDERBUFFER, rbo);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 1280, 720);
			glBindRenderbuffer(GL_RENDERBUFFER, 0);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
			{
				// frame buffer generation success
			}
			else
			{
				std::cerr << "Frame Buffer Generation failed" << std::endl;
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

#if CREATE_PNG == 1
		std::vector<std::future<void>> threadVec;
		threadVec.reserve(10);
#endif
		while (!glfwWindowShouldClose(window))
		{
			double deltaTime = (double)(clock() - clockCount) / CLOCKS_PER_SEC;
			clockCount = clock();
			// std::cout << deltaTime << std::endl;
			degree += 0;
			for (int temp = 0; temp < Params::steps; ++temp) {
				getAcceleration(&s);
				leapFrogStep(&s, Params::timeStep);
			}
			assignValue(position, s.particleNum, s.position);

			int display_w, display_h;
			glfwGetFramebufferSize(window, &display_w, &display_h);
			static std::vector<unsigned char> raw_data_normal(display_w * display_h * 4);
			static std::vector<unsigned char> raw_data_depth(display_w * display_h * 4);

			static glm::mat4 vs = glm::lookAt(glm::vec3{ 30, 10, 23 }, glm::vec3{ 0, 5, 0 }, glm::vec3{ 0, 1, 0 });
			static glm::mat4 vp = glm::perspective(45 / 180.0f * 3.1415926f, 1280.0f / 720.0f, 0.1f, 10000.0f) * vs;
			{ // drawing normal map
				glBindFramebuffer(GL_FRAMEBUFFER, fbo);
				glViewport(0, 0, display_w, display_h);
				glClearColor(transparent_color.x, transparent_color.y, transparent_color.z, transparent_color.w);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				glEnable(GL_DEPTH_TEST);

				prog_normal["vp"] = vp;
				prog_normal["vs"] = vs;
				prog_normal["near"] = myNear;
				prog_normal["far"] = myFar;
				prog_normal["model"] = glm::rotate(glm::mat4(1.0f), degree * 3.1415926f / 180.0f, glm::vec3(0, 1, 0)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.3f));

				mesh.LoadInstancedArrays(position);
				prog_normal.use();
				mesh.drawInstanced(position.size());

				// make sure vector has enouth space to store
				raw_data_normal.reserve(display_w * display_h * 4);
				glReadPixels(0, 0, display_w, display_h, GL_RGBA, GL_UNSIGNED_BYTE, raw_data_normal.data());

				glClearColor(transparent_color.x, transparent_color.y, transparent_color.z, transparent_color.w);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				prog_depth["vp"] = vp;
				prog_depth["vs"] = vs;
				prog_depth["near"] = myNear;
				prog_depth["far"] = myFar;
				prog_depth["model"] = glm::rotate(glm::mat4(1.0f), degree * 3.1415926f / 180.0f, glm::vec3(0, 1, 0)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.3f));

				prog_depth.use();
				mesh.drawInstanced(position.size());

				raw_data_depth.reserve(display_w * display_h * 4);
				glReadPixels(0, 0, display_w, display_h, GL_RGBA, GL_UNSIGNED_BYTE, raw_data_depth.data());

				glDisable(GL_DEPTH_TEST);
				glBindFramebuffer(GL_FRAMEBUFFER, 0);

#if CREATE_PNG == 1
				static double timePassed = 0;
				timePassed += deltaTime;
				if (timePassed > 0.5)
				{
					timePassed = 0;

					char temp[100];
					static int fileCount = 0;
					sprintf(temp, "maps/normal%d.png", fileCount);
					threadVec.push_back(std::async(std::launch::async, outputPng, temp, raw_data_normal, display_w, display_h));
					for (int i = 0; i < threadVec.size(); ++i)
					{
						if (future_is_ready(threadVec[i]))
						{
							threadVec.erase(threadVec.begin() + i);
							--i;
						}
					}
					char temp2[100];
					sprintf(temp2, "maps/depth%d.png", fileCount++);
					threadVec.push_back(std::async(std::launch::async, outputPng, temp2, raw_data_depth, display_w, display_h));
					for (int i = 0; i < threadVec.size(); ++i)
					{
						if (future_is_ready(threadVec[i]))
						{
							threadVec.erase(threadVec.begin() + i);
							--i;
						}
					}
				}
#endif // CREATE_PNG
			}
			glViewport(0, 0, display_w, display_h);
			glEnable(GL_DEPTH_TEST);

			glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glEnable(GL_DEPTH_TEST);
			if(true){
				float vertices[] = {
					// position			// texture coords
					1.0f, 1.0f, 0.0f,	1.0f, 1.0f,	// top right
					1.0f, -1.0f, 0.0f,	1.0f, 0.0f,	// bottom right
					-1.0f, -1.0f, 0.0f,	0.0f, 0.0f,	// bottom left
					-1.0f, 1.0f, 0.0f,	0.0f, 1.0f	// top left
				};
				unsigned int indices[] = {
					0, 1, 3, // first triangle
					1, 2, 3  // second triangle
				};

				static unsigned int VBO = 0, VAO = 0, EBO = 0;
				if (VAO == 0)
					glGenVertexArrays(1, &VAO);
				if (VBO == 0)
					glGenBuffers(1, &VBO);
				if (EBO == 0)
					glGenBuffers(1, &EBO);

				glBindVertexArray(VAO);

				glBindBuffer(GL_ARRAY_BUFFER, VBO);
				glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

				// position attribute
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
				glEnableVertexAttribArray(0);

				// texture coord attribute
				glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
				glEnableVertexAttribArray(1);

				static unsigned int texture1 = 0, texture2 = 0;
				if (texture1 == 0)
					glGenTextures(1, &texture1);
				glBindTexture(GL_TEXTURE_2D, texture1); // all upcoming GL_TEXTURE_2D operations now have effect on this texture object
				// set the texture wrapping parameters
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				// set texture filtering parameters
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				// load image, create texture and generate mipmaps
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, display_w, display_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, raw_data_depth.data());
				glGenerateMipmap(GL_TEXTURE_2D);

				if (texture2 == 0)
					glGenTextures(1, &texture2);
				glBindTexture(GL_TEXTURE_2D, texture2);
				// set the texture wrapping parameters
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				// set texture filtering parameters
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, display_w, display_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, raw_data_normal.data());
				glGenerateMipmap(GL_TEXTURE_2D);

				// note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
				glBindBuffer(GL_ARRAY_BUFFER, 0);

				// You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
				// VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
				glBindVertexArray(0);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, texture1);
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, texture2);
				prog["texture1"] = 0;
				prog["texture2"] = 1;
				prog["light_pos"] = light_pos;
				prog["vs"] = vs;
				prog["inv"] = glm::inverse(vp);
				prog["ww"] = display_w;
				prog["wh"] = display_h;
				prog["near"] = myNear;
				prog["far"] = myFar;
				prog["bling_phong"] = static_cast<int>(bling_phong);
				prog.use();
				//mesh.drawInstanced(position.size());
				glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			}

			if (true) {
				progLight["vp"] = vp;
				progLight["model"] = glm::translate(glm::mat4(1.0f), light_pos) * glm::scale(glm::mat4(1.0f), glm::vec3{ 0.2f });

				progLight.use();
				mesh.draw();
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

				ImGui::SliderFloat("degree", &degree, 0.0f, 360.0f);			 // Edit 1 float using a slider from 0.0f to 1.0f
				ImGui::ColorEdit3("clear color", (float *)&clear_color);		 // Edit 3 floats representing a color
				ImGui::ColorEdit3("object color", glm::value_ptr(object_color)); // Edit 3 floats representing a color
				ImGui::SliderFloat3("Position", glm::value_ptr(light_pos), -10, 10);
				ImGui::SliderFloat("near", &myNear, 0.0f, 10.0f);			 // Edit 1 float using a slider from 0.0f to 1.0f
				ImGui::SliderFloat("far", &myFar, 1.0f, 100.0f);			 // Edit 1 float using a slider from 0.0f to 1.0f

				if (ImGui::Button("Button")) // Buttons return true when clicked (most widgets return true when edited/activated)
					counter++;
				ImGui::SameLine();
				ImGui::Text("counter = %d", counter);
				ImGui::Checkbox("Flat Shading", &flat_shading);
				ImGui::Checkbox("Bling Phong", &bling_phong);
				ImGui::Image(reinterpret_cast<ImTextureID>(text.id()), ImVec2{ 128, 128 });
				//ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
				ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", deltaTime, 1 / deltaTime);
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

#if CREATE_PNG == 1
		while (threadVec.size() != 0) {
			for (int i = 0; i < threadVec.size(); ++i)
			{
				if (future_is_ready(threadVec[i]))
				{
					threadVec.erase(threadVec.begin() + i);
					--i;
				}
			}
		}
#endif
		glDeleteFramebuffers(1, &fbo);
	}
	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}
