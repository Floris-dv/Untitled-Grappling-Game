#include "pch.h"
#include "Renderer.h"

unsigned int Renderer::boundFBO = 0;
VertexArray Renderer::VAO;
VertexBuffer Renderer::VBO;
Shader* Renderer::PostProcessingShader;

const float frameBufferVertices[] = {
	// positions   // texCoords
	-1.0f,  1.0f,  0.0f, 1.0f,
	-1.0f, -1.0f,  0.0f, 0.0f,
	 1.0f, -1.0f,  1.0f, 0.0f,

	-1.0f,  1.0f,  0.0f, 1.0f,
	 1.0f, -1.0f,  1.0f, 0.0f,
	 1.0f,  1.0f,  1.0f, 1.0f
};

Renderer::Renderer(unsigned int width, unsigned int height) : width(width), height(height), mainFBO(0), mainRB(0), mainTex(0), pingpongFBOs{ 0, 0 }, pingpongTexs{ 0, 0 }
{
	CreateFBOs();
	CreateTextures();
	if (!VAO.IsValid()) {
		VAO = VertexArray();
		VBO = VertexBuffer(sizeof(frameBufferVertices), frameBufferVertices);
		BufferLayout bl{ {{GL_FLOAT, 2}, {GL_FLOAT, 2}} };
		VAO.AddBuffer(VBO, bl);
		PostProcessingShader = new Shader("Shaders/FrameBuffer.vert", "Shaders/PostProcessing.frag");

		PostProcessingShader->use();

		// the screen texture
		PostProcessingShader->setInt("screen", 0);
	}
}

void Renderer::Bind(GLuint FBO)
{
	if (boundFBO != FBO) {
		boundFBO = FBO;
		glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	}
}

void Renderer::CreateTextures()
{
	Bind(mainFBO);
	// mainTex:
	{
		glGenTextures(1, &mainTex);
		glBindTexture(GL_TEXTURE_2D, mainTex);
#if ENABLE_HDR
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, width, height);
#else
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA, width, height);
#endif

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mainTex, 0);
	}

	// mainRB:
	{
		glGenRenderbuffers(1, &mainRB);
		glBindRenderbuffer(GL_RENDERBUFFER, mainRB);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mainRB);
	}

	const float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f }; // to be able to set the wrapping mode to CLAMP_TO_BORDER

	// shadowDirMap
	{
		glGenTextures(1, &shadowDirMap);
		glBindTexture(GL_TEXTURE_2D, shadowDirMap);

		glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT, DEPTH_MAP_RES, DEPTH_MAP_RES);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	}

	// shadowPointMap
	{
		glGenTextures(1, &shadowPointMap);
		glBindTexture(GL_TEXTURE_CUBE_MAP, shadowPointMap);

		for (unsigned int i = 0; i < 6; i++)
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 1, GL_DEPTH_COMPONENT, DEPTH_MAP_RES, DEPTH_MAP_RES, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL); // has to be glTexImage2D: glTexStorage2D doesn't work with cube maps

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
		glTexParameterfv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BORDER_COLOR, borderColor);
	}

	// pingpongTexs:
	{
		glGenTextures(2, pingpongTexs);
		for (int i = 0; i < 2; ++i) {
			Bind(pingpongFBOs[i]);
			glBindTexture(GL_TEXTURE_2D, pingpongTexs[i]);
#if ENABLE_HDR
			glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, width, height);
#else
			glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA, width, height);
#endif
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongTexs[i], 0);
		}
	}
}

void Renderer::DestroyTextures()
{
	glDeleteTextures(1, &mainTex);

	glDeleteRenderbuffers(1, &mainRB);

	glDeleteTextures(2, pingpongTexs);
}

void Renderer::CreateFBOs()
{
	glGenFramebuffers(1, &mainFBO);

	glGenFramebuffers(1, &shadowFBO);

	glGenFramebuffers(2, pingpongFBOs);
}

void Renderer::DestroyFBOs()
{
	glDeleteFramebuffers(1, &mainFBO);

	glDeleteFramebuffers(1, &shadowFBO);

	glDeleteFramebuffers(2, pingpongFBOs);
}

void Renderer::Clear(GLuint FBO, GLbitfield mask)
{
	Bind(FBO);
	glClear(mask);
}

void Renderer::Draw(Object& ob, const Shader& shader)
{
	Bind(mainFBO);
	ob.Draw(shader, true);
}

void Renderer::SetDimensions(unsigned int x, unsigned int y)
{
	width = x;
	height = y;

	// Reload the textures
	DestroyTextures();
	CreateTextures();
}

void Renderer::DrawShadows(Object& ob, const Shader& shader)
{
	Bind(shadowFBO);
	ob.Draw(shader, false);
}

void Renderer::IDraw(Object& ob, const Shader& shader, unsigned int count)
{
	Bind(mainFBO);
	ob.DrawInstanced(shader, true, count);
}

void Renderer::IDraw(Model& ob, Shader& shader, unsigned int count)
{
	Bind(mainFBO);
	ob.DrawInstanced(shader, true, count);
}

void Renderer::BindShadow(GLuint depthMap)
{
	Bind(shadowFBO);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthMap, 0);

	glClear(GL_DEPTH_BUFFER_BIT);
}

void Renderer::IDrawShadows(Object& ob, const Shader& shader, unsigned int count)
{
	Bind(shadowFBO);

	ob.DrawInstanced(shader, false, count);
}

void Renderer::Finalize() {
	VAO.Bind();
	PostProcessingShader->setInt("screen", 0);

	Bind(0);
	glClear(GL_COLOR_BUFFER_BIT);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mainTex);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glEnable(GL_DEPTH_TEST);

	VAO.UnBind();
}