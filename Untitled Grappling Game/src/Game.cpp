#include "pch.h"

#include "Bloom.h"
#include "Game.h"
#include "Levels.h"
#include "SaveFile.h"
#include "UtilityMacros.h"

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

// TODO: improve this
static bool paused;
static float lastX, lastY;
static bool FirstFrame = true;

void Game::InitializeCallbacks() {
  m_Window->GetFunctions().CursorPosFn =
      [this](float fxpos,
             float fypos) { // to not flicker on the first frame, as the mouse
                            // is almost never exactly in the center
        if (FirstFrame) {
          lastX = fxpos;
          lastY = fypos;
          FirstFrame = false;
          return;
        }

        if (paused) { // do nothing if paused, only update lastX & lastY, to not
                      // get a massive snap
          // if the left mouse button is pressed: continue what you were doing
          if (m_Window->GetMouseButtonDown(0)) {
            // unless over an ImGUI window or its items (those are counted
            // seperately when focused)
            if (!(ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow) ||
                  ImGui::IsAnyItemFocused())) // 4 means: is any window focused
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
            paused = !paused;
            m_Window->SetCursor(paused);
            return;
          default:
            break;
          }
        }
      };
}

Game::Game(const std::string &startLevel, Shader *instancedShader,
           Shader *normalShader, Shader *textureShader, Window *window)
    : m_InstancedShader(instancedShader), m_NormalShader(normalShader),
      m_UIShader(textureShader), m_AudioSystem(nullptr),
      m_Level(startLevel, window), m_Window(window),
      m_MatrixUBO(sizeof(glm::mat4) + sizeof(glm::vec4), "Matrices"),
      m_Camera(std::make_unique<GrapplingCamera>(
          20.0f, 1.0f, Camera::CameraOptions{2.5f, 0.1f, 100.0f},
          m_Window->GetAspectRatio(), glm::vec3{0.0f},
          glm::vec3{0.0f, 1.0f, 0.0f}, 90.0f, 0.0f)) {
  m_CrosshairTexture = StartLoadingTexture(
      std::filesystem::path("resources/Textures/Crosshair.png"));
  // m_AudioSystem.AddSound("resources/my_sound.wav",
  //                       m_Level.GetBlocks().back().Start);

  Settings.CameraOptions = &m_Camera->Options;

  m_MatrixUBO.Bind();
  m_MatrixUBO.SetBlock(*m_NormalShader);
  m_MatrixUBO.SetBlock(*m_InstancedShader);

  m_UIShader->Use();
  m_UIShader->SetInt("crosshairTex", 7);
}

void Game::swap(Game &other) noexcept {
  SWAP(m_InstancedShader);
  SWAP(m_NormalShader);
  SWAP(m_UIShader);
  SWAP(m_Timer);
  SWAP(m_State);
  SWAP(m_Level);
  SWAP(m_Window);
  SWAP(m_Endscreen);
  SWAP(m_StartTime);
  SWAP(m_StartFrame);
  SWAP(m_MatrixUBO);
  SWAP(m_Camera);
  SWAP(m_LevelNr);
  SWAP(m_BlockEditingIndex);
  SWAP(m_CrosshairTexture);
  SWAP(Settings);
  SWAP(m_AudioSystem);
}

void Game::Update() {
  float time = m_Window->GetTime();
  float deltaTime = time - m_StartFrame;
  m_StartFrame = time;

  switch (m_State) {
  case Game::GameState::Playing:
    m_Camera->ProcessKeyboard(GetMovement(), deltaTime);
    m_Camera->UpdateCameraVectors(deltaTime);

    if (m_Level.UpdatePhysics(*(GrapplingCamera *)m_Camera.get())) {
      m_State = GameState::Endscreen;
      m_Timer.Stop();
      m_Endscreen = Endscreen(
          "Level", std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::duration<double>(m_Timer.GetTime())));

      m_Camera->Reset();
    }
    break;

  case Game::GameState::Editing:
    m_Camera->ProcessKeyboard(GetMovement(), deltaTime);
    m_Camera->UpdateCameraVectors(deltaTime);
    break;

  case Game::GameState::Endscreen:
    switch (m_Endscreen.GetState()) {
    case Endscreen::State::Next_Level:
      SetLevel(m_LevelNr + 1);
      m_Endscreen.Close();
      m_State = GameState::Playing;
      break;

    case Endscreen::State::Restart:
      SetLevel(m_LevelNr);
      m_Endscreen.Close();
      m_State = GameState::Playing;
      break;
    default:
      break;
    }
    break;
  }
  // m_AudioSystem.SetListenerOptions(m_Camera->Position, m_Camera->Front);
}

void Game::Render() {
  m_MatrixUBO.SetData(0, sizeof(glm::mat4),
                      glm::value_ptr(m_Camera->GetVPMatrix()));
  m_MatrixUBO.SetData(64, sizeof(glm::vec3),
                      glm::value_ptr(m_Camera->Position));

  switch (m_State) {
  case Game::GameState::Playing:
    m_Level.Render(m_InstancedShader, m_NormalShader);
    break;

  case Game::GameState::Editing:
    m_Level.RenderEditingMode(m_BlockEditingIndex, m_Camera.get());
    m_Level.RenderOneByOne(m_NormalShader, m_NormalShader);
    break;

  case Game::GameState::Endscreen:
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
  if (m_CrosshairTexture.index())
    m_CrosshairTexture =
        std::get<LoadingTexture::Future>(m_CrosshairTexture).get()->Finish();

  ImGui::DragFloat("Crosshair size", &m_CrosshairSize, 0.01f, 0.0f, 0.5f);

  m_UIShader->Use();
  glBindTextureUnit(7, std::get<Texture>(m_CrosshairTexture).ID);

  m_UIShader->SetVec2(
      "uSize", {m_CrosshairSize / m_Window->GetAspectRatio(), m_CrosshairSize});

  screenVAO->Bind();
  glDrawArrays(GL_TRIANGLES, 0, 6);
  screenVAO->UnBind();
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
