#include "pch.h"

#define OVERLOAD_GLM_OSTREAM 0
#include "GrapplingCamera.h"
#include "Level.h"
#include "Log.h"
#include "RigidBody.h"
#include "SaveFile.h"
#include "UtilityMacros.h"
#include "VertexData.h"
#include "Window.h"
#include "imgui/imgui.h"
#include <ImGuizmo/ImGuizmo.h>

template <typename OStream>
inline OStream &operator<<(OStream &output, Level::Block const &input) {
  output << input.Center;
  output << input.Halfsize;
  output << input.Rotation;

  return output;
}

template <typename IStream>
inline IStream &operator>>(IStream &input, Level::Block &output) {
  input >> output.Center;
  input >> output.Halfsize;
  input >> output.Rotation;

  return input;
}

Level::Level(const std::string &levelFile, Window *window)
    : m_FileName(levelFile), m_Window(window) {
  std::ifstream file(levelFile, std::ios::in | std::ios::binary);

  if (!file)
    NG_ERROR("File {} doesn't exist", levelFile);

  uint32_t version_nr;
  file >> version_nr;

  file >> m_AllowEditing;

  file >> m_Blocks;

  m_MainMaterial = EmptyMaterial(file);
  m_FinishMaterial = EmptyMaterial(file);

  SetupMatrices();
  SetupInstanceVBO();
}

void Level::swap(Level &other) {
  SWAP(m_AllowEditing);
  SWAP(m_Blocks);
  SWAP(m_FileName);
  SWAP(m_FinishMaterial);
  SWAP(m_InstanceVBO);
  SWAP(m_MainMaterial);
  SWAP(m_Matrices);
  SWAP(m_StartTime);
  SWAP(m_Window);
}

void Level::Write(std::string_view levelFile) {
  std::ofstream file(levelFile.data(), std::ios::binary);

  file << VERSION_NR << '\n';

  file << m_AllowEditing << '\n';

  file << m_Blocks;

  m_MainMaterial.Serialize(file);
  m_FinishMaterial.Serialize(file);
}

void Level::Render(Material *material, Material *finishMaterial) {
  s_BlockMesh.DrawInstanced(material, true,
                            (GLsizei)m_Matrices.size() -
                                1); // finish box needs to be another color
  s_BlockMesh.Draw(m_Matrices.front(), finishMaterial, true);
}

void Level::Render(Shader *shader, Shader *finishShader) {
  m_FinishMaterial.Load(*finishShader);

  s_BlockMesh.DrawInstanced(&m_MainMaterial, shader,
                            (GLsizei)m_Matrices.size() - 1);

  s_BlockMesh.Draw(m_Matrices.front(), &m_FinishMaterial, finishShader);
}

void Level::RenderOneByOne(Shader *shader, Shader *finishShader) {
  for (size_t i = 1; i < m_Matrices.size(); i++)
    s_BlockMesh.Draw(m_Matrices[i], &m_MainMaterial, shader);

  s_BlockMesh.Draw(m_Matrices.front(), &m_FinishMaterial, finishShader);
}

