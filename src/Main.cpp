#include "pch.h"
// OpenGL
#include <glad/glad.h>

// Data file:
#include "VertexData.h"

// vendor files
// stb_image
#include <stb_image.h>
// glm
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>
// imgui
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

// own files
#include "Shader.h"
#include "Camera.h"
#include "Model.h"
#include "Timer.h"
#include "Settings.h"
#include "Setup.h"
#include "Shadows.h"
#include "Object.h"
#include "Log.h"
#include "Window.h"

#include "Framebuffers.h"

#include "DataBuffers.h"

// DebugBreak header file by Scott Tsai
#include <DebugBreak.h>

// #include "Renderer.h"

extern float now;

float Surface = .48f;

extern bool showDepthMap;

unsigned int loadCubeMap(std::array<std::string, 6> faces, std::string directory) {
#if QUICK_LOADING
	std::array<std::future<LoadingTexture*>, 6> futures;

	for (int i = 0; i < 6; i++) {
		futures[i] = std::async(std::launch::async, [](std::string fname, std::string directory, TextureType type) {
			std::string filename = directory + '/' + fname;
			// if it's not heap allocated, the ret_text will delete it's data, and then it's useless
			LoadingTexture* t = new LoadingTexture(filename, type);

			return t;
			},
			faces[i], directory, TextureType::unknown
				);
	}
#endif

	unsigned int Map;

	glGenTextures(1, &Map);
	glBindTexture(GL_TEXTURE_CUBE_MAP, Map);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

#if QUICK_LOADING
	for (int i = 0; i < 6; i++) {
		try {
			LoadingTexture* t = futures[i].get();
			if (t == nullptr)
				NG_ERROR("Future got no result!");

			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
				0, t->Format, t->Width, t->Height, 0, GL_RGB, GL_UNSIGNED_BYTE, t->read());

			delete t; // the deletor calls stbi_image_free
		}
		catch (const std::exception& e) { NG_ERROR(e.what()); }
	}
#else
	for (int i = 0; i < 6; i++) {
		std::string filename = directory + '/' + faces[i];
		int x, y, channels;
		unsigned char* data = stbi_load(filename.c_str(), &x, &y, &channels, 0);
		unsigned int format;

		if (channels == 1)
			format = GL_RED;
		else if (channels == 3)
			format = GL_RGB;
		else if (channels == 4)
			format = GL_RGBA;
		else {
			std::cerr << "Texture in file " + filename + " is weird: has " + std::to_string(channels) + " amound of channels, instead of the normal 1, 3, or 4" << std::endl;
			debug_break();
		}

		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
			0, format, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		stbi_image_free(data);
	}
#endif

	return Map;
}

enum class BloomMode {
	PreFilter = 0,
	DownSample,
	UpSample_First,
	UpSample
};

