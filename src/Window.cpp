#include "DebugBreak.h"
#include "UtilityMacros.h"
#include "pch.h"
#ifdef _WIN32
#include <Windows.h>
#endif

DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATION
#include <glad/glad.h>
DISABLE_WARNING_POP

#include <GLFW/glfw3.h>

// imgui
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <imgui/imgui.h>

#include "Window.h"

#include "Log.h"
#include "Timer.h"

#if ENABLE_LOGGING
static void APIENTRY debug_message_callback(GLenum source, GLenum type,
                                            GLuint id, GLenum severity,
                                            GLsizei length,
                                            const GLchar *message,
                                            const void *userParam);
#endif

void Window::Init(const WindowProps &props) {
  PROFILE_FUNCTION_ONCE();
  NG_INFO("Creating window {0} ({1}, {2})", props.Title, props.Width,
          props.Height);
  if (!glfwInit())
    NG_CRITICAL("Could not initialize GLFW");

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_DEPTH_BITS, 32);

#if ENABLE_DEBUG_MESSAGE_CALLBACK
  glfwSetErrorCallback([](int error, const char *description) {
    NG_ERROR("GLFW Error ({0}): {1}", error, description);
  });
#endif

  m_Window = glfwCreateWindow(static_cast<int>(props.Width),
                              static_cast<int>(props.Height),
                              m_Props.Title.c_str(), nullptr, nullptr);

  glfwMakeContextCurrent(m_Window);
  glfwSetWindowAttrib(m_Window, GLFW_MOUSE_PASSTHROUGH, false);

  m_Props = props;
  m_Props.Functions.WindowResizeFn = []([[maybe_unused]] uint32_t width,
                                        [[maybe_unused]] uint32_t height) {};
  m_Props.WindowRef = this;

  glfwSetWindowUserPointer(m_Window, &m_Props);
  SetVSync(true);

#pragma region callbacks
  glfwSetCharCallback(m_Window, [](GLFWwindow *window, unsigned int codepoint) {
    WindowProps &data = *(WindowProps *)glfwGetWindowUserPointer(window);

    data.Functions.CharFn(codepoint);
  });

  glfwSetWindowCloseCallback(m_Window, [](GLFWwindow *window) {
    WindowProps &data = *(WindowProps *)glfwGetWindowUserPointer(window);

    data.Functions.CloseFn();
  });

  glfwSetCursorPosCallback(
      m_Window, [](GLFWwindow *window, double xPos, double yPos) {
        WindowProps &data = *(WindowProps *)glfwGetWindowUserPointer(window);

        data.Functions.CursorPosFn(static_cast<float>(xPos),
                                   static_cast<float>(yPos));
      });

  glfwSetKeyCallback(m_Window, [](GLFWwindow *window, int key, int scanCode,
                                  int action, int mods) {
    WindowProps &data = *(WindowProps *)glfwGetWindowUserPointer(window);

    data.Functions.KeyPressFn((Key)key, scanCode, (Action)action, mods);

    if (action != GLFW_RELEASE)
      return;

    auto range = data.WindowRef->GetKeyMap((Key)key);
    for (auto it = range.first; it != range.second; ++it) {
      it->second();
    }
  });

  glfwSetMouseButtonCallback(m_Window, [](GLFWwindow *window, int button,
                                          int action,
                                          [[maybe_unused]] int modifiers) {
    WindowProps &data = *(WindowProps *)glfwGetWindowUserPointer(window);

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
  });

  glfwSetScrollCallback(
      m_Window, [](GLFWwindow *window, double xOffset, double yOffset) {
        WindowProps &data = *(WindowProps *)glfwGetWindowUserPointer(window);

        data.Functions.ScrollFn(static_cast<float>(xOffset),
                                static_cast<float>(yOffset));
      });

  glfwSetFramebufferSizeCallback(
      m_Window, [](GLFWwindow *window, int width, int height) {
        bool isMinimized = false;
        while (width == 0 || height == 0) {
          isMinimized = true;
          glfwGetFramebufferSize(window, &width, &height);
          glfwWaitEvents();
        }
        uint32_t uWidth = static_cast<uint32_t>(width),
                 uHeight = static_cast<uint32_t>(height);

        if (isMinimized)
          return; // The new call to this callback will fix it: don't want a
                  // double overwrite

        WindowProps &data =
            *static_cast<WindowProps *>(glfwGetWindowUserPointer(window));

        data.Width = uWidth;
        data.Height = uHeight;

        data.Functions.WindowResizeFn(uWidth, uHeight);
      });

#pragma endregion

  SetupOpenGL();

  SetupImGui();
}

void Window::PollSwap() {
  glfwPollEvents();
  glfwSwapBuffers(m_Window);
}

void Window::SetKey(Key key, std::function<void(void)> function) {
  m_KeyMap.emplace(key, function);
}

void Window::SetVSync(bool vSync) {
  glfwSwapInterval(static_cast<int>(vSync));
  m_Props.VSync = vSync;
  return;
}

void Window::Maximize() { glfwMaximizeWindow(m_Window); }

