#include "pch.h"
#ifdef _WIN32
#include <Windows.h>
#endif

#include <glad/glad.h>
#include <GLFW/glfw3.h>

// imgui
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include "Window.h"

#include "Log.h"
#include "Timer.h"

static void APIENTRY debug_message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);

Window* Window::s_Window;

void Window::Init(const WindowProps& props)
{
	PROFILE_FUNCTION_ONCE();
	NG_INFO("Creating window {0} ({1}, {2})", props.Title, props.Width, props.Height);
	if (!glfwInit())
		NG_CRITICAL("Could not initialize GLFW");

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_DEPTH_BITS, 32);

#if USE_DEBUG_MESSAGE_CALLBACK
	glfwSetErrorCallback([](int error, const char* description) {
		NG_ERROR("GLFW Error ({0}): {1}", error, description);
		});
#endif

	m_Window = glfwCreateWindow((int)props.Width, (int)props.Height, m_Props.Title.c_str(), nullptr, nullptr);

	glfwMakeContextCurrent(m_Window);

	m_Props = props;
	m_Props.Functions.WindowResizeFn = [](uint32_t width, uint32_t height) {};
	m_Props.WindowRef = this;

	glfwSetWindowUserPointer(m_Window, &m_Props);
	SetVSync(true);

#pragma region callbacks
	glfwSetCharCallback(m_Window, [](GLFWwindow* window, unsigned int codepoint) {
		WindowProps& data = *(WindowProps*)glfwGetWindowUserPointer(window);

		data.Functions.CharFn(codepoint);
		}
	);

	glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window) {
		WindowProps& data = *(WindowProps*)glfwGetWindowUserPointer(window);

		data.Functions.CloseFn();
		}
	);

	glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos) {
		WindowProps& data = *(WindowProps*)glfwGetWindowUserPointer(window);

		data.Functions.CursorPosFn((float)xPos, (float)yPos);
		}
	);

	glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scanCode, int action, int mods) {
		WindowProps& data = *(WindowProps*)glfwGetWindowUserPointer(window);

		data.Functions.KeyPressFn((Key)key, scanCode, (Action)action, mods);
		}
	);

	glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int modifiers) {
		WindowProps& data = *(WindowProps*)glfwGetWindowUserPointer(window);

		switch (action) {
		case GLFW_PRESS:
			data.Functions.PressMouseFn(button);
			break;
		case GLFW_RELEASE:
			data.Functions.ReleaseMouseFn(button);
			break;
		case GLFW_REPEAT:
			data.Functions.HeldMouseFn(button);
			break;
		}
		}
	);

	glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset) {
		WindowProps& data = *(WindowProps*)glfwGetWindowUserPointer(window);

		data.Functions.ScrollFn((float)xOffset, (float)yOffset);
		}
	);

	glfwSetFramebufferSizeCallback(m_Window, [](GLFWwindow* window, int width, int height) {
		bool isMinimized = false;
		while (width == 0 || height == 0) {
			isMinimized = true;
			glfwGetFramebufferSize(window, &width, &height);
			glfwWaitEvents();
		}

		if (isMinimized)
			return; // The new call to this callback will fix it: don't want a double overwrite

		WindowProps& data = *(WindowProps*)glfwGetWindowUserPointer(window);

		data.Width = width;
		data.Height = height;

		data.Functions.WindowResizeFn(width, height);
		}
	);

#pragma endregion

	SetupOpenGL();

	SetupImGui();
}

void Window::PollSwap()
{
	glfwPollEvents();
	glfwSwapBuffers(m_Window);
}

void Window::SetVSync(bool vSync)
{
	glfwSwapInterval((int)vSync);
	m_Props.VSync = vSync;
	return;
}

void Window::Maximize()
{
	glfwMaximizeWindow(m_Window);
}