int main() {
	Timer ti("Whole setup");

	Log::Init();

#if !QUICK_LOADING
	// set up everything:
	Setup();
#endif

	// load models
	stbi_set_flip_vertically_on_load(true);

	Model rock("resources/rock/rock.obj", true, false);
	Model backpack("resources/backpack/backpack.obj", true, false);
	Model saber("resources/saber/lightsaber.fbx");

	std::vector<std::shared_future<LoadingTexture*>> floorTextures = {
		StartLoadingTexture("Textures/FloorDiffuse.jpg", TextureType::diffuse),

		StartLoadingTexture("Textures/FloorSpecular.png", TextureType::specular),
		StartLoadingTexture("Textures/FloorNormal.jpg", TextureType::normal),

		StartLoadingTexture("Textures/FloorDisplacement.jpg", TextureType::height)
	};

	std::vector<std::shared_future<LoadingTexture*>> bluePrintTexture{ StartLoadingTexture("Textures/Blueprint.jpg", TextureType::diffuse) };

#if QUICK_LOADING
	Setup();
#endif

	// Renderer renderer(WIDTH, HEIGHT);

	std::array<glm::vec3, 4> pointLightPositions = {
	  glm::vec3(0.7f,  -6.8f,  5.0f),
	  glm::vec3(2.3f, -0.5f, -3.3f),
	  glm::vec3(-1.3f,  0.4f,  2.0f),
	  glm::vec3(0.0f,  0.0f, -3.0f)
	};

	BufferLayout boxLayout;
	boxLayout.Push<float>(3); // position
	boxLayout.Push<float>(3); // normal
	boxLayout.Push<float>(2); // texCoords

	unsigned int skyBoxTex;
	{
		const std::array<std::string, 6> faces = {
			"right.jpg",
			"left.jpg",
			"top.jpg",
			"bottom.jpg",
			"front.jpg",
			"back.jpg"
		};

		{
			Timer t("Cube Map");
			skyBoxTex = loadCubeMap(faces, "Textures/Skybox");
		}
	}

	VertexArray floorVAO;
	VertexBuffer floorVBO(sizeof(floorVertices), floorVertices.data());
	{
		BufferLayout floorBL{ {
			{GL_FLOAT, 3},
			{GL_FLOAT, 3},
			{GL_FLOAT, 2},
			{GL_FLOAT, 3},
			{GL_FLOAT, 3},
			}
		};

		floorVAO.AddBuffer(floorVBO, floorBL);
	}

	// Set up the shaders:
	Shader shader("Shaders/Backpack.vert", "Shaders/Backpack.frag");
	Shader asteroidShader("Shaders/Asteroids.vert", "Shaders/Backpack.frag");
	Shader lightShader("Shaders/Light.vert", "Shaders/Light.frag");
	Shader skyBoxShader("Shaders/Skybox.vert", "Shaders/Skybox.frag");
	Shader normalShader("Shaders/ShowNormals.vert", "Shaders/Light.frag", "Shaders/ShowNormals.geom");
	Shader instancedNormalShading("Shaders/InstancedNormalShowing.vert", "Shaders/Light.frag", "Shaders/ShowNormals.geom");
	Shader postProcessingShader("Shaders/Framebuffer.vert", "Shaders/PostProcessing.frag");
	Shader blurShader("Shaders/Framebuffer.vert", "Shaders/Blur.frag");
	Shader geomPassShader("Shaders/GBuffer.vert", "Shaders/GBuffer.frag");
	Shader instancedGeomPassShader("Shaders/InstancedGBuffer.vert", "Shaders/GBuffer.frag");

	Shader lightPassShader("Shaders/FrameBuffer.vert", "Shaders/LightPass.frag");
	Shader pointLightPassShader("Shaders/Light.vert", "Shaders/PointlightPass.frag");
	Shader bloomPassShader("Shaders/Framebuffer.vert", "Shaders/Bloompass.frag");

	Shader bloomShader("Shaders/Bloom.comp");

	Shader textureShader("Shaders/Texture.vert", "Shaders/Texture.frag");

	Material blueprintMaterial(&textureShader, std::move(bluePrintTexture));

	blueprintMaterial.LoadTextures(true);

	Object<SimpleVertex> blueprintBox(&blueprintMaterial, boxVertices, SimpleVertex::Layout);

	Material lightMaterial(&lightShader, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f });

	Object<SimpleVertex> box(&lightMaterial, boxVertices, SimpleVertex::Layout);

	Object<MinimalVertex> sphere;
	{
		Timer t("Loading the sphere");
		auto [vertices, indices] = CreateSphere(20, 20);
		sphere = Object(&lightMaterial, std::move(vertices), MinimalVertex::Layout, std::move(indices));
	}

	Object<MinimalVertex> map;
	{
		Timer t("Generating the mesh");
		auto mapVertices = GenerateMesh(200, 10, 200);
		map = Object(&lightMaterial, std::move(mapVertices), MinimalVertex::Layout);
	}

	// uniform buffer(s):
	// Matrices uniform block is at binding point 0:
	UniformBuffer viewProjUBO(sizeof(glm::mat4), "Matrices");
	{
		viewProjUBO.SetBlock(shader);
		viewProjUBO.SetBlock(asteroidShader);
		viewProjUBO.SetBlock(lightShader);
		viewProjUBO.SetBlock(geomPassShader);
		viewProjUBO.SetBlock(instancedGeomPassShader);
		viewProjUBO.SetBlock(textureShader);
	}

	glm::vec3 lightDir(0.0f, 3.0f, 4.0f);

	// Lights uniform block is at binding point 1
	UniformBuffer lightsUBO(480, "Lights", lightData.data());
	{
		lightsUBO.SetBlock(shader);
		lightsUBO.SetBlock(asteroidShader);
		lightsUBO.SetBlock(lightPassShader);
		lightsUBO.SetBlock(textureShader);
	}

	bool normalsToggled = false;
	float normalMagnitude = 0.1f;
	float average = 0.0f;

	unsigned int count = 1750;

	bool multiSamplingToggled = true;

	const float near_plane = 1.0f;
	const float far_plane = 2500.0f;

	// set shader uniforms
	{
		normalShader.Use();
		normalShader.SetFloat("Magnitude", normalMagnitude);
		normalShader.SetVec3("lightColor", 1.0f, 1.0f, 0.0f);

		instancedNormalShading.Use();
		instancedNormalShading.SetFloat("Magnitude", normalMagnitude);
		instancedNormalShading.SetVec3("lightColor", 1.0f, 1.0f, 0.0f);

		lightShader.Use();
		lightShader.SetVec3("lightColor", 1.0f, 1.0f, 1.0f);

		shader.Use();
		shader.SetFloat("material.shininess", 256.0f);
		shader.SetFloat("spotLight.cutOff", 0.9762960071199334f);
		shader.SetFloat("spotLight.outerCutOff", 0.9659258262890683f);
		shader.SetInt("shadowMapDir", 14);
		shader.SetInt("shadowMapPoint", 15);
		shader.SetFloat("far_plane", far_plane);

		asteroidShader.Use();
		asteroidShader.SetFloat("material.shininess", 256.0f);
		asteroidShader.SetFloat("spotLight.cutOff", 0.9762960071199334f);
		asteroidShader.SetFloat("spotLight.outerCutOff", 0.9659258262890683f);
		asteroidShader.SetInt("shadowMapDir", 14);
		asteroidShader.SetInt("shadowMapPoint", 15);
		asteroidShader.SetFloat("far_plane", far_plane);

		lightPassShader.Use();
		lightPassShader.SetInt("gPosition", 0);
		lightPassShader.SetInt("gNormal", 1);
		lightPassShader.SetInt("gAlbedoSpec", 2);
		lightPassShader.SetFloat("spotLight.cutOff", 0.9762960071199334f);
		lightPassShader.SetFloat("spotLight.outerCutOff", 0.9659258262890683f);
		lightPassShader.SetFloat("shininess", 32.0f);
		lightPassShader.SetInt("shadowMapDir", 14);

		pointLightPassShader.Use();
		pointLightPassShader.SetInt("gPosition", 0);
		pointLightPassShader.SetInt("gNormal", 1);
		pointLightPassShader.SetInt("gAlbedoSpec", 2);
		pointLightPassShader.SetInt("addTo", 3);
		pointLightPassShader.SetFloat("shininess", 32.0f);
		pointLightPassShader.SetVec3("light.ambient", 0.05f, 0.05f, 0.05f);
		pointLightPassShader.SetVec3("light.diffuse", 1.0f, 1.0f, 1.0f);
		pointLightPassShader.SetVec3("light.specular", 0.1f, 0.1f, 0.1f);
		pointLightPassShader.SetFloat("light.linear", 0.09f);
		pointLightPassShader.SetFloat("light.quadratic", 0.032f);
		pointLightPassShader.SetFloat("far_plane", far_plane);
		pointLightPassShader.SetInt("shadowMapPoint", 15);

		instancedGeomPassShader.Use();
		instancedGeomPassShader.SetFloat("height_scale", 0.1f);

		geomPassShader.Use();
		geomPassShader.SetFloat("height_scale", 0.1f);
		geomPassShader.SetFloat("material.shininess", 256.0f);

		bloomPassShader.Use();
		bloomPassShader.SetInt("screen", 0);
	}

	Shader::ClearShaderCache();

	const unsigned int amount = 100000;
	glm::mat4* modelMatrices = new glm::mat4[amount];

	srand((unsigned int)std::chrono::steady_clock::now().time_since_epoch().count()); // initialize random seed
	GenerateModelMatricesInRing(amount, modelMatrices, 150.0f, 75.0f);

	VertexBuffer asteroidPosVBO(amount * sizeof(glm::mat4), &modelMatrices[0]);
	{
		BufferLayout bl(5);

		bl.Push<glm::mat4>(1);
		bl.SetInstanced();
		{
			Timer t("OpenGL Loading rock");
			rock.DoOpenGL();
		}

		rock.getMesh().VAO.AddBuffer(asteroidPosVBO, bl);
	}

	std::array<glm::vec3, 9> objectPositions = {
		glm::vec3(-3.0, -0.5, -3.0),
		glm::vec3(0.0, -0.5, -3.0),
		glm::vec3(3.0, -0.5, -3.0),
		glm::vec3(-3.0, -0.5, 0.0),
		glm::vec3(0.0, -0.5, 0.0),
		glm::vec3(3.0, -0.5, 0.0),
		glm::vec3(-3.0, -0.5, 3.0),
		glm::vec3(0.0, -0.5, 3.0),
		glm::vec3(3.0, -0.5, 3.0),
	};

	glm::mat4* backpackMatrices = new glm::mat4[objectPositions.size()];
	for (unsigned int i = 0; i < objectPositions.size(); i++) {
		glm::mat4 model = glm::mat4(1.0f);

		model = glm::translate(model, objectPositions[i]);

		model = glm::scale(model, glm::vec3(.5f));

		backpackMatrices[i] = model;
	}

	VertexBuffer backpackPositions((unsigned int)objectPositions.size() * sizeof(glm::mat4), backpackMatrices);

	{
		BufferLayout bl(5);

		bl.Push<glm::mat4>(1);
		bl.SetInstanced();
		{
			Timer t("OpenGL Loading backpack");
			backpack.DoOpenGL();
		}

		backpack.getMesh().VAO.AddBuffer(backpackPositions, bl);
	}
	Material m(&shader, std::move(floorTextures));
	Object<Vertex> floor(&m, floorVertices, Vertex::Layout);
	floor.DoOpenGL(true);

	// only when everything is set up, do this:
	Window::Get().Maximize();

	// capture the mouse: hide it and set it to the center of the screen
	Window::Get().SetCursor(false);

	Window::Get().ResetTime();

	ti.~Timer();

	float lSS = 20.0f;

	float exposure = 1.0f;

	bool normalMappingToggled = false;

