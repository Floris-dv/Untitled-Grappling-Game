#include "pch.h"
#include "Log.h" // Before because it includes minwindef.h, and that redefines the APIENTRY macro, and all other files have a check with #ifndef APIENTRY

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
#include "Object.h"
#include "Window.h"
#include "Level.h"

#include "Framebuffers.h"

#include "DataBuffers.h"

// DebugBreak header file by Scott Tsai
#include <DebugBreak.h>

// #include "Renderer.h"
// OpenGL
#include <glad/glad.h>

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

	std::vector<std::shared_future<LoadingTexture*>> loadingBluePrintTexture{ StartLoadingTexture("resources/Textures/Blueprint.jpg", TextureType::diffuse) };

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
			skyBoxTex = loadCubeMap(faces, "resources/Textures/Skybox");
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

	// This would be WAY better with #embed
	// Set up the Shaders:
	auto shader = std::make_shared<Shader>("src/Shaders/NoInstance.vert", "src/Shaders/MainFrag.frag");
	auto asteroidShader = std::make_shared<Shader>("src/Shaders/Instanced.vert", "src/Shaders/MainFrag.frag");
	auto lightShader = std::make_shared<Shader>("src/Shaders/LightVert.vert", "src/Shaders/LightFrag.frag");
	auto skyBoxShader = std::make_shared<Shader>("src/Shaders/SkyboxVert.vert", "src/Shaders/SkyboxFrag.frag");;
	Shader postProcessingShader("src/Shaders/Framebuffer.vert", "src/Shaders/PostProcessing.frag");
	Shader bloomPassShader("src/Shaders/Framebuffer.vert", "src/Shaders/Bloompass.frag");

	Shader bloomShader("src/Shaders/Bloom.comp");

	std::shared_ptr<Shader> textureShader = std::make_shared<Shader>("src/Shaders/TextureVert.vert", "src/Shaders/TextureFrag.frag");

	auto blueprintMaterial = std::make_shared<Material>(textureShader, std::move(loadingBluePrintTexture));

	auto skyBoxMaterial = std::make_shared<Material>(skyBoxShader, std::vector<Texture>());

	auto levelMaterial = std::make_shared<Material>(asteroidShader, glm::vec3{ 0.3f, 0.35f, 1.0f }*2.0f, glm::vec3{ 1.0f, 0.5f, 0.5f });

	blueprintMaterial->LoadTextures(true);

	Object<SimpleVertex> blueprintBox(blueprintMaterial, boxVertices, SimpleVertex::Layout);

	auto lightMaterial = std::make_shared<Material>(lightShader, glm::vec3{ 0.0f, 1.0f, 1.0f }, glm::vec3{ 0.0f, 0.0f, 0.0f });

	Object<SimpleVertex> box(lightMaterial, boxVertices, SimpleVertex::Layout);

	Object<MinimalVertex> sphere;
	{
		Timer t("Loading the sphere");
		auto [vertices, indices] = CreateSphere(20, 20);
		sphere = Object(lightMaterial, std::move(vertices), MinimalVertex::Layout, std::move(indices));
	}

	// uniform buffer(s):
	// Matrices uniform block is at binding point 0:
	UniformBuffer matrixUBO(sizeof(glm::mat4) + sizeof(glm::vec4), "Matrices"); // vec4 because of padding
	{
		matrixUBO.SetBlock(*shader);
		matrixUBO.SetBlock(*asteroidShader);
		matrixUBO.SetBlock(*lightShader);
		matrixUBO.SetBlock(*textureShader);
	}

	glm::vec3 lightDir(0.0f, 3.0f, 4.0f);
	lightDir = glm::normalize(lightDir);

	// Lights uniform block is at binding point 1
	UniformBuffer lightsUBO(480, "Lights", lightData.data());
	{
		lightsUBO.SetBlock(*shader);
		lightsUBO.SetBlock(*asteroidShader);
		lightsUBO.SetBlock(*textureShader);
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
		lightShader->Use();
		lightShader->SetVec3("lightColor", 1.0f, 1.0f, 1.0f);

		shader->Use();
		shader->SetFloat("material.shininess", 256.0f);
		shader->SetFloat("spotLight.cutOff", 0.9762960071199334f);
		shader->SetFloat("spotLight.outerCutOff", 0.9659258262890683f);
		shader->SetFloat("far_plane", far_plane);

		asteroidShader->Use();
		asteroidShader->SetFloat("material.shininess", 256.0f);
		asteroidShader->SetFloat("spotLight.cutOff", 0.9762960071199334f);
		asteroidShader->SetFloat("spotLight.outerCutOff", 0.9659258262890683f);
		asteroidShader->SetFloat("far_plane", far_plane);
		asteroidShader->SetFloat("height_scale", 0.1f);

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

	Level level({ 10.0f, 10.0f, 10.0f }, { { 90.0f, 0.0f, -10.0f }, {110.0f, 0.0f, 10.0f } }, { { {0.0f, -20.0f, 0.0f}, { 20.0f, 20.0f, 20.0f }} }, levelMaterial);
	// Level level("Level.dat", &levelMaterial);
	level.Write("Level.dat");

	// only when everything is set up, do this:
	Window::Get().Maximize();

	// capture the mouse: hide it and set it to the center of the screen
	Window::Get().SetCursor(false);

	Window::Get().ResetTime();

	ti.~Timer();

	float lSS = 20.0f;

	float exposure = 1.0f;

	bool normalMappingToggled = false;

	glm::vec3 spherePos(0.0f);

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

	// saber.DoOpenGL();
	while (!Window::Get().ShouldClose())
	{
		StartFrame();
		level.UpdatePhysics(Camera::Get());

		// Set the current framebuffer to the correct one
		Framebuffers::Bind(Framebuffers::main);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		const glm::mat4 VP = Camera::Get().GetVPMatrix();

		// update the UBO's:
		{
			matrixUBO.SetData(0, sizeof(glm::mat4), glm::value_ptr(VP));
			matrixUBO.SetData(64, sizeof(glm::vec3), &Camera::Get().Position);

			lightsUBO.SetData(0, 12, glm::value_ptr(lightDir));
			lightsUBO.SetData(384, 12, glm::value_ptr(Camera::Get().Position)); // Spotlight pos
			lightsUBO.SetData(400, 12, glm::value_ptr(Camera::Get().Front));	// Spotlight dir

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
		level.Render();

		// draw pointlights
		{
			Profiler p("pointLights drawing");

			lightShader->Use();
			for (int i = 0; i < pointLightPositions.size(); i++) {
				glm::mat4 model(1.0f);
				model = glm::translate(model, pointLightPositions[i]);
				model = glm::scale(model, glm::vec3(0.1f));
				lightMaterial->SetColors({ 2.0f, 2.0f, 2.0f }, { 0.0f, 0.0f, 0.0f });
				// renderer.Draw(box, lightShader);
				box.Draw(model, false);
			}
		}
		// draw the asteroids
		{
			Profiler p("asteroid drawing");
			Transform t;

			// renderer.IDraw(rock, asteroidShader, count);
			rock.DrawInstanced(*asteroidShader, count, Camera::Get().GetFrustum(), t);
		}
		// depth shenanigans: has to happen after everything, but before skybox
		{
			float depth;
			// After everything:
			if (Window::Get().GetMouseButtonDown(1)) {
				glReadPixels(Window::Get().GetWidth() / 2, Window::Get().GetHeight() / 2, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);

				float zNorm = 2.0f * depth - 1.0f;
				float zFar = Camera::Get().Options.ZFar;
				float zNear = Camera::Get().Options.ZNear;
				float zView = -2.0f * zNear * zFar / ((zFar - zNear) * zNorm - zNear - zFar);

				spherePos = Camera::Get().Position + Camera::Get().Front * zView;

				if (zView < 200.0f)
					Camera::Get().m_GrapplingHook.Launch(spherePos);
			}
			else
				Camera::Get().m_GrapplingHook.Release();

			glm::mat4 model(1.0f);
			model = glm::translate(model, spherePos);
			sphere.Draw(model, false);
		}

		// Last, draw the skybox:
		{
			Profiler p("skyBox drawing");

			glDisable(GL_CULL_FACE); // disable face culling: this is because the vertex buffer I use (lightVAO) is focused on the outside, not on the inside, and it's not supposed to cull any faces anyway
			glDepthFunc(GL_LEQUAL); // change depth function so depth test passes when values are equal to depth buffer's content (as it's filled with ones as default)
			skyBoxShader->Use();

			glm::mat4 unTranslatedView(glm::mat3(Camera::Get().GetViewMatrix()));

			skyBoxShader->SetMat4("ProjunTranslatedView", Camera::Get().GetProjMatrix() * unTranslatedView);

			skyBoxShader->SetInt("skyBox", 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_CUBE_MAP, skyBoxTex);

			// renderer.Draw(box, skyBoxShader);
			box.Draw(skyBoxMaterial.get(), {}, false);
			glDepthFunc(GL_LESS); // set depth function back to default

			glEnable(GL_CULL_FACE);
		}
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

		Framebuffers::Draw(exposure, Framebuffers::mainTex, bloomRTs[2]);
		EndFrame();
	}

	Destroy();
}