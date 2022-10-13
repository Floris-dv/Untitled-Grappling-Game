#include "pch.h"
#include "Shader.h"
#include "glad/glad.h"
#include "Framebuffers.h"

#include "Settings.h"
#include "Shadows.h"

namespace Shadows {
#if ENABLE_SHADOWS
	namespace Shaders {
		void Setup() {
			dirLight = new Shader("src/Shaders/Shadow.vert", "src/Shaders/Shadow.frag");
			iDirLight = new Shader("src/Shaders/InstancedDirShadow.vert", "src/Shaders/Shadow.frag");

			pointLight = new Shader("src/Shaders/PointLightShadow.vert", "src/Shaders/PointLightShadow.frag", "Shaders/PointLightShadow.geom");
			iPointLight = new Shader("src/Shaders/InstancedPointShadows.vert", "src/Shaders/PointLightShadow.frag", "Shaders/PointLightShadow.geom");
		}

		void Delete() {
			delete dirLight, iDirLight, pointLight, iPointLight;
		}
	}
#endif

	void Setup() {
#if ENABLE_SHADOWS
		Shaders::Setup();

		const float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };

		// dirMap:
		{
			glGenTextures(1, &dirMap);
			glBindTexture(GL_TEXTURE_2D, dirMap);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, DEPTH_MAP_RES, DEPTH_MAP_RES, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

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
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, DEPTH_MAP_RES, DEPTH_MAP_RES, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

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
#endif
	}

	void Delete() {
#if ENABLE_SHADOWS
		Shaders::Delete();

		glDeleteTextures(1, &dirMap);
		glDeleteTextures(1, &pointMap);

		glDeleteFramebuffers(1, &FBO);
#endif
	}

#if ENABLE_SHADOWS
	void Bind(GLuint depthMap, Shader* shader) {
		Framebuffers::Bind(FBO);

		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthMap, 0);

		glClear(GL_DEPTH_BUFFER_BIT);

		shader->use();
	}
#endif
}