void Window::SetCursor(bool enabled)
{
	glfwSetInputMode(m_Window, GLFW_CURSOR, enabled ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}

float Window::GetTime() const
{
	return (float)glfwGetTime();
}

void Window::ResetTime()
{
	glfwSetTime(0.0);
}

bool Window::ShouldClose() const
{
	return glfwWindowShouldClose(m_Window);
}

void Window::SetShouldClose(bool shouldClose)
{
	glfwSetWindowShouldClose(m_Window, shouldClose);
}

bool Window::GetKeyPressed(Key key)
{
	return (bool)glfwGetKey(m_Window, (int)key);
}

bool Window::GetMouseButtonDown(int button)
{
	return glfwGetMouseButton(m_Window, button);
}

void Window::SetupOpenGL()
{
	// load GLAD
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		NG_CRITICAL("Failed to initialize GLAD");

#if USE_DEBUG_MESSAGE_CALLBACK
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(debug_message_callback, nullptr);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
#endif

	// background color, change if you want
	glClearColor(0.2f, 0.3f, 0.4f, 1.0f);

	// enable Face culling
	glEnable(GL_CULL_FACE);
	// which faces are culled
	glCullFace(GL_BACK);
	// which are counted "front faces"
	glFrontFace(GL_CCW); // (counter-)clockwise winding

	// Enable depth testing buffer, if you stop this it look wonky
	glEnable(GL_DEPTH_TEST);

#if CORRECT_GAMMA
	glEnable(GL_FRAMEBUFFER_SRGB);
#endif
}

void Window::SetupGLFW()
{
	if (!glfwInit())
		NG_CRITICAL("Could not initialize GLFW");

#if USE_DEBUG_MESSAGE_CALLBACK
	glfwSetErrorCallback([](int error, const char* description) {
		NG_ERROR("GLFW Error ({0}): {1}", error, description);
		});
#endif

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_DEPTH_BITS, 32);
}

void Window::Destroy()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwTerminate();
	s_Window = nullptr;
}

void Window::SetupImGui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui::StyleColorsDark();
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	// Setup platform things
	ImGui_ImplGlfw_InitForOpenGL(m_Window, true);
	ImGui_ImplOpenGL3_Init("#version 460");
}

static void APIENTRY debug_message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	const char* sourceName;
	const char* typeName;

	switch (source) {
	case GL_DEBUG_SOURCE_API:				sourceName = "API";					   break;
	case GL_DEBUG_SOURCE_APPLICATION:		sourceName = "APPLICATION";			   break;
	case GL_DEBUG_SOURCE_OTHER:				sourceName = "OTHER";				   break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER:	sourceName = "SHADER COMPILER";		   break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:		sourceName = "THIRD PARTY";			   break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:		sourceName = "WINDOW SYSTEM";		   break;
	default:								sourceName = "UNKNOWN";				   break;
	}

	switch (type) {
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: typeName = "DEPRECATED_BEHAVIOR";	   break;
	case GL_DEBUG_TYPE_ERROR:				typeName = "ERROR";					   break;
	case GL_DEBUG_TYPE_MARKER:				typeName = "MARKER";				   break;
	case GL_DEBUG_TYPE_OTHER:				typeName = "OTHER";					   break;
	case GL_DEBUG_TYPE_PERFORMANCE:			typeName = "PERFORMANCE";			   break;
	case GL_DEBUG_TYPE_POP_GROUP:			typeName = "POP_GROUP";				   break;
	case GL_DEBUG_TYPE_PORTABILITY:			typeName = "PORTABILITY";			   break;
	case GL_DEBUG_TYPE_PUSH_GROUP:			typeName = "PUSH_GROUP";			   break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:	typeName = "UNDEFINED_BEHAVIOR";	   break;
	default:								typeName = "UNKNOWN";				   break;
	}

	switch (severity) {
	case GL_DEBUG_SEVERITY_NOTIFICATION:	NG_TRACE("OpenGL callback: source: {}; type: {}, ID: {}\nMessage: {}", sourceName, typeName, id, message);	break;
	case GL_DEBUG_SEVERITY_LOW:				NG_INFO("OpenGL callback: source: {}; type: {}, ID: {}\nMessage: {}", sourceName, typeName, id, message);	break;
	case GL_DEBUG_SEVERITY_MEDIUM:			NG_WARN("OpenGL callback: source: {}; type: {}, ID: {}\nMessage: {}", sourceName, typeName, id, message);	break;
	case GL_DEBUG_SEVERITY_HIGH:			NG_ERROR("OpenGL callback: source: {}; type: {}, ID: {}\nMessage: {}", sourceName, typeName, id, message);	break;
	}
}