#if ENABLE_SHADOWS
	int SHADOWS = 1;
#else
	const bool SHADOWS = false;
#endif

#if ENABLE_BLOOM
	Framebuffers::Bind(0);

	bool bloomToggled = false;
	bool prevBloomToggled = false;

	Framebuffers::Bind(0);

	float threshold = 0.8f;
	float knee = 0.1f;

	unsigned int pingpongFBO[2];
	unsigned int pingpongBuffer[2];
	glCreateFramebuffers(2, pingpongFBO);
	glCreateTextures(GL_TEXTURE_2D, 2, pingpongBuffer);
	for (unsigned int i = 0; i < 2; i++)
	{
		glTextureStorage2D(pingpongBuffer[i], 1, GL_RGBA16F, Window::Get().GetWidth(), Window::Get().GetHeight());
		glTextureParameteri(pingpongBuffer[i], GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTextureParameteri(pingpongBuffer[i], GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTextureParameteri(pingpongBuffer[i], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(pingpongBuffer[i], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glNamedFramebufferTexture(pingpongFBO[i], GL_COLOR_ATTACHMENT0, pingpongBuffer[i], 0);
	}

	unsigned int bloomRTs[3];
	glCreateTextures(GL_TEXTURE_2D, 3, bloomRTs);
	for (unsigned int i = 0; i < 3; i++) {
		glTextureStorage2D(bloomRTs[i], Framebuffers::bloomMipCount, GL_RGBA16F, Framebuffers::bloomTexSize.x, Framebuffers::bloomTexSize.y);

		glGenerateTextureMipmap(bloomRTs[i]);
		glTextureParameteri(bloomRTs[i], GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTextureParameteri(bloomRTs[i], GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTextureParameteri(bloomRTs[i], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(bloomRTs[i], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
#endif

	pointLightPassShader.Use();
	pointLightPassShader.SetVec2("screenSize", (float)Window::Get().GetWidth(), (float)Window::Get().GetHeight());
	saber.DoOpenGL();
	while (!Window::Get().ShouldClose())
	{
		StartFrame();

#if ENABLE_SHADOWS
		ImGui::DragInt("Number of shadows", &SHADOWS, 1.0f, 0, pointLightPositions.size());

		glm::mat4 dirLightSpaceMatrix;
		std::array<glm::mat4, 6> pointLightSpaceMatrix;

		// generate the depth map:
		if (SHADOWS)
		{
			glm::mat4 view = glm::lookAt(lightDir, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

			glm::mat4 projection = glm::ortho(-lSS, lSS, -lSS, lSS, near_plane, far_plane);

			dirLightSpaceMatrix = projection * view;

			glViewport(0, 0, DEPTH_MAP_RES, DEPTH_MAP_RES);

			Shadows::Bind(Shadows::dirMap, Shadows::Shaders::iDirLight);

			// for the dir light:
			{
				// render the rock

				Shadows::Shaders::iDirLight->use();

				Shadows::Shaders::iDirLight->setMat4("lightSpaceMatrix", dirLightSpaceMatrix);

				rock.DrawInstanced(*Shadows::Shaders::iDirLight, count, false);

				// render the backpack
				backpack.DrawInstanced(*Shadows::Shaders::iDirLight, (unsigned int)objectPositions.size(), false);

				glActiveTexture(GL_TEXTURE14);
				glBindTexture(GL_TEXTURE_2D, Shadows::dirMap);
			}

			Shadows::Bind(Shadows::pointMap, Shadows::Shaders::iPointLight);

			// set up pointLightSpaceMatrix
			projection = glm::perspective(glm::radians(90.0f), 1.0f, near_plane, far_plane);

			pointLightSpaceMatrix[0] = projection *
				glm::lookAt(pointLightPositions[0], pointLightPositions[0] + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
			pointLightSpaceMatrix[1] = projection *
				glm::lookAt(pointLightPositions[0], pointLightPositions[0] + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
			pointLightSpaceMatrix[2] = projection *
				glm::lookAt(pointLightPositions[0], pointLightPositions[0] + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			pointLightSpaceMatrix[3] = projection *
				glm::lookAt(pointLightPositions[0], pointLightPositions[0] + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
			pointLightSpaceMatrix[4] = projection *
				glm::lookAt(pointLightPositions[0], pointLightPositions[0] + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
			pointLightSpaceMatrix[5] = projection *
				glm::lookAt(pointLightPositions[0], pointLightPositions[0] + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f));

			{
				// render the rocks
				glm::mat4 model = glm::mat4(1.0f);

				Shadows::Shaders::iPointLight->setVec3("lightPos", pointLightPositions[0]);
				Shadows::Shaders::iPointLight->setFloat("far_plane", far_plane);

				for (int i = 0; i < 6; i++)
					Shadows::Shaders::iPointLight->setMat4("lightSpaceMatrices[" + std::to_string(i) + "]", pointLightSpaceMatrix[i]);

				/*
				for (int i = 0; i < count; i++) {
					pointLightShadowShader.setMat4("model", modelMatrices[i]);
					rock.Draw(pointLightShadowShader);
				}
				*/
				// asteroidShadowShader.setMat4("lightSpaceMatrix", pointLightSpaceMatrix);
				rock.DrawInstanced(*Shadows::Shaders::iPointLight, count, false);

				// render the backpack
				backpack.DrawInstanced(*Shadows::Shaders::iPointLight, objectPositions.size(), false);
			}

			glActiveTexture(GL_TEXTURE15);
			glBindTexture(GL_TEXTURE_CUBE_MAP, Shadows::pointMap);

			glViewport(0, 0, WIDTH, HEIGHT);
		}
#endif

		// Set the current framebuffer to the correct one
		Framebuffers::Bind(Framebuffers::main);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		const glm::mat4 VP = Camera::Get().GetVPMatrix();

		// update the UBO's:
		{
			viewProjUBO.SetData(0, 64, glm::value_ptr(VP));

			lightsUBO.SetData(0, 12, glm::value_ptr(lightDir));
			lightsUBO.SetData(384, 12, glm::value_ptr(Camera::Get().m_Position));
			lightsUBO.SetData(400, 12, glm::value_ptr(Camera::Get().m_Front));

			// set light direction
			ImGui::DragFloat3("Light direction", glm::value_ptr(lightDir), 0.1f, -5.0f, 5.0f);

			// set pointlight positions
			for (size_t i = 0; i < pointLightPositions.size(); i++)
				lightsUBO.SetData(64 + i * 80, 12, glm::value_ptr(pointLightPositions[i]));
		}

		// clearing
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		ImGui::DragInt("Count", (int*)&count, 20.0f, 0, amount);

		ImGui::Checkbox("Normal showing", &normalsToggled);

		ImGui::DragFloat("lightSpaceSize", &lSS, 1.0f, 1.0f, 1000.0f);

		for (int i = 0; i < pointLightPositions.size(); i++)
			ImGui::DragFloat3(("Light " + std::to_string(i)).c_str(), glm::value_ptr(pointLightPositions[i]), 0.05f, -100.0f, 100.0f, nullptr, 1);

		ImGui::DragFloat("Magnitude", &normalMagnitude, 0.1f, 0, 100);

		ImGui::NewLine();

		glEnable(GL_DEPTH_TEST);
		// draw normals
		if (normalsToggled)
		{
			Profiler p("normal drawing");
			normalShader.Use();
			normalShader.SetFloat("Magnitude", normalMagnitude);
			normalShader.SetMat4("view", Camera::Get().GetViewMatrix());
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f)); // translate it down so it's at the center of the scene
			model = glm::scale(model, glm::vec3(5.0f));
			normalShader.SetMat4("model", model);

			instancedNormalShading.Use();
			instancedNormalShading.SetFloat("Magnitude", normalMagnitude);
			instancedNormalShading.SetMat4("view", Camera::Get().GetViewMatrix());
			instancedNormalShading.SetMat4("proj", Camera::Get().GetProjMatrix());

			// renderer.IDraw(rock, instancedNormalShading, count);

			// renderer.IDraw(backpack, instancedNormalShading, objectPositions.size());
		}
		// draw pointlights
		{
			Profiler p("pointLights drawing");

			lightShader.Use();
			for (int i = 0; i < pointLightPositions.size(); i++) {
				glm::mat4 model(1.0f);
				model = glm::translate(model, pointLightPositions[i]);
				model = glm::scale(model, glm::vec3(0.1f));
				lightShader.SetMat4("model", model);
				lightMaterial.SetColors({ 2.0f, 2.0f, 2.0f }, { 0.0f, 0.0f, 0.0f });
				// renderer.Draw(box, lightShader);
				box.Draw(lightShader, false);
			}

			glDisable(GL_CULL_FACE);
			lightShader.SetMat4("model", glm::mat4(1.0f));
			// renderer.Draw(map, lightShader);

			lightMaterial.SetColors({ 0.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f });
			// renderer.Draw(sphere, lightShader);
			sphere.Draw(lightShader, false);

			glEnable(GL_CULL_FACE);
		}
		// draw backpack
		{
			Profiler p("backpack drawing");

			shader.Use();
			// pass transformation matrices to the shader
			shader.SetVec3("viewPos", Camera::Get().m_Position);
#if ENABLE_SHADOWS
			if (SHADOWS)
				shader.setMat4("lightSpaceMatrix", dirLightSpaceMatrix);
#endif
			Transform t;
			t.setLocalScale(glm::vec3{ 5.0f });
			// render the loaded model
			shader.SetMat4("model", t.getModelMatrix());
			asteroidShader.Use();
			asteroidShader.SetVec3("viewPos", Camera::Get().m_Position);
			asteroidShader.SetFloat("height_scale", 0.1f);

			// renderer.IDraw(backpack, asteroidShader, objectPositions.size());
			backpack.DrawInstanced(asteroidShader, objectPositions.size(), Camera::Get().GetFrustum(), t);
		}
		// draw the asteroids
		{
			Profiler p("asteroid drawing");
			Transform t;

			// renderer.IDraw(rock, asteroidShader, count);
			rock.DrawInstanced(asteroidShader, count, Camera::Get().GetFrustum(), t);
		}
		// draw the floor
		{
			Profiler p("floor drawing");

			shader.Use();

			shader.SetBool("material.useTex", true);

			glm::mat4 model(1.0f);

			shader.SetMat4("model", model);
			shader.SetVec3("viewPos", Camera::Get().m_Position);
			floor.Draw(true);

			textureShader.Use();

			textureShader.SetMat4("model", model);

			blueprintBox.Draw(true);
		}
		// Last, draw the skybox:
		{
			Profiler p("skyBox drawing");

			glDisable(GL_CULL_FACE); // disable face culling: this is because the vertex buffer I use (lightVAO) is focused on the outside, not on the inside, and it's not supposed to cull any faces anyway
			glDepthFunc(GL_LEQUAL); // change depth function so depth test passes when values are equal to depth buffer's content (as it's filled with ones as default)
			skyBoxShader.Use();

			glm::mat4 unTranslatedView(glm::mat3(Camera::Get().GetViewMatrix()));

			skyBoxShader.SetMat4("ProjunTranslatedView", Camera::Get().GetProjMatrix() * unTranslatedView);

			glBindTexture(GL_TEXTURE_CUBE_MAP, skyBoxTex);

			// renderer.Draw(box, skyBoxShader);
			box.Draw(skyBoxShader, false);
			glDepthFunc(GL_LESS); // set depth function back to default

			glEnable(GL_CULL_FACE);
		}
		shader.Use();

		Transform t;
		t.setLocalPosition({ 20, 0, 0 });
		t.setLocalScale(glm::vec3{ 0.01f });

		shader.SetMat4("model", t.getModelMatrix());
		shader.SetVec3("viewPos", Camera::Get().m_Position);
		glm::mat4 MVP = VP;
		saber.Draw(shader, Camera::Get().GetFrustum(), t);

#if ENABLE_BLOOM
		if (bloomToggled)
		{
			Profiler p("bloom");

			ImGui::SliderFloat("Threshold", &threshold, 0.0f, 20.0f);
			ImGui::SliderFloat("Knee", &knee, 0.0f, 0.1f);
			bloomShader.Use();
			bloomShader.SetInt("o_Image", 0);
			glBindImageTexture(0, bloomRTs[0], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);

			bloomShader.SetVec4("Params", threshold, threshold - knee, knee * 2, .25f / knee);
			bloomShader.SetVec2("LodAndMode", 0.0f, (float)BloomMode::PreFilter);

			glBindTextureUnit(0, Framebuffers::mainTex);
			bloomShader.SetInt("u_Texture", 0);

			glDispatchCompute((GLuint)ceil(Framebuffers::bloomTexSize.x * 0.0625f), (GLuint)ceil(Framebuffers::bloomTexSize.y * 0.0625f), 1);

			// Downsampling:
			for (int currentMip = 1; currentMip < Framebuffers::bloomMipCount; currentMip++) {
				glm::ivec2 mipSize = glm::vec2(Framebuffers::bloomTexSize) * (float)pow(2, -currentMip);

				// Ping
				bloomShader.SetVec2("LodAndMode", currentMip - 1.0f, (float)BloomMode::DownSample);

				glBindImageTexture(0, bloomRTs[1], currentMip, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);

				glBindTextureUnit(0, bloomRTs[0]);
				glDispatchCompute((GLuint)ceil(mipSize.x * 0.0625f), (GLuint)ceil(mipSize.y * 0.0625f), 1);

				// Pong
				bloomShader.SetVec2("LodAndMode", (float)currentMip, (float)BloomMode::DownSample);

				glBindImageTexture(0, bloomRTs[0], currentMip, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);

				glBindTextureUnit(0, bloomRTs[1]);
				glDispatchCompute((GLuint)ceil(mipSize.x * 0.0625f), (GLuint)ceil(mipSize.y * 0.0625f), 1);
			}

			// First upsample
			glBindImageTexture(0, bloomRTs[2], Framebuffers::bloomMipCount - 1, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);

			bloomShader.SetVec2("LodAndMode", float(Framebuffers::bloomMipCount - 2), (float)BloomMode::UpSample_First);

			glBindTextureUnit(0, bloomRTs[0]);

			glm::ivec2 currentMipSize = glm::vec2(Framebuffers::bloomTexSize) * (float)pow(2, -((int)Framebuffers::bloomMipCount - 1));

			glDispatchCompute((GLuint)glm::ceil((float)currentMipSize.x * 0.0625f), (GLuint)glm::ceil((float)currentMipSize.y * 0.0625f), 1);

			bloomShader.SetInt("u_BloomTexture", 1);

			// Rest of the upsamples
			for (int currentMip = Framebuffers::bloomMipCount - 2; currentMip > -1; currentMip--) {
				currentMipSize = glm::vec2(Framebuffers::bloomTexSize) * (float)pow(2, -currentMip);
				glBindImageTexture(0, bloomRTs[2], currentMip, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);

				bloomShader.SetVec2("LodAndMode", (float)currentMip, (float)BloomMode::UpSample);

				glBindTextureUnit(0, bloomRTs[0]);

				glBindTextureUnit(1, bloomRTs[2]);

				glDispatchCompute((GLuint)glm::ceil((float)currentMipSize.x * 0.0625f), (GLuint)glm::ceil((float)currentMipSize.y * 0.0625f), 1);
			}
		}
#endif

		// frameBuffer render:
		ImGui::DragFloat("exposure", &exposure, 0.01f, 0.0f, 10.0f);

#if ENABLE_BLOOM
		ImGui::Checkbox("Bloom", &bloomToggled);
		if (bloomToggled != prevBloomToggled)
		{
			prevBloomToggled = bloomToggled;
			Framebuffers::PostProcessingShader->Use();
			Framebuffers::PostProcessingShader->SetBool("bloom", bloomToggled);
			Framebuffers::PostProcessingShader->SetInt("bloomBlur", 1);

			Framebuffers::Bind(Framebuffers::main);
			GLuint attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
			glDrawBuffers((int)bloomToggled + 1, attachments);
		}
#endif

		// renderer.Finalize();

		ImGui::DragFloat3("Front:", (float*)&Camera::Get().m_Front, 0.0f, -1.0f, 1.0f);

		Framebuffers::Draw(exposure, Framebuffers::mainTex, bloomRTs[2]);
		EndFrame();
	}

	Destroy();
}