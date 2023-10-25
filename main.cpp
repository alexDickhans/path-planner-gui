// Dear ImGui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include <stdio.h>
#include <fstream>
#include <iostream>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

ImVec2 add(ImVec2 a, ImVec2 b) {
	return {a.x + b.x, a.y + b.y};
}

ImVec2 minus(ImVec2 a, ImVec2 b) {
	return {a.x - b.x, a.y - b.y};
}

ImVec2 mult(ImVec2 a, double scalar) {
	return ImVec2(a.x * scalar, a.y * scalar);
}

float distance(ImVec2 a, ImVec2 b) {
	return sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2));
}

float convertToField(float x) {
	return x / 708.0 * 140.0;
}

class Spline {
private:
	ImVec2 points[4];

	bool isFocus[4]{false, false, false, false};

	std::string motionProfile{"nullptr"};

public:
	Spline(ImVec2 lastPos, ImVec2 lastArm, ImVec2 mousePos) {
		this->points[0] = lastPos;
		this->points[1] = add(lastPos, minus(lastPos, lastArm));
		this->points[2] = add(this->points[1], mult(minus(mousePos, this->points[1]), 0.5));
		this->points[3] = mousePos;
	}
	Spline(ImVec2 a, ImVec2 b, ImVec2 c, ImVec2 d) {
		this->points[0] = a;
		this->points[1] = b;
		this->points[2] = c;
		this->points[3] = d;
	}

	void printSpline(ImVec2 windowPosition) {
		ImGui::GetForegroundDrawList()->AddBezierCubic(add(points[0], windowPosition), add(points[1], windowPosition), add(points[2], windowPosition), add(points[3], windowPosition), IM_COL32(0, 0, 0, 225), 2);
		ImGui::GetForegroundDrawList()->AddLine(add(points[0], windowPosition), add(points[1], windowPosition), IM_COL32(10, 10, 10, 255), 3);
		ImGui::GetForegroundDrawList()->AddLine(add(points[2], windowPosition), add(points[3], windowPosition), IM_COL32(10, 10, 10, 255), 3);
		ImGui::GetForegroundDrawList()->AddCircleFilled(add(points[0], windowPosition), 8, IM_COL32(0, 255, 0, 255));
		ImGui::GetForegroundDrawList()->AddCircleFilled(add(points[1], windowPosition), 8, IM_COL32(0, 255, 0, 255));
		ImGui::GetForegroundDrawList()->AddCircleFilled(add(points[2], windowPosition), 10, IM_COL32(255, 0, 0, 255));
		ImGui::GetForegroundDrawList()->AddCircleFilled(add(points[3], windowPosition), 10, IM_COL32(255, 0, 0, 255));
	}

	bool processMouse(ImVec2 windowPosition, bool overallFocus) {

		for (auto &focus: isFocus) {
			overallFocus = focus || overallFocus;
		}

		for (int i = 0; i < 4; i++) {
			if (distance(minus(ImGui::GetMousePos(), windowPosition), points[i]) < 8 && ImGui::IsMouseDown(0) && !overallFocus) {
				isFocus[i] = true;
				overallFocus = true;
			} else if (!ImGui::IsMouseDown(0)) {
				isFocus[i] = false;
			}

			if (isFocus[i]) {
				points[i] = minus(ImGui::GetMousePos(), windowPosition);
			}
		}

		return overallFocus;
	}

	ImVec2& get(int i) {
		return points[i];
	}

	Spline convertEntireToField(ImVec2 windowPosition) {
		return {ImVec2(convertToField(points[0].y), convertToField(points[0].x)),
		ImVec2(convertToField(points[1].y), convertToField(points[1].x)),
		ImVec2(convertToField(points[2].y), convertToField(points[2].x)),
		ImVec2(convertToField(points[3].y), convertToField(points[3].x))};
	}

	std::string getMotionProfiling() {
		return motionProfile;
	}

	std::string* getMPPointer() {
		return &motionProfile;
	}
};

// Simple helper function to load an image into a OpenGL texture with common settings
bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height)
{
	// Load from file
	int image_width = 0;
	int image_height = 0;
	unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
	if (image_data == NULL)
		return false;

	// Create a OpenGL texture identifier
	GLuint image_texture;
	glGenTextures(1, &image_texture);
	glBindTexture(GL_TEXTURE_2D, image_texture);

	// Setup filtering parameters for display
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

	// Upload pixels into texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
	stbi_image_free(image_data);

	*out_texture = image_texture;
	*out_width = image_width;
	*out_height = image_height;

	return true;
}

static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

