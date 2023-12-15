#pragma once
// Header file for vertex data generation and data:

#include "Vertex.h"

// Simple sphere generation, not anything fancy
inline std::pair<std::vector<MinimalVertex>, std::vector<GLuint>>
CreateSphere(size_t stacks, size_t slices);

inline std::pair<std::vector<MinimalVertex>, std::vector<GLuint>>
CreateCylinder(size_t stacks, size_t slices);

// Can't be constexpr because it's used by a span. Should not be modified
const inline std::array<SimpleVertex, 36> boxVertices = {{
    // Front face
    {{-1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    {{1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
    {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},

    // Right face
    {{1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
    {{1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{1.0f, 1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{1.0f, 1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
    {{1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},

    // Back
    {{1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
    {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
    {{-1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
    {{-1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
    {{1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
    {{1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},

    // Left
    {{-1.0f, -1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{-1.0f, -1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
    {{-1.0f, 1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
    {{-1.0f, 1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
    {{-1.0f, 1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{-1.0f, -1.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},

    // top
    {{-1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
    {{1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
    {{1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{-1.0f, 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{-1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},

    // bottom
    {{1.0f, -1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
    {{-1.0f, -1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
    {{-1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
    {{-1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
    {{1.0f, -1.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
    {{1.0f, -1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
}};

constexpr std::array<Vertex, 6> floorVertices = {{
    {{-10.0f, -7.5f, -10.0f},
     {0.0f, 1.0f, 0.0f},
     {0.0f, 0.0f},
     {1.0f, 0.0f, 0.0f},
     {0.0f, 0.0f, 1.0f}},
    {{-10.0f, -7.5f, 10.0f},
     {0.0f, 1.0f, 0.0f},
     {0.0f, 2.0f},
     {1.0f, 0.0f, 0.0f},
     {0.0f, 0.0f, 1.0f}},
    {{10.0f, -7.5f, 10.0f},
     {0.0f, 1.0f, 0.0f},
     {2.0f, 2.0f},
     {1.0f, 0.0f, 0.0f},
     {0.0f, 0.0f, 1.0f}},
    {{-10.0f, -7.5f, -10.0f},
     {0.0f, 1.0f, 0.0f},
     {0.0f, 0.0f},
     {1.0f, 0.0f, 0.0f},
     {0.0f, 0.0f, 1.0f}},
    {{10.0f, -7.5f, 10.0f},
     {0.0f, 1.0f, 0.0f},
     {2.0f, 2.0f},
     {1.0f, 0.0f, 0.0f},
     {0.0f, 0.0f, 1.0f}},
    {{10.0f, -7.5f, -10.0f},
     {0.0f, 1.0f, 0.0f},
     {2.0f, 0.0f},
     {1.0f, 0.0f, 0.0f},
     {0.0f, 0.0f, 1.0f}},
}};

// Data for light uniform:
constexpr std::array<float, 120> lightData = {{
    // Dirlight:
    -0.2f, -1.0f, -0.3f, 0.0f, // direction (+ padding)
    0.8f, 0.8f, 0.8f, 0.0f,    // ambient (+ padding)
    0.2f, 0.2f, 0.2f, 0.0f,    // diffuse (+ padding)
    0.0f, 0.0f, 0.0f, 0.0f,    // specular (+ padding)

    // Pointlight[0]:
    0.7f, 0.2f, 2.0f, 0.0f,    // position (+ padding)
    0.05f, 0.05f, 0.05f, 0.0f, // ambient (+ padding)
    1.0f, 1.0f, 1.0f, 0.0f,    // diffuse (+ padding)
    0.1f, 0.1f, 0.1f, 0.0f,    // specular (+ padding)
    0.09f,                     // linear;
    0.032f,                    // quadratic;
    0.0f, 0.0f,                // padding

    // Pointlight[1]:
    2.3f, -0.5f, -3.3f, 0.0f,  // position (+ padding)
    0.05f, 0.05f, 0.05f, 0.0f, // ambient (+ padding)
    1.0f, 1.0f, 1.0f, 0.0f,    // diffuse (+ padding)
    0.1f, 0.1f, 0.1f, 0.0f,    // specular (+ padding)
    0.09f,                     // linear;
    0.032f,                    // quadratic;
    0.0f, 0.0f,                // padding

    // Pointlight[2]:
    -1.3f, 0.4f, 2.0f, 0.0f,   // position (+ padding)
    0.05f, 0.05f, 0.05f, 0.0f, // ambient (+ padding)
    1.0f, 1.0f, 1.0f, 0.0f,    // diffuse (+ padding)
    0.1f, 0.1f, 0.1f, 0.0f,    // specular (+ padding)
    0.09f,                     // linear;
    0.032f,                    // quadratic;
    0.0f, 0.0f,                // padding

    // Pointlight[3]:
    0.0f, 0.0f, -3.0f, 0.0f,   // position (+ padding)
    0.05f, 0.05f, 0.05f, 0.0f, // ambient (+ padding)
    1.0f, 1.0f, 1.0f, 0.0f,    // diffuse (+ padding)
    0.1f, 0.1f, 0.1f, 0.0f,    // specular (+ padding)
    0.09f,                     // linear;
    0.032f,                    // quadratic;
    0.0f, 0.0f,                // padding

    // Spotlight:
    0.0f, 0.0f, 3.0f, 0.0f,    // position (+ padding)
    0.0f, 0.0f, -1.0f, 0.0f,   // direction (+ padding)
    0.0f, 0.0f, 0.0f, 0.0f,    // ambient (+ padding)
    10.0f, 10.0f, 10.0f, 0.0f, // diffuse (+ padding)
    2.0f, 2.0f, 2.0f,          // specular (+ padding)

    0.09f,  // linear
    0.032f, // quadratic

    0.9762960071199334f, // cutOff ( = cos(15 degrees)
    0.9659258262890683f, // outerCutOff ( = cos(17.5 degrees)
}};

std::pair<std::vector<MinimalVertex>, std::vector<unsigned int>>
CreateSphere(size_t stacks, size_t slices) {
  static constexpr float PI = glm::pi<float>();
  std::vector<glm::vec3> positions;
  positions.reserve(stacks * slices);
  std::vector<unsigned int> indices;
  indices.reserve((slices * stacks + slices) * 6);

  // loop through stacks.
  for (size_t i = 0; i < stacks; ++i) {
    float V = (float)i / (float)stacks;
    float phi = V * PI;

    // loop through the slices.
    for (size_t j = 0; j < slices; ++j) {
      float U = (float)j / (float)slices;
      float theta = U * (PI * 2);

      // use spherical coordinates to calculate the positions.
      float x = cos(theta) * sin(phi);
      float y = cos(phi);
      float z = sin(theta) * sin(phi);

      positions.push_back({x, y, z});
    }
  }

  std::vector<MinimalVertex> vertices;
  vertices.reserve(stacks * slices * 2);

  // Calc The Index Positions
  for (size_t i = 0; i < slices * stacks + slices; ++i) {
    indices.push_back((unsigned int)(i * 2));
    indices.push_back((unsigned int)((i + slices + 1) * 2));
    indices.push_back((unsigned int)((i + slices) * 2));
    // MAYBE: add tangents/bitangents here
    vertices.emplace_back(positions[i],
                          glm::cross(positions[i] - positions[i + slices],
                                     positions[i] - positions[i + slices + 1]));
    indices.push_back((unsigned int)(i * 2));
    indices.push_back((unsigned int)((i + 1) * 2));
    indices.push_back((unsigned int)((i + slices + 1) * 2));

    vertices.emplace_back(positions[i],
                          glm::cross(positions[i] - positions[i + slices + 1],
                                     positions[i] - positions[i + 1]));
  }

  return std::make_pair(vertices, indices);
}

// Stacks = number of pizza slices
// Slices = number of hockey pucks, minus one
std::pair<std::vector<MinimalVertex>, std::vector<unsigned int>>
CreateCylinder(size_t stacks, size_t slices) {
  static constexpr float PI = glm::pi<float>();

  std::vector<glm::vec3> positions;
  positions.reserve(stacks * slices);
  std::vector<unsigned int> indices;
  indices.reserve((slices * stacks + slices) * 6);

  std::vector<MinimalVertex> vertices;
  vertices.reserve(2 * stacks * slices);

  // loop through stacks.
  for (size_t i = 0; i < stacks; ++i) {
    float V = (float)i / (float)stacks;
    float phi = V * PI;

    // loop through the slices.
    for (size_t j = 0; j < slices; ++j) {
      float U = (float)j / (float)slices;
      float theta = U * (PI * 2);

      // use spherical coordinates to calculate the positions.
      float x = cos(theta) * sin(phi);
      float y = cos(phi);
      float z = sin(theta) * sin(phi);

      positions.push_back({x, y, z});
    }
  }
  return std::make_pair(vertices, indices);
}
