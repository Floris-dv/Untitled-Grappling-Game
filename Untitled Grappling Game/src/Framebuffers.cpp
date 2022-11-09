#include "pch.h"
#include "Settings.h"
#include "Log.h"
#include "Timer.h"
#include "Framebuffers.h"

#include "Shader.h"

#include "DataBuffers.h"

#include <glad/glad.h>

#include "Camera.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/integer.hpp>

namespace Framebuffers {
	static GLuint bound = 0;
	static VertexBuffer* VBO;

	void Bind(GLuint FBO) {
		if (bound != FBO) {
			bound = FBO;
			glBindFramebuffer(GL_FRAMEBUFFER, FBO);
		}
	}

	void Draw(float exposure, GLuint texture, GLuint bloomTexture) {
		Profiler p("Final postprocessing");

		glDisable(GL_DEPTH_TEST);

		VAO->Bind();

		PostProcessingShader->Use();

		PostProcessingShader->SetInt("screen", 0);
		if (bloomTexture) {
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, bloomTexture);
			PostProcessingShader->SetInt("bloomBlur", 1);
		}

		Bind(0);
		glClear(GL_COLOR_BUFFER_BIT);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glEnable(GL_DEPTH_TEST);

		VAO->UnBind();
	}

	// Creates the textures and stores in the renderbuffers (redundant to create them again), also binds them to the main FBO
	void CreateTexturesRBs() {
		// mainTex:
		{
			glCreateTextures(GL_TEXTURE_2D, 1, &mainTex);

			// HDR texture
			glTextureStorage2D(mainTex, 1, GL_RGBA16F, Window::Get().GetWidth(), Window::Get().GetHeight());

			glTextureParameteri(mainTex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTextureParameteri(mainTex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTextureParameteri(mainTex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTextureParameteri(mainTex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glNamedFramebufferTexture(main, GL_COLOR_ATTACHMENT0, mainTex, 0);	// we only need a color buffer
		}

		// Render buffer:
		glNamedRenderbufferStorage(mainRB, GL_DEPTH24_STENCIL8, Window::Get().GetWidth(), Window::Get().GetHeight());
		glNamedFramebufferRenderbuffer(main, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mainRB); // If I don't do this I get an error

		auto status = glCheckNamedFramebufferStatus(mainRB, GL_FRAMEBUFFER);

		if (status != GL_FRAMEBUFFER_COMPLETE)
			NG_ERROR("Framebuffer incomplete: {}", status);
	}

	// If the size of the Window::Get() changes
	void Reset() {
		glDeleteTextures(1, &mainTex);

		CreateTexturesRBs();

		// bloomTex:
		bloomTexSize = glm::ivec2(Window::Get().GetWidth(), Window::Get().GetHeight()) / 2;
		bloomTexSize += glm::ivec2(16 - (bloomTexSize.x % 16), 16 - (bloomTexSize.y % 16));

		bloomMipCount = (unsigned int)glm::log2(glm::min(Window::Get().GetWidth(), Window::Get().GetHeight())) - 4; // don't want mips of like 1x1
	}

	void size_callback(uint32_t width, uint32_t height) noexcept;

	void Setup() {
		Window::Get().GetFunctions().WindowResizeFn = size_callback;

		// set up the VBO and VAO
		const float frameBufferVertices[] = {
			// positions   // texCoords
			-1.0f,  1.0f,  0.0f, 1.0f,
			-1.0f, -1.0f,  0.0f, 0.0f,
			 1.0f, -1.0f,  1.0f, 0.0f,

			-1.0f,  1.0f,  0.0f, 1.0f,
			 1.0f, -1.0f,  1.0f, 0.0f,
			 1.0f,  1.0f,  1.0f, 1.0f
		};
		VAO = new VertexArray();
		VBO = new VertexBuffer(sizeof(frameBufferVertices), frameBufferVertices);
		BufferLayout bl;
		bl.Push<float>(2); // position
		bl.Push<float>(2); // texCoords
		VAO->AddBuffer(*VBO, bl);

		// actually make all the framebuffers
		{
			// configure main post-processing framebuffer
			glCreateFramebuffers(1, &main);

			glCreateRenderbuffers(1, &mainRB);

#if ENABLE_BLOOM
			bloomTexSize = glm::ivec2(Window::Get().GetWidth(), Window::Get().GetHeight()) / 2;
			bloomTexSize += glm::ivec2(16 - (bloomTexSize.x % 16), 16 - (bloomTexSize.y % 16));

			bloomMipCount = (unsigned int)glm::floor(glm::log2(glm::min(Window::Get().GetWidth(), Window::Get().GetHeight()))) - 4; // don't want mips of like 1x1
#endif
		}
		CreateTexturesRBs();

		// set up the shader
		PostProcessingShader = new Shader("src/Shaders/FrameBuffer.vert", "src/Shaders/PostProcessing.frag");

		PostProcessingShader->Use();

		// the screen texture
		PostProcessingShader->SetInt("screen", 0);
	}

	void Delete() {
		delete VBO;
		glDeleteFramebuffers(1, &main);

		glDeleteRenderbuffers(1, &mainRB);
		glDeleteTextures(1, &mainTex);
	}
}

// if the user changes the window size, this function is called
void Framebuffers::size_callback(uint32_t width, uint32_t height) noexcept {
	// make sure the viewport matches the new Window::Get() dimensions
	glViewport(0, 0, width, height);

	Camera::Get().AspectRatio = (float)width / (float)height;

	// main framebuffer updating
	Reset();
}