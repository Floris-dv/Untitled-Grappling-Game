#pragma once
#include "DataBuffers.h"
#include "GrapplingCamera.h"
#include "Material.h"
#include "Mesh.h"
#include "RigidBody.h"
#include "UtilityMacros.h"
#include "Window.h"

/* TODO
 * Structure that I want:
 *  - Level: base class (blocks, finishBox)
 *  - PlayingLevel: Purely playing (creates the matrices, then deletes them),
 * instanced rendering. Can read
 *  - EditingLevel: Normal rendering. Can read/write
 */

#define VERSION(major, minor, build)                                           \
  (((major) << 22) | ((minor) << 12) | (build))

class Level {
public:
  struct Block {
    glm::vec3 Center = glm::vec3{0.0f};
    glm::vec3 Halfsize = glm::vec3{0.5f};
    glm::quat Rotation = {1.0f, 0.0f, 0.0f, 0.0f};
  };

public:
  Level() : m_AllowEditing(true) {}

  Level(const std::string &levelFile, Window *window);

  Level(Level &&level) noexcept { swap(level); }

  void swap(Level &other);

  OVERLOAD_OPERATOR_RVALUE(Level)

  DELETE_COPY_CONSTRUCTOR(Level)

  void Write(std::string_view levelFile);

  // Assumes the MVP-matrix has been set
  void Render(Material *material, Material *finishMaterial);

  void Render(Shader *shader, Shader *finishShader);

  // Doesn't use instanced rendering, instead uses normal rendering, used for
  // editing mode
  void RenderOneByOne(Shader *shader, Shader *finishShader);

  void RenderEditingMode(size_t &index, Camera *camera);

  const std::vector<Block> &GetBlocks() { return m_Blocks; }

  // Returns: if the level is finished
  bool UpdatePhysics(GrapplingCamera &camera);

  // Needs to be called when switching from RenderOneByOne() to Render()
  void UpdateInstanceVBO();

  double GetStartTime() { return m_StartTime; }

  const EmptyMaterial &GetTheme() { return m_MainMaterial; }

private:
  // First block: finish box, Second block (by convention): startplatform
  std::vector<Block> m_Blocks;
  std::vector<BoxCollider> m_BoxColliders;
  std::vector<glm::mat4> m_Matrices;

  inline static Mesh<MinimalVertex> s_BlockMesh;

  EmptyMaterial m_MainMaterial;
  EmptyMaterial m_FinishMaterial;

  VertexBuffer m_InstanceVBO;

  double m_StartTime = 0.0;

  // TODO: add a level name here

  std::string m_FileName;

  bool m_AllowEditing;

  Window *m_Window;

  static constexpr uint32_t VERSION_NR = VERSION(0, 0, 2);

  // Uses m_Matrices to setup m_InstanceVBO and Block::Object
  void SetupInstanceVBO();

  // Uses m_Blocks to setup m_Matrices
  void SetupMatrices();

  float SDF(const glm::vec3 &samplePoint);
};

OVERLOAD_STD_SWAP(Level)