void Level::RenderEditingMode(size_t &index, Camera *camera) {
  if (!m_AllowEditing)
    return;

  static constexpr ImGuiDataType size_t_DataType =
      std::is_same_v<size_t, uint32_t> ? ImGuiDataType_U32 : ImGuiDataType_U64;

  static ImGuizmo::OPERATION s_CurrentGizmoOperation(ImGuizmo::TRANSLATE);
  static ImGuizmo::MODE s_CurrentGizmoMode(ImGuizmo::WORLD);

  {
    const size_t numMatrices = m_Matrices.size();

    // TODO: Center this
    if (ImGui::Button("<"))
      index--;
    ImGui::SameLine();
    ImGui::DragScalar("##EMPTY", size_t_DataType, &index, 1.0f, 0,
                      &numMatrices);
    ImGui::SameLine();
    if (ImGui::Button(">"))
      index++;
    index = (index + numMatrices) % numMatrices;
    ImGui::SameLine();
    if (ImGui::Button("+")) {
      m_Matrices.push_back(glm::identity<glm::mat4>());
      m_Blocks.push_back({});
      index = m_Matrices.size() - 1;
    }
  }
  float *matrix = glm::value_ptr(m_Matrices[index]);

  if (ImGui::IsKeyPressed(ImGuiKey_T))
    s_CurrentGizmoOperation = ImGuizmo::TRANSLATE;
  if (ImGui::IsKeyPressed(ImGuiKey_R))
    s_CurrentGizmoOperation = ImGuizmo::ROTATE;
  if (ImGui::IsKeyPressed(ImGuiKey_E))
    s_CurrentGizmoOperation = ImGuizmo::SCALE;
  if (ImGui::RadioButton("Translate",
                         s_CurrentGizmoOperation == ImGuizmo::TRANSLATE))
    s_CurrentGizmoOperation = ImGuizmo::TRANSLATE;
  ImGui::SameLine();
  if (ImGui::RadioButton("Rotate", s_CurrentGizmoOperation == ImGuizmo::ROTATE))
    s_CurrentGizmoOperation = ImGuizmo::ROTATE;
  ImGui::SameLine();
  if (ImGui::RadioButton("Scale", s_CurrentGizmoOperation == ImGuizmo::SCALE))
    s_CurrentGizmoOperation = ImGuizmo::SCALE;
  glm::vec3 matrixTranslation, matrixRotation, matrixScale;
  ImGuizmo::DecomposeMatrixToComponents(
      matrix, glm::value_ptr(matrixTranslation), glm::value_ptr(matrixRotation),
      glm::value_ptr(matrixScale));

  ImGui::DragFloat3("Tr", glm::value_ptr(matrixTranslation), 1.0f);
  ImGui::DragFloat3("Rt", glm::value_ptr(matrixRotation), 0.1f);
  ImGui::DragFloat3("Sc", glm::value_ptr(matrixScale), 1.0f);
  ImGuizmo::RecomposeMatrixFromComponents(glm::value_ptr(matrixTranslation),
                                          glm::value_ptr(matrixRotation),
                                          glm::value_ptr(matrixScale),
                                          matrix); // changes m_Matrices[index]

  m_Blocks[index] = {matrixTranslation, matrixScale * 0.5f,
                     glm::quat(glm::radians(matrixRotation))};

  m_BoxColliders[index].Center = matrixTranslation;
  m_BoxColliders[index].MatrixRS = glm::mat3(m_Matrices[index]);
  m_BoxColliders[index].Inverse_MatrixRS =
      glm::inverse(m_BoxColliders[index].MatrixRS);

  if (s_CurrentGizmoOperation != ImGuizmo::SCALE) {
    if (ImGui::RadioButton("Local", s_CurrentGizmoMode == ImGuizmo::LOCAL))
      s_CurrentGizmoMode = ImGuizmo::LOCAL;
    ImGui::SameLine();
    if (ImGui::RadioButton("World", s_CurrentGizmoMode == ImGuizmo::WORLD))
      s_CurrentGizmoMode = ImGuizmo::WORLD;
  }
  static bool useSnap(false);
  if (ImGui::IsKeyPressed(ImGuiKey_Y))
    useSnap = !useSnap;
  ImGui::Checkbox("Enable snapping", &useSnap);
  ImGui::SameLine();
  static glm::vec3 snap{0.0f};
  switch (s_CurrentGizmoOperation) {
  case ImGuizmo::TRANSLATE:
    ImGui::DragFloat3("Snap", &snap.x);
    break;
  case ImGuizmo::ROTATE:
    ImGui::DragFloat("Angle Snap", &snap.x);
    break;
  case ImGuizmo::SCALE:
    ImGui::DragFloat("Scale Snap", &snap.x);
    break;
  default:
    break;
  }
  ImGuizmo::SetRect(0, 0, static_cast<float>(m_Window->GetWidth()),
                    static_cast<float>(m_Window->GetHeight()));
  ImGuizmo::Manipulate(glm::value_ptr(camera->GetViewMatrix()),
                       glm::value_ptr(camera->GetProjMatrix()),
                       s_CurrentGizmoOperation, s_CurrentGizmoMode, matrix,
                       nullptr, useSnap ? &snap.x : nullptr);

  if (ImGui::Button("Save as")) {
    NG_INFO("Saving File to {}", m_FileName);
    m_FileName = GetSaveFileNameAllPlatforms(m_Window->GetGLFWWindow());
    NG_INFO("Saving File to {}", m_FileName);
    Write(m_FileName);
  }
  if (!m_FileName.empty())
    if (ImGui::Button("Save"))
      Write(m_FileName);
}

