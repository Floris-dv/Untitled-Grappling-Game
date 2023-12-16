#include "pch.h"

#include "EditingCamera.h"
#include "Endscreen.h"
#include "Game.h"
#include "Levels.h"
#include "Log.h"
#include "VertexData.h"
#include "Window.h"

#include <imgui/imgui.h>

Camera::Camera_Movement Game::GetMovement() {
  if (ImGui::IsWindowFocused(4) || ImGui::IsAnyItemFocused()) {
    return Camera::MOVEMENT_NONE;
  }

  unsigned int movement = Camera::MOVEMENT_NONE;
  movement |=
      (m_Window->GetKeyPressed(KEY_W) || m_Window->GetKeyPressed(KEY_UP)) *
      Camera::MOVEMENT_FORWARD;
  movement |=
      (m_Window->GetKeyPressed(KEY_A) || m_Window->GetKeyPressed(KEY_LEFT)) *
      Camera::MOVEMENT_LEFT;
  movement |=
      (m_Window->GetKeyPressed(KEY_S) || m_Window->GetKeyPressed(KEY_DOWN)) *
      Camera::MOVEMENT_BACKWARD;
  movement |=
      (m_Window->GetKeyPressed(KEY_D) || m_Window->GetKeyPressed(KEY_RIGHT)) *
      Camera::MOVEMENT_RIGHT;
  movement |= m_Window->GetKeyPressed(KEY_SPACE) * Camera::MOVEMENT_UP;
  movement |= (m_Window->GetKeyPressed(KEY_LEFT_SHIFT) ||
               m_Window->GetKeyPressed(KEY_RIGHT_SHIFT)) *
              Camera::MOVEMENT_DOWN;

  return (Camera::Camera_Movement)movement;
}

void Game::InitializeCallbacks() {
  m_Window->GetFunctions().CursorPosFn =
      [this](float fxpos,
             float fypos) { // to not flicker on the first frame, as the mouse
                            // is almost never exactly in the center
        static float lastX, lastY; // TODO: improve this
        static bool FirstFrame = true;
        if (FirstFrame) {
          lastX = fxpos;
          lastY = fypos;
          FirstFrame = false;
          return;
        }

        if (m_State == GameState::Paused || m_State == GameState::Endscreen) {
          // do nothing if paused, only update lastX & lastY, to not get a
          // massive snap if the left mouse button is pressed: continue what you
          // were doing
          if (m_Window->GetMouseButtonDown(0)) {
            // unless over an ImGUI window or its items (those are counted
            // seperately when focused)
            if (!(ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow) ||
                  ImGui::IsAnyItemFocused()))
              goto out; // I know this isn't recommended, but if I avoid it it
                        // makes for some ugly code
          }
          lastX = fxpos;
          lastY = fypos;
          return;
        }
      out:
        const float xoffset = fxpos - lastX;
        // reversed since y-coordinates range from bottom to top
        const float yoffset = lastY - fypos;
        lastX = fxpos;
        lastY = fypos;

        m_Camera->ProcessMouseMovement(xoffset, yoffset);
      };

  m_Window->GetFunctions().KeyPressFn =
      [this](Key key, [[maybe_unused]] int scanCode, Action action,
             [[maybe_unused]] int mods) {
        if (action == Action::PRESS) {
          switch (key) {
          case KEY_Q:
            m_Window->SetShouldClose(true);
            return;
          case KEY_ESCAPE:
            if (m_State == GameState::Paused) {
              m_State = GameState::Playing;
              m_Window->SetCursor(false);
            } else if (m_State == GameState::Playing) {
              m_State = GameState::Paused;
              m_Window->SetCursor(true);
            }
            return;
          case KEY_Z:
            switch (m_State) {
            case GameState::Editing:
              m_State = GameState::Playing;
              m_Camera = std::make_unique<GrapplingCamera>(
                  20.0f, 1.0f, m_Camera->Options, m_Camera->AspectRatio,
                  m_Camera->Position, glm::vec3(0.0f, 1.0, 0.0f),
                  m_Camera->GetEulerAngles()->x, m_Camera->GetEulerAngles()->y);
              break;
            case GameState::Playing:
              m_State = GameState::Editing;
              m_Camera = std::make_unique<EditingCamera>(
                  m_Camera->Options, m_Camera->AspectRatio, m_Camera->Position,
                  glm::vec3(0.0f, 1.0f, 0.0f), m_Camera->GetEulerAngles()->x,
                  m_Camera->GetEulerAngles()->y);

              break;
            case GameState::Endscreen:
            case GameState::Paused:
              break;
            }
          default:
            break;
          }
        }
      };
}

Game::Game(const std::string &startLevel, Shader *instancedShader,
           Shader *normalShader, Shader *textureShader, Window *window)
    : m_InstancedShader(instancedShader), m_NormalShader(normalShader),
      m_UIShader(textureShader), m_AudioSystem(nullptr), m_Window(window),
      m_MatrixUBO(sizeof(glm::mat4) + sizeof(glm::vec4), "Matrices"),
      m_Camera(std::make_unique<GrapplingCamera>(
          20.0f, 1.0f, Camera::CameraOptions{2.5f, 0.1f, 100.0f},
          m_Window->GetAspectRatio(), glm::vec3{0.0f},
          glm::vec3{0.0f, 1.0f, 0.0f}, 90.0f, 0.0f)),
      m_Level(startLevel, window) {
  m_CrosshairTexture = StartLoadingTexture(
      std::filesystem::path("resources/Textures/Crosshair.png"));
  m_AudioSystem.AddSound("resources/my_sound.wav",
                         m_Level.GetBlocks()[3].Start);

  Settings.CameraOptions = &m_Camera->Options;

  m_MatrixUBO.Bind();
  m_MatrixUBO.SetBlock(*m_NormalShader);
  m_MatrixUBO.SetBlock(*m_InstancedShader);

  m_UIShader->Use();
  m_UIShader->SetInt("crosshairTex", 7);

  auto [Vertices, Indices] = CreateCylinder(8, 2);
  m_Rope = Mesh<MinimalVertex>(std::span(Vertices), MinimalVertex::Layout,
                               std::span(Indices));
}

