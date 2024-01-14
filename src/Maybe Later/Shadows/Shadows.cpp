#include "pch.h"

#include "Framebuffers.h"
#include "Settings.h"
#include "Shader.h"
#include "Shadows.h"
#include "glad/glad.h"

namespace Shadows {
void ShadowPass() {
  ImGui::DragInt("Number of shadows", &SHADOWS, 1.0f, 0,
                 pointLightPositions.size());

  glm::mat4 dirLightSpaceMatrix;
  std::array<glm::mat4, 6> pointLightSpaceMatrix;

  // generate the depth map:
  if (SHADOWS) {
    glm::mat4 view =
        glm::lookAt(lightDir, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 projection =
        glm::ortho(-lSS, lSS, -lSS, lSS, near_plane, far_plane);

    dirLightSpaceMatrix = projection * view;

    glViewport(0, 0, DEPTH_MAP_RES, DEPTH_MAP_RES);

    Shadows::Bind(Shadows::dirMap, Shadows::Shaders::iDirLight);

    // for the dir light:
    {
      // render the rock

      Shadows::Shaders::iDirLight->use();

      Shadows::Shaders::iDirLight->setMat4("lightSpaceMatrix",
                                           dirLightSpaceMatrix);

      rock.DrawInstanced(*Shadows::Shaders::iDirLight, count, false);

      // render the backpack
      backpack.DrawInstanced(*Shadows::Shaders::iDirLight,
                             static_cast<unsigned int>(objectPositions.size()),
                             false);

      glActiveTexture(GL_TEXTURE14);
      glBindTexture(GL_TEXTURE_2D, Shadows::dirMap);
    }

    Shadows::Bind(Shadows::pointMap, Shadows::Shaders::iPointLight);

    // set up pointLightSpaceMatrix
    projection =
        glm::perspective(glm::radians(90.0f), 1.0f, near_plane, far_plane);

    pointLightSpaceMatrix[0] =
        projection *
        glm::lookAt(pointLightPositions[0],
                    pointLightPositions[0] + glm::vec3(1.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, -1.0f, 0.0f));
    pointLightSpaceMatrix[1] =
        projection *
        glm::lookAt(pointLightPositions[0],
                    pointLightPositions[0] + glm::vec3(-1.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, -1.0f, 0.0f));
    pointLightSpaceMatrix[2] =
        projection *
        glm::lookAt(pointLightPositions[0],
                    pointLightPositions[0] + glm::vec3(0.0f, 1.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 1.0f));
    pointLightSpaceMatrix[3] =
        projection *
        glm::lookAt(pointLightPositions[0],
                    pointLightPositions[0] + glm::vec3(0.0f, -1.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, -1.0f));
    pointLightSpaceMatrix[4] =
        projection *
        glm::lookAt(pointLightPositions[0],
                    pointLightPositions[0] + glm::vec3(0.0f, 0.0f, 1.0f),
                    glm::vec3(0.0f, -1.0f, 0.0f));
    pointLightSpaceMatrix[5] =
        projection *
        glm::lookAt(pointLightPositions[0],
                    pointLightPositions[0] + glm::vec3(0.0f, 0.0f, -1.0f),
                    glm::vec3(0.0f, -1.0f, 0.0f));

    {
      // render the rocks
      glm::mat4 model = glm::mat4(1.0f);

      Shadows::Shaders::iPointLight->setVec3("lightPos",
                                             pointLightPositions[0]);
      Shadows::Shaders::iPointLight->setFloat("far_plane", far_plane);

      for (int i = 0; i < 6; i++)
        Shadows::Shaders::iPointLight->setMat4("lightSpaceMatrices[" +
                                                   std::to_string(i) + "]",
                                               pointLightSpaceMatrix[i]);

      /*
      for (int i = 0; i < count; i++) {
              pointLightShadowShader.setMat4("model", modelMatrices[i]);
              rock.Draw(pointLightShadowShader);
      }
      */
      // asteroidShadowShader.setMat4("lightSpaceMatrix",
      // pointLightSpaceMatrix);
      rock.DrawInstanced(*Shadows::Shaders::iPointLight, count, false);

      // render the backpack
      backpack.DrawInstanced(*Shadows::Shaders::iPointLight,
                             objectPositions.size(), false);
    }

    glActiveTexture(GL_TEXTURE15);
    glBindTexture(GL_TEXTURE_CUBE_MAP, Shadows::pointMap);

    glViewport(0, 0, WIDTH, HEIGHT);
  }
}

namespace Shaders {
void Setup() {
  dirLight = new Shader("src/Shaders/Shadow.vert", "src/Shaders/Shadow.frag");
  iDirLight = new Shader("src/Shaders/InstancedDirShadow.vert",
                         "src/Shaders/Shadow.frag");

  pointLight = new Shader("src/Shaders/PointLightShadow.vert",
                          "src/Shaders/PointLightShadow.frag",
                          "Shaders/PointLightShadow.geom");
  iPointLight = new Shader("src/Shaders/InstancedPointShadows.vert",
                           "src/Shaders/PointLightShadow.frag",
                           "Shaders/PointLightShadow.geom");
}

void Delete() { delete dirLight, iDirLight, pointLight, iPointLight; }
} // namespace Shaders

void Setup() {
  Shaders::Setup();

  const float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};

  // dirMap:
  {
    glGenTextures(1, &dirMap);
    glBindTexture(GL_TEXTURE_2D, dirMap);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, DEPTH_MAP_RES,
                 DEPTH_MAP_RES, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
  }

  // pointMap
  {
    glGenTextures(1, &pointMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, pointMap);

    for (unsigned int i = 0; i < 6; i++)
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT,
                   DEPTH_MAP_RES, DEPTH_MAP_RES, 0, GL_DEPTH_COMPONENT,
                   GL_FLOAT, NULL);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BORDER_COLOR, borderColor);
  }

  glGenFramebuffers(1, &FBO);

  Framebuffers::Bind(FBO);

  // needs to happen, to tell OpenGL we're not going to render any color data
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);

  Framebuffers::Bind(0);
}

void Delete() {
  Shaders::Delete();

  glDeleteTextures(1, &dirMap);
  glDeleteTextures(1, &pointMap);

  glDeleteFramebuffers(1, &FBO)
}

void Bind(GLuint depthMap, Shader *shader) {
  Framebuffers::Bind(FBO);

  glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthMap, 0);

  glClear(GL_DEPTH_BUFFER_BIT);

  shader->use();
}
} // namespace Shadows