bool Level::UpdatePhysics(GrapplingCamera &camera) {
  camera.IsOnGround = false;
  if (m_StartTime == 0.0f &&
      (glm::abs(camera.Vel.x) + glm::abs(camera.Vel.z)) > 0.0f) {
    m_StartTime = m_Window->GetTime();
  }

  if (camera.PhysicsPosition.y < -50.0f) {
    camera.Reset();
    m_StartTime = 0.0f;
  }

  // TODO: add better constant factor for the camera block here
  CapsuleCollider cameraCapsule;
  cameraCapsule.MatrixRS = camera.GetModelMatrix();
  cameraCapsule.Inverse_MatrixRS = glm::inverse(camera.GetModelMatrix());
  cameraCapsule.Center = camera.PhysicsPosition;
  cameraCapsule.r = 1;
  cameraCapsule.y_base = 1;
  cameraCapsule.y_cap = 2;

  for (size_t i = 1; i < m_Blocks.size() - 1; i++) {
    HitInfo collision = Collide(&cameraCapsule, &m_BoxColliders[i]);
    if (collision.Hit) {
      float slope = glm::acos(glm::dot(collision.Normal, glm::vec3(0, 1, 0)));
      if (slope < glm::radians(camera.MaxStandSlope)) {
        camera.Vel.y = 0;
        camera.IsOnGround = true;
        camera.IsJumping = false;
      }
      camera.PhysicsPosition += collision.Normal * collision.PenetrationDepth;
      camera.Position += collision.Normal * collision.PenetrationDepth;
    }
  }

  ImGui::Text("Distance from camera: %f", SDF(camera.PhysicsPosition));

  return Intersect(&cameraCapsule, &m_BoxColliders.front());
}

// Needs to be called when switching from RenderOneByOne() to Render()
void Level::UpdateInstanceVBO() {
  glNamedBufferData(
      m_InstanceVBO.ID(),
      (GLsizeiptr)((m_Matrices.size() - 1) * sizeof(m_Matrices[0])),
      &m_Matrices[1], GL_STATIC_DRAW);
}

void Level::SetupInstanceVBO() {
  std::shared_ptr<Material> s(nullptr);
  m_InstanceVBO =
      VertexBuffer((static_cast<unsigned int>(m_Matrices.size()) - 1) *
                       sizeof(m_Matrices[0]),
                   &m_Matrices[1]);

  if (!s_BlockMesh.IsValid())
    s_BlockMesh = Mesh<MinimalVertex>(boxVertices, MinimalVertex::Layout);

  s_BlockMesh.SetInstanceBuffer(m_InstanceVBO);
}

void Level::SetupMatrices() {
  m_Matrices.clear();
  m_BoxColliders.clear();
  m_Matrices.reserve(m_Blocks.size() - 1);
  m_BoxColliders.reserve(m_Blocks.size() - 1);

  for (Block &block : m_Blocks) {
    glm::mat4 matrix = glm::mat4_cast(block.Rotation) *
                       glm::scale(glm::mat4{1.0f}, block.Halfsize);

    matrix[3] = glm::vec4(block.Center, 1.0f);
    m_Matrices.push_back(matrix);

    BoxCollider boxCollider;
    boxCollider.Center = block.Center;
    boxCollider.MatrixRS = glm::mat3(matrix);
    boxCollider.Inverse_MatrixRS = glm::inverse(boxCollider.MatrixRS);
    m_BoxColliders.push_back(boxCollider);
  }
}

float Level::SDF(const glm::vec3 &samplePoint) {
  float distance = std::numeric_limits<float>::max();

  for (size_t i = 1; i < m_Blocks.size() - 1; i++) {
    const auto &block = m_Blocks[i];
    glm::vec3 copy = samplePoint;
    glm::vec3 blockSize = 2.0f * block.Halfsize;
    copy -= block.Center;
    copy = glm::conjugate(block.Rotation) * copy;
    glm::vec3 q = glm::abs(copy) - blockSize;
    distance = glm::min(length(glm::max(q, glm::vec3(0.0))) +
                            glm::min(glm::max(q.x, glm::max(q.y, q.z)), 0.0f),
                        distance);
  };

  return distance;
}
