#include "pch.h"
#include "Log.h" // Before everything else because it includes minwindef.h, and that redefines the APIENTRY macro, and all other files have a check with #ifndef APIENTRY

// OpenGL
#include <glad/glad.h>

// imgui
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include <imguizmo/ImGuizmo.h>

// settings
#include "Settings.h"

#include "Setup.h"
#include "Camera.h"
#include "GrapplingCamera.h"

// for Framebuffers setup, also sets framebuffer_size_callback:
#include "Framebuffers.h"

#include <DebugBreak.h>


#include "Timer.h"

#include "Window.h"

// to get turning right:
float lastX, lastY;

bool paused = false;
static bool FirstFrame = true;
bool GammaCorrection = true;
bool fullScreen = false;

// different as else you don't get smooth movement, and let diagonal in
static void updateMovement(float deltaTime) {
	if (ImGui::IsWindowFocused(4) || ImGui::IsAnyItemFocused()) {
		Camera::Get()->UpdateCameraVectors(deltaTime);
		return;
	}

	unsigned int movement = Camera::MOVEMENT_NONE;
	movement |= (Window::Get().GetKeyPressed(KEY_W) || Window::Get().GetKeyPressed(KEY_UP)) * Camera::MOVEMENT_FORWARD;
	movement |= (Window::Get().GetKeyPressed(KEY_A) || Window::Get().GetKeyPressed(KEY_LEFT)) * Camera::MOVEMENT_LEFT;
	movement |= (Window::Get().GetKeyPressed(KEY_S) || Window::Get().GetKeyPressed(KEY_DOWN)) * Camera::MOVEMENT_BACKWARD;
	movement |= (Window::Get().GetKeyPressed(KEY_D) || Window::Get().GetKeyPressed(KEY_RIGHT)) * Camera::MOVEMENT_RIGHT;
	movement |= Window::Get().GetKeyPressed(KEY_SPACE) * Camera::MOVEMENT_UP;
	movement |= Window::Get().GetKeyPressed(KEY_LEFT_SHIFT) * Camera::MOVEMENT_DOWN;

	Camera::Get()->ProcessKeyboard((Camera::Camera_Movement)movement, deltaTime);
	Camera::Get()->UpdateCameraVectors(deltaTime);

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

		Camera::Get()->ProcessMouseMovement(xoffset, yoffset);
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
			}
		}
	};

	Window::Get().GetFunctions().ScrollFn = [](float xOffset, float yOffset) {
		Camera::Get()->ProcessMouseScroll(xOffset, yOffset);
	};

	Framebuffers::Setup();

	// Shadows::Setup();
}

void Destroy() {
	// terminate and leave it clean
	Framebuffers::Delete();
	Window::Destroy();
}

void StartFrame() {
	static float deltaTime = 0.0f;
	static float now = 0.0f;
	static float lastFrame = 0.0f;
	static float dTime40 = 0.0f;
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

	ImGuizmo::BeginFrame();

	ImGui::Text("Starting frame: %f", deltaTime);

	updateMovement(deltaTime);
}

void EndFrame() {
	// Render ImGui
	ImGui::Text("%f", ImGui::GetIO().Framerate);

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}

	Window::Get().PollSwap();
}