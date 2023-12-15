#include "pch.h"

#include "Log.h"

#include "Application.h"
#include "Timer.h"
#include "VertexData.h"
#include <glm/gtc/integer.hpp>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <imgui/imgui.h>
#include <imguizmo/ImGuizmo.h>
#include <miniaudio/miniaudio.h>

void Application::Initialize() {
  NG_TRACE("Initializing application");
  PROFILE_FUNCTION_ONCE();

  m_Window.GetFunctions().WindowResizeFn =
      std::bind_front(&Application::WindowResizeCallback, this);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glCreateFramebuffers(1, &m_MainFramebuffer);

  glCreateRenderbuffers(1, &m_MainRenderbuffer);

  FillMainRenderBufferAndTexture();

  const float screenVertices[] = {// positions   // texCoords
                                  -1.0f, 1.0f, 0.0f, 1.0f,  -1.0f, -1.0f,
                                  0.0f,  0.0f, 1.0f, -1.0f, 1.0f,  0.0f,

                                  -1.0f, 1.0f, 0.0f, 1.0f,  1.0f,  -1.0f,
                                  1.0f,  0.0f, 1.0f, 1.0f,  1.0f,  1.0f};
  m_ScreenVAO = VertexArray();
  m_ScreenVBO = VertexBuffer(sizeof(screenVertices), screenVertices);
  BufferLayout bl;
  bl.Push<float>(2); // position
  bl.Push<float>(2); // texCoords
  m_ScreenVAO.AddBuffer(m_ScreenVBO, bl);

  std::construct_at(&m_PostprocessingLayer, m_Window.GetWidth(),
                    m_Window.GetHeight(), &m_ScreenVAO);

  m_PostprocessingLayer.SetBloomSettings(m_BloomSettings);

  m_NormalShader =
      Shader("src/Shaders/NoInstance.vert", "src/Shaders/MainFrag.frag");
  m_InstancedShader =
      Shader("src/Shaders/Instanced.vert", "src/Shaders/MainFrag.frag");
  m_TextureShader =
      Shader("src/Shaders/Crosshair.vert", "src/Shaders/Crosshair.frag");

  m_LightsUBO = UniformBuffer(lightData.size() * sizeof(lightData[0]), "Lights",
                              lightData.data());

  m_LightsUBO.SetBlock(m_NormalShader);
  m_LightsUBO.SetBlock(m_InstancedShader);

  // TODO: remove this somehow
  const float far_plane = 2500.0f;

  // set shader uniforms
  {
    m_NormalShader.Use();
    m_NormalShader.SetFloat("material.shininess", 256.0f);
    m_NormalShader.SetFloat("spotLight.cutOff", 0.9762960071199334f);
    m_NormalShader.SetFloat("spotLight.outerCutOff", 0.9659258262890683f);
    m_NormalShader.SetFloat("far_plane", far_plane);

    m_InstancedShader.Use();
    m_InstancedShader.SetFloat("material.shininess", 256.0f);
    m_InstancedShader.SetFloat("spotLight.cutOff", 0.9762960071199334f);
    m_InstancedShader.SetFloat("spotLight.outerCutOff", 0.9659258262890683f);
    m_InstancedShader.SetFloat("far_plane", far_plane);
    m_InstancedShader.SetFloat("height_scale", 0.1f);
  }

  NG_TRACE("Initializing Game");

  std::construct_at(&m_Game, "Levels/Level1.dat", &m_InstancedShader, &m_NormalShader,
                &m_TextureShader, &m_Window);

  m_Game.InitializeCallbacks();
}

void Application::FillMainRenderBufferAndTexture() {
  // Need to create new one because glTextureStorage2D only allows for the
  // texture to be filled once, not refilled
  glCreateTextures(GL_TEXTURE_2D, 1, &m_MainTexture);

  // HDR texture
  glTextureStorage2D(m_MainTexture, 1, GL_RGBA16F, (GLsizei)m_Window.GetWidth(),
                     (GLsizei)m_Window.GetHeight());

  glTextureParameteri(m_MainTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTextureParameteri(m_MainTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTextureParameteri(m_MainTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTextureParameteri(m_MainTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glNamedFramebufferTexture(m_MainFramebuffer, GL_COLOR_ATTACHMENT0,
                            m_MainTexture,
                            0); // we only need a color buffer

  // Render buffer:
  glNamedRenderbufferStorage(m_MainRenderbuffer, GL_DEPTH24_STENCIL8,
                             (GLsizei)m_Window.GetWidth(),
                             (GLsizei)m_Window.GetHeight());
  glNamedFramebufferRenderbuffer(
      m_MainFramebuffer, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
      m_MainRenderbuffer); // If I don't do this I get an error

  auto status =
      glCheckNamedFramebufferStatus(m_MainRenderbuffer, GL_FRAMEBUFFER);

  if (status != GL_FRAMEBUFFER_COMPLETE)
    NG_ERROR("Framebuffer incomplete: {}", status);
}

void Application::StartFrame() {
  glBindFramebuffer(GL_FRAMEBUFFER, m_MainFramebuffer);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);

  // start new ImGui frame
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  ImGuizmo::BeginFrame();
}

void Application::EndFrame() {
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
  }

  m_Window.PollSwap();
}

void Application::PostProcess() {
  m_PostprocessingLayer.Postprocess(m_MainTexture);
}

void Application::WindowResizeCallback(uint32_t width, uint32_t height) {
  NG_TRACE("Resizing window to {} {}", width, height);
  glViewport(0, 0, (GLsizei)width, (GLsizei)height);

  m_Game.WindowResizeCallback(width, height);
  m_PostprocessingLayer.OnWindowResize(width, height);

  glDeleteTextures(1, &m_MainTexture);
  FillMainRenderBufferAndTexture();
}

Application::Application()
    : m_ScreenVAO(false),
      m_Window(WindowProps{
          "Untitled Grappling Game"}) { /* TODO: Set bloom settings */
  m_BloomSettings = BloomSettings{0.8f, 0.05f, {1, 1}, 1, true};
  Initialize();
}

void Application::Mainloop() {
  while (!m_Window.ShouldClose()) {
    StartFrame();
    m_Game.Update();
    m_Game.Render();
    m_Game.Finalize();
    PostProcess();
    m_Game.DrawUI(&m_ScreenVAO);
    EndFrame();
  }
}

void Application::Cleanup() {
  glDeleteFramebuffers(1, &m_MainFramebuffer);
  glDeleteRenderbuffers(1, &m_MainRenderbuffer);
  glDeleteTextures(1, &m_MainTexture);
}
