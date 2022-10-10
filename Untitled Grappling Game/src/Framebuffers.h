#pragma once

#include "Shader.h"

#include "DataBuffers.h"

#include <glm/glm.hpp>
#include "Window.h"

namespace Framebuffers {
	// for the quad:
	inline VertexArray* VAO;

	// Tex stands for texture:
	// RB stands for RenderBuffer
	inline unsigned int main;
	inline unsigned int mainTex;
#if ENABLE_BLOOM
	inline glm::ivec2 bloomTexSize;

	inline unsigned int bloomMipCount;
#endif

	inline Shader* PostProcessingShader;

	inline unsigned int mainRB;

	void Delete();

	// Draw the contents of the currently bound framebuffer to the screen
	void Draw(float exposure, GLuint texture = mainTex, GLuint bloomTexture = 0);

	void Bind(GLuint FBO);

	void Setup();
}