void save(std::vector<Spline> path, ImVec2 windowPosition, std::string filename, std::string name) {
	std::vector<Spline> convertedPath;

	for (auto &item: path) {
		convertedPath.emplace_back(item.convertEntireToField(windowPosition));
	}

	std::ofstream file(filename);

	file.clear();

	file << "#pragma once" << std::endl;
	file << "#include <vector>" << std::endl;

	file << "std::vector<std::pair<PathPlanner::BezierSegment, Pronounce::SinusoidalVelocityProfile*>> " << name << " = " << "{";

	for (auto &item: convertedPath) {
		file << "{PathPlanner::BezierSegment(";
		for (int i = 0; i < 4; i++) {
			file << "PathPlanner::Point(" << item.get(i).x << "_in, " << item.get(i).y << "_in)";
			if (i != 3) {
				file << ",";
			}
		}
		file << ")," << std::endl << item.getMotionProfiling() << "}," << std::endl;
	}

	file << "};" << std::endl;

	file << "// PathPlanner made path" << std::endl;

	file.close();
}

// Main code
int main(int, char**)
{
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		return 1;

	const char* glsl_version = "#version 130";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
//	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only

	// Create window with graphics context
	GLFWwindow* window = glfwCreateWindow(1920, 1080, "Path planner", nullptr, nullptr);
	if (window == nullptr)
		return 1;
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable vsync

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigWindowsMoveFromTitleBarOnly = true;

	// Setup Dear ImGui style
//	ImGui::StyleColorsDark();
	ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	int my_image_width = 0;
	int my_image_height = 0;
	GLuint my_image_texture = 0;
	bool ret = LoadTextureFromFile("./images/field.png", &my_image_texture, &my_image_width, &my_image_height);
	IM_ASSERT(ret);

	// Our state
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	std::vector<Spline> splines = {Spline(ImVec2(400, 50), ImVec2(700, 50), ImVec2(50, 200))};

	while (!glfwWindowShouldClose(window))
	{
		// Poll and handle events (inputs, window resize, etc.)
		// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
		// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
		// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
		// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
		glfwPollEvents();

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Field");
		ImGui::Image((void*)(intptr_t)my_image_texture, ImVec2(my_image_width, my_image_height));
		ImVec2 windowPosition = minus(ImGui::GetWindowPos(), ImVec2(-10.0, -30.0));
		ImGui::End();

		// 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
		{
			static float f = 0.0f;
			static int counter = 0;

			ImGui::Begin(
					"Hello, world!", NULL, ImGuiWindowFlags_MenuBar);                          // Create a window called "Hello, world!" and append into it.

			if (ImGui::BeginMenuBar())
			{
				if (ImGui::BeginMenu("File"))
				{
					if (ImGui::MenuItem("Open..", "Ctrl+O")) { /* Do stuff */ }
					if (ImGui::MenuItem("Save", "Ctrl+S"))   { save(splines, windowPosition, "testFile.cpp", "path1"); }
					if (ImGui::MenuItem("New", "Ctrl+N"))  { /* Do stuff */ }
					ImGui::EndMenu();
				}
				ImGui::EndMenuBar();
			}

			ImGui::Text("X: %f, Y: %f", (ImGui::GetMousePos().y-windowPosition.y) / my_image_width * 140.0,
						(ImGui::GetMousePos().x-windowPosition.x) / my_image_height * 140.0);

			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
			ImGui::ColorEdit3("clear color", (float *) &clear_color); // Edit 3 floats representing a color

			if (ImGui::Button(
					"Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
				counter++;
			ImGui::SameLine();
			ImGui::Text("counter = %d", counter);

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
			ImGui::End();
		}

		bool isFocus = false;

		for (auto &item: splines) {
			isFocus = item.processMouse(windowPosition, isFocus) || isFocus;
		}

		if (ImGui::IsMouseDoubleClicked(1) && splines.size() > 1) {
			for (int i = 0; i < splines.size(); i++) {
				if (distance(splines.at(i).get(3), minus(ImGui::GetMousePos(), windowPosition)) < 8) {
					splines.erase(splines.begin() + i);
					break;
				}
			}
		}

		if (ImGui::IsMouseDoubleClicked(0) && !isFocus) {
			splines.emplace_back(splines.at(splines.size()-1).get(3), splines.at(splines.size()-1).get(2), minus(ImGui::GetMousePos(), windowPosition));
		}

		for (auto &item: splines) {
			item.printSpline(windowPosition);
		}

		// Rendering
		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}
#ifdef __EMSCRIPTEN__
	EMSCRIPTEN_MAINLOOP_END;
#endif

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}