void Window::SetCursor(bool enabled) {
  glfwSetInputMode(m_Window, GLFW_CURSOR,
                   enabled ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}

float Window::GetTime() const { return static_cast<float>(glfwGetTime()); }

void Window::ResetTime() { glfwSetTime(0.0); }

bool Window::ShouldClose() const { return glfwWindowShouldClose(m_Window); }

void Window::SetShouldClose(bool shouldClose) {
  glfwSetWindowShouldClose(m_Window, shouldClose);
}

bool Window::GetKeyPressed(Key key) {
  return glfwGetKey(m_Window, static_cast<int>(key)) == GLFW_PRESS;
}

bool Window::GetMouseButtonDown(int button) {
  return glfwGetMouseButton(m_Window, button);
}

void Window::SetupOpenGL() {
  // load GLAD
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    NG_CRITICAL("Failed to initialize GLAD");

#if ENABLE_DEBUG_MESSAGE_CALLBACK
  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  glDebugMessageCallback(debug_message_callback, nullptr);
  glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr,
                        GL_TRUE);
#endif

  // background color, change if you want
  glClearColor(0.0f, 0.0f, 0.1f, 1.0f);

  // enable Face culling
  glEnable(GL_CULL_FACE);
  // which faces are culled
  glCullFace(GL_BACK);
  // which are counted "front faces"
  glFrontFace(GL_CCW); // (counter-)clockwise winding

  // Enable depth testing buffer
  glEnable(GL_DEPTH_TEST);

  glDepthFunc(GL_ALWAYS);

#if CORRECT_GAMMA
  glEnable(GL_FRAMEBUFFER_SRGB);
#endif
}

void Window::SetupGLFW() {
  if (!glfwInit())
    NG_CRITICAL("Could not initialize GLFW");

#if USE_DEBUG_MESSAGE_CALLBACK
  glfwSetErrorCallback([](int error, const char *description) noexcept {
    NG_ERROR("GLFW Error ({0}): {1}", error, description);
  });
#endif

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_DEPTH_BITS, 32);
}

Window::~Window() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwTerminate();
}

void Window::SetupImGui() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGui::StyleColorsDark();
  ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

  // Setup platform things
  ImGui_ImplGlfw_InitForOpenGL(m_Window, true);
  ImGui_ImplOpenGL3_Init("#version 460");
}

#if ENABLE_DEBUG_MESSAGE_CALLBACK
static void APIENTRY
debug_message_callback(GLenum source, GLenum type, GLuint id, GLenum severity,
                       [[maybe_unused]] GLsizei length, const GLchar *message,
                       [[maybe_unused]] const void *userParam) {
  // userParam is always ignored

  const char *sourceName;
  const char *typeName;

  switch (source) {
  case GL_DEBUG_SOURCE_API:
    sourceName = "API";
    break;
  case GL_DEBUG_SOURCE_APPLICATION:
    sourceName = "APPLICATION";
    break;
  case GL_DEBUG_SOURCE_OTHER:
    sourceName = "OTHER";
    break;
  case GL_DEBUG_SOURCE_SHADER_COMPILER:
    sourceName = "SHADER COMPILER";
    break;
  case GL_DEBUG_SOURCE_THIRD_PARTY:
    sourceName = "THIRD PARTY";
    break;
  case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
    sourceName = "WINDOW SYSTEM";
    break;
  default:
    sourceName = "UNKNOWN";
    break;
  }

  switch (type) {
  case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
    typeName = "DEPRECATED_BEHAVIOR";
    break;
  case GL_DEBUG_TYPE_ERROR:
    typeName = "ERROR";
    break;
  case GL_DEBUG_TYPE_MARKER:
    typeName = "MARKER";
    break;
  case GL_DEBUG_TYPE_OTHER:
    typeName = "OTHER";
    break;
  case GL_DEBUG_TYPE_PERFORMANCE:
    typeName = "PERFORMANCE";
    break;
  case GL_DEBUG_TYPE_POP_GROUP:
    typeName = "POP_GROUP";
    break;
  case GL_DEBUG_TYPE_PORTABILITY:
    typeName = "PORTABILITY";
    break;
  case GL_DEBUG_TYPE_PUSH_GROUP:
    typeName = "PUSH_GROUP";
    break;
  case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
    typeName = "UNDEFINED_BEHAVIOR";
    break;
  default:
    typeName = "UNKNOWN";
    break;
  }

  switch (severity) {
  default:
    [[fallthrough]];
  case GL_DEBUG_SEVERITY_NOTIFICATION:
    NG_TRACE("OpenGL callback: source: {}; type: {}, ID: {}\nMessage: {}",
             sourceName, typeName, id, message);
    break;
  case GL_DEBUG_SEVERITY_LOW:
    NG_INFO("OpenGL callback: source: {}; type: {}, ID: {}\nMessage: {}",
            sourceName, typeName, id, message);
    break;
  case GL_DEBUG_SEVERITY_MEDIUM:
    NG_WARN("OpenGL callback: source: {}; type: {}, ID: {}\nMessage: {}",
            sourceName, typeName, id, message);
    break;
  case GL_DEBUG_SEVERITY_HIGH:
    NG_ERROR("OpenGL callback: source: {}; type: {}, ID: {}\nMessage: {}",
             sourceName, typeName, id, message);
    // debug_break();
    break;
  }
}
#endif
