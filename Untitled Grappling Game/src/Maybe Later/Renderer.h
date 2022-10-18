#pragma once
#include "DataBuffers.h"
#include "Model.h"
#include "Object.h"

class Renderer
{
	static unsigned int boundFBO;
	static VertexArray VAO;
	static VertexBuffer VBO;
	// needs to be a pointer as it needs to be initialized as 0
	static Shader* PostProcessingShader;

	GLuint mainFBO;
	GLuint mainTex;
	GLuint mainRB; // depth & stencil

	GLuint shadowFBO;
	GLuint shadowDirMap;
	GLuint shadowPointMap;

	GLuint pingpongFBOs[2];
	GLuint pingpongTexs[2]; // Same width/height as the screen

	unsigned int width, height;

	void CreateTextures(); // also creates the RBs
	void DestroyTextures(); // also destroys the RBs

	void CreateFBOs();
	void DestroyFBOs();

public:
	Renderer(unsigned int width, unsigned int height);

	void Bind(GLuint FBO);

	void BindShadow(GLuint depthMap);
	void BindMain() { Bind(mainFBO); }

	void SetDimensions(unsigned int x, unsigned int y);

	void Clear(GLuint FBO, GLbitfield mask);
	void ClearMain() { Clear(mainFBO, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); }

	void Draw(Object& ob, const Shader& shader);
	void IDraw(Object& ob, const Shader& shader, unsigned int count = 0); // instanced
	void IDraw(Model& ob, Shader& shader, unsigned int count = 0); // instanced

	void DrawShadows(Object& ob, const Shader& shader);
	void IDrawShadows(Object& ob, const Shader& shader, unsigned int count = 0); // instanced

	void Finalize();
};