void Game::Update() {
  float time = m_Window->GetTime();
  float deltaTime = time - m_StartFrame;
  m_StartFrame = time;

  switch (m_State) {
  case GameState::Playing:
    m_Camera->ProcessKeyboard(GetMovement(), deltaTime);
    [[fallthrough]];

  case GameState::Paused:
    m_Camera->UpdateCameraVectors(deltaTime);

    if (m_Level.UpdatePhysics(*(GrapplingCamera *)m_Camera.get())) {
      m_State = GameState::Endscreen;
      m_Timer.Stop();
      m_Endscreen = Endscreen(
          "Level", std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::duration<double>(m_Timer.GetTime())));
      m_Window->SetCursor(true);

      m_Camera->Reset();
    }
    break;

  case GameState::Editing:
    m_Camera->ProcessKeyboard(GetMovement(), deltaTime);
    m_Camera->UpdateCameraVectors(deltaTime);
    break;

  case GameState::Endscreen:
    switch (m_Endscreen.GetState()) {
    case Endscreen::State::Next_Level:
      SetLevel(m_LevelNr + 1);
      m_Endscreen.Close();
      m_State = GameState::Playing;
      m_Window->SetCursor(false);
      break;

    case Endscreen::State::Restart:
      SetLevel(m_LevelNr);
      m_Endscreen.Close();
      m_State = GameState::Playing;
      m_Window->SetCursor(false);
      break;

    case Endscreen::State::Open:
      break;

    case Endscreen::State::Closed:
      NG_WARN("GameState is endscreen while endscreen is closed");
      break;
    }
    break;
  }
  m_AudioSystem.SetListenerOptions(m_Camera->Position, m_Camera->Front);
}

void Game::Render() {
  m_MatrixUBO.SetData(0, sizeof(glm::mat4),
                      glm::value_ptr(m_Camera->GetVPMatrix()));
  m_MatrixUBO.SetData(64, sizeof(glm::vec3),
                      glm::value_ptr(m_Camera->Position));

  switch (m_State) {
  case GameState::Paused:
    [[fallthrough]];
  case GameState::Playing:

    m_Level.Render(m_InstancedShader, m_NormalShader);
    {
      glm::mat4 modelMatrix(1.0f);
      m_Rope.Draw(modelMatrix, &m_Level.GetTheme(), m_NormalShader);
    }
    break;

  case GameState::Editing:
    m_Level.RenderEditingMode(m_BlockEditingIndex, m_Camera.get());
    m_Level.RenderOneByOne(m_NormalShader, m_NormalShader);
    break;

  case GameState::Endscreen:
    m_Level.Render(m_InstancedShader, m_NormalShader);
    m_Endscreen.Render();
    break;
  }
}

void Game::Finalize() {
  if (m_State == GameState::Playing) {
    if (m_Window->GetMouseButtonDown(1)) {
      if (((GrapplingCamera *)m_Camera.get())->IsGrappling())
        return;

      float depth;
      glReadPixels((int)m_Window->GetWidth() / 2,
                   (int)m_Window->GetHeight() / 2, 1, 1, GL_DEPTH_COMPONENT,
                   GL_FLOAT, &depth);

      float zNorm = 2.0f * depth - 1.0f;
      float zFar = m_Camera->Options.ZFar;
      float zNear = m_Camera->Options.ZNear;
      float zView =
          -2.0f * zNear * zFar / ((zFar - zNear) * zNorm - zNear - zFar);

      glm::vec3 pos = m_Camera->Position + m_Camera->Front * zView;

      if (zView < 80.0f)
        ((GrapplingCamera *)m_Camera.get())->LaunchAt(pos);

    } else {
      ((GrapplingCamera *)m_Camera.get())->Release();
    }
  }
}

void Game::DrawUI(VertexArray *screenVAO) {
  switch (m_State) {
  case GameState::Playing:
    ImGui::Text("Playing");
    if (m_CrosshairTexture.index())
      m_CrosshairTexture =
          std::get<LoadingTexture::Future>(m_CrosshairTexture).get()->Finish();

    ImGui::DragFloat("Crosshair size", &m_CrosshairSize, 0.01f, 0.0f, 0.5f);

    m_UIShader->Use();
    glBindTextureUnit(7, std::get<Texture>(m_CrosshairTexture).ID);

    m_UIShader->SetVec2("uSize", {m_CrosshairSize / m_Window->GetAspectRatio(),
                                  m_CrosshairSize});

    screenVAO->Bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
    screenVAO->UnBind();
    break;

  case GameState::Paused:
    ImGui::Text("Paused");
    break;
  case GameState::Editing:
    ImGui::Text("Editing");
    break;
  case GameState::Endscreen:
    break;
  }
}

void Game::SetLevel(int level) {
  m_LevelNr = level % NR_LEVELS;
  if (m_LevelNr == 0)
    m_LevelNr = 1;
  m_Level = Level(GetLevelByNr(m_LevelNr), m_Window);
  m_Camera->Reset();
}

void Game::WindowResizeCallback(uint32_t width, uint32_t height) {
  m_Camera->AspectRatio = (float)width / (float)height;
}
