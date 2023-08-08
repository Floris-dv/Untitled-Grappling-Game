#include "pch.h"

#include "Game.h"
#include "utils.h"

#include "Levels.h"

#include "Log.h"
#include "Material.h"   // DEBUG
#include "VertexData.h" // DEBUG

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
extern bool paused;
extern float lastX, lastY;
static bool FirstFrame = true;

Game::Game(std::unique_ptr<Camera> &&camera, Level &&level,
           std::shared_ptr<Shader> instancedShader,
           std::shared_ptr<Shader> normalShader, Window *window)
    : m_Camera(std::move(camera)), m_Level(std::move(level)),
      m_InstancedShader(std::move(instancedShader)),
      m_NormalShader(std::move(normalShader)), m_Window(window),
      m_MatrixUBO(sizeof(glm::mat4) + sizeof(glm::vec4), "Matrices") {

  auto lightShader = std::make_shared<Shader>("src/Shaders/LightVert.vert",
                                              "src/Shaders/LightFrag.frag");
  auto lightMaterial = std::make_shared<Material>(
      lightShader, glm::vec3{0.0f, 1.0f, 1.0f}, glm::vec3{0.0f, 0.0f, 0.0f});

  m_MatrixUBO.Bind();
  m_MatrixUBO.SetBlock(*m_NormalShader);
  m_MatrixUBO.SetBlock(*m_InstancedShader);
  m_MatrixUBO.SetBlock(*lightShader);

  auto [vertices, indices](CreateSphere(20, 20));
  Sphere = Object<MinimalVertex>(lightMaterial, vertices, MinimalVertex::Layout,
                                 indices);

  NG_INFO("SETTING");
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
            if (!(ImGui::IsWindowFocused(4) ||
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
        const float yoffset =
            lastY -
            fypos; // reversed since y-coordinates range from bottom to top
        lastX = fxpos;
        lastY = fypos;

        m_Camera->ProcessMouseMovement(xoffset, yoffset);
      };
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
      m_LevelNr++;
      m_Level =
          Level(GetLevelByNr(m_LevelNr), m_InstancedShader, m_NormalShader);
      m_Endscreen.Close();
      break;

    case Endscreen::State::Restart:
      m_Level =
          Level(GetLevelByNr(m_LevelNr), m_InstancedShader, m_NormalShader);
      m_Endscreen.Close();
      break;
    }
    break;
  }
}

void Game::Render() {
  m_MatrixUBO.SetData(0, sizeof(glm::mat4),
                      glm::value_ptr(m_Camera->GetVPMatrix()));
  m_MatrixUBO.SetData(64, sizeof(glm::vec3),
                      glm::value_ptr(m_Camera->Position));

  switch (m_State) {
  case Game::GameState::Playing:
    m_Level.Render();
    break;

  case Game::GameState::Editing:
    m_Level.RenderEditingMode(m_BlockEditingIndex);
    m_Level.RenderOneByOne();
    break;

  case Game::GameState::Endscreen:
    m_Level.Render();
    m_Endscreen.Render();
    break;
  }
}

void Game::Finalize() {
  if (m_State == GameState::Playing) {
    if (m_Window->GetMouseButtonDown(1)) {
      float depth;
      glReadPixels(m_Window->GetWidth() / 2, m_Window->GetHeight() / 2, 1, 1,
                   GL_DEPTH_COMPONENT, GL_FLOAT, &depth);

      float zNorm = 2.0f * depth - 1.0f;
      float zFar = m_Camera->Options.ZFar;
      float zNear = m_Camera->Options.ZNear;
      float zView =
          -2.0f * zNear * zFar / ((zFar - zNear) * zNorm - zNear - zFar);

      m_SpherePos = m_Camera->Position + m_Camera->Front * zView;

      if (zView < 80.0f) {
        ((GrapplingCamera *)m_Camera.get())->LaunchAt(m_SpherePos);
      }
    } else {
      ((GrapplingCamera *)m_Camera.get())->LaunchAt(m_SpherePos);
    }

    glm::mat4 model(1.0f);
    model = glm::translate(
        model, ((GrapplingCamera *)Camera::Get())->GrapplingPosition());
    Sphere.Draw(model, false);

    // TODO: bloom
  }

  // TODO: draw crossair
}

void Game::SetLevel(int level) {
  m_LevelNr = level % NR_LEVELS;
  m_Level = Level(GetLevelByNr(m_LevelNr), m_InstancedShader, m_NormalShader);
  m_Camera->Reset();
}