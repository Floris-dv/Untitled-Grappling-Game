#include "pch.h"

// OpenGL
#include <glad/glad.h>

// imgui
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// settings
#include "Settings.h"

#include "Setup.h"
// Camera::Get()
#include "Camera.h"

#include "Shadows.h"

// for Framebuffers setup, also sets framebuffer_size_callback:
#include "Framebuffers.h"

#include <DebugBreak.h>

#include "Log.h"

#include "Timer.h"

#include "Window.h"

// to get turning right:
float lastX, lastY;

bool paused = false;
static bool FirstFrame = true;
bool GammaCorrection = true;
bool fullScreen = false;

// to get to movement speed right
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// to give framerate, is the summed up deltatime of the last 40 frames, in seconds
float dTime40 = 0.0f;

float now;

// different as else you don't get smooth movement, and let diagonal in
static void updateMovement() {
	if (ImGui::IsWindowFocused(4) || ImGui::IsAnyItemFocused()) {
		Camera::Get().UpdateCameraVectors(deltaTime); // high FPS: doesn't matter, low FPS: disables dropping inputs (because friction is too large)
		return;
	}

	if (Window::Get().GetKeyPressed(KEY_W) || Window::Get().GetKeyPressed(KEY_UP))
		Camera::Get().ProcessKeyboard(Camera_Movement::FORWARD, deltaTime);
	if (Window::Get().GetKeyPressed(KEY_A) || Window::Get().GetKeyPressed(KEY_LEFT))
		Camera::Get().ProcessKeyboard(Camera_Movement::LEFT, deltaTime);
	if (Window::Get().GetKeyPressed(KEY_S) || Window::Get().GetKeyPressed(KEY_DOWN))
		Camera::Get().ProcessKeyboard(Camera_Movement::BACKWARD, deltaTime);
	if (Window::Get().GetKeyPressed(KEY_D) || Window::Get().GetKeyPressed(KEY_RIGHT))
		Camera::Get().ProcessKeyboard(Camera_Movement::RIGHT, deltaTime);
	if (Window::Get().GetKeyPressed(KEY_SPACE))
		Camera::Get().ProcessKeyboard(Camera_Movement::UP, deltaTime);

	Camera::Get().UpdateCameraVectors(deltaTime); // high FPS: doesn't matter, low FPS: disables dropping inputs (because friction is too large)

	return;
}

void Setup() {
	PROFILE_FUNCTION_ONCE();

	Window::Initialize({});

	Window::Get().GetFunctions().CursorPosFn = [](float fxpos, float fypos) {// to not flicker on the first frame, as the mouse is almost never exactly in the center
		if (FirstFrame) {
			lastX = fxpos;
			lastY = fypos;
			FirstFrame = false;
			return;
		}

		if (paused) {// do nothing if paused, only update lastX & lastY, to not get a massive snap
			// if the left mouse button is pressed: continue what you were doing
			if (Window::Get().GetMouseButtonDown(0)) {
				// unless over an ImGUI window or its items (those are counted seperately when focused)
				// if (!(ImGui::IsWindowFocused(4) || ImGui::IsAnyItemFocused())) // 4 means: is any window hovered
				goto out;  // I know this isn't recommended, but if I avoid it it makes for some ugly code
			}

			lastX = fxpos;
			lastY = fypos;
			return;
		}
	out:
		const float xoffset = fxpos - lastX;
		const float yoffset = lastY - fypos; // reversed since y-coordinates range from bottom to top
		lastX = fxpos;
		lastY = fypos;

		Camera::Get().ProcessMouseMovement(xoffset, yoffset);
	};

	Window::Get().GetFunctions().KeyPressFn = [](Key key, int scanCode, Action action, int mods) {
		if (action == Action::PRESS) {
			switch (key) {
			case KEY_Q:
				Window::Get().SetShouldClose(true);  return;
			case KEY_ESCAPE:
				paused = !paused;
				Window::Get().SetCursor(paused);
				return;
			case KEY_E:
				GammaCorrection = !GammaCorrection;
				if (!GammaCorrection)
					glDisable(GL_FRAMEBUFFER_SRGB);
				else
					glEnable(GL_FRAMEBUFFER_SRGB);
				return;
			}
		}
	};

	Window::Get().GetFunctions().ScrollFn = [](float xOffset, float yOffset) {
		Camera::Get().ProcessMouseScroll(xOffset, yOffset);
	};

	Framebuffers::Setup();

	Shadows::Setup();

	Camera::Initialize((float)Window::Get().GetWidth() / (float)Window::Get().GetHeight(), 0.1f, 1000.0f, 80.0f);
}

void Destroy() {
	// terminate and leave it clean
	Framebuffers::Delete();
	Window::Destroy();
}

void StartFrame() {
	now = Window::Get().GetTime(); // returns in seconds

	// movement:
	{
		deltaTime = now - lastFrame;
		lastFrame = now;
		if (dTime40 == 0.0f)
			dTime40 = 40.0f * deltaTime;
		else {
			dTime40 *= 39.0f / 40.0f; // make it 39 the dTime
			dTime40 += deltaTime; // make it back 40
		}
	}

	// start new ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::Text("Starting frame: %f", deltaTime);

	ImGui::DragFloat("k", &Camera::Get().k, 0.01f, 0.0f, 25.0f);
	updateMovement();
}

void EndFrame() {
	// Render ImGui
	std::string frameRate = std::to_string((int)std::round(1.0f / dTime40 * 40.0f));
	ImGui::Text(frameRate.c_str());
	ImGui::Text("Frame took %f ms", dTime40 / 40.0f * 1000.0f);

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}

	Window::Get().PollSwap();
}