#include "pch.h"
#include "Log.h" // Before because it includes minwindef.h, and that redefines the APIENTRY macro, and all other files have a check with #ifndef APIENTRY

// Data file:
#include "VertexData.h"

// vendor files
// stb_image
#include <stb_image.h>
// glm
// #include <glm/gtx/norm.hpp>
// imgui
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include <imguizmo/ImGuizmo.h>

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
#include "SaveFile.h"
#include "GrapplingCamera.h"
#include "EditingCamera.h"

#include "Framebuffers.h"

#include "DataBuffers.h"

// DebugBreak header file by Scott Tsai
#include <DebugBreak.h>

// #include "Renderer.h"
// OpenGL
#include <glad/glad.h>

extern float now;

extern float deltaTime;

float Surface = .48f;

extern bool showDepthMap;

unsigned int loadCubeMap(std::array<std::string, 6> faces, std::string directory) {
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

	unsigned int Map;

	glGenTextures(1, &Map);
	glBindTexture(GL_TEXTURE_CUBE_MAP, Map);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	for (int i = 0; i < 6; i++) {
		try {
			LoadingTexture* t = futures[i].get();
			if (t == nullptr) {
				NG_ERROR("Future got no result!");
				continue;
			}

			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
				0, t->Format, t->Width, t->Height, 0, GL_RGB, GL_UNSIGNED_BYTE, t->read());

			delete t; // the deletor calls stbi_image_free
		}
		catch (const std::exception& e) { NG_ERROR(e.what()); }
	}

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

	// This would be WAY better with #embed
	// Set up the Shaders:
	auto shader = std::make_shared<Shader>("src/Shaders/NoInstance.vert", "src/Shaders/MainFrag.frag");
	auto asteroidShader = std::make_shared<Shader>("src/Shaders/Instanced.vert", "src/Shaders/MainFrag.frag");
	auto lightShader = std::make_shared<Shader>("src/Shaders/LightVert.vert", "src/Shaders/LightFrag.frag");
	Shader lightInstanced("src/Shaders/LightInstanced.vert", "src/Shaders/LightFrag.frag");
	Shader bloomShader("src/Shaders/Bloom.comp");
	Shader crosshairShader("src/Shaders/Crosshair.vert", "src/Shaders/Crosshair.frag");

	auto crosshairFuture = StartLoadingTexture("resources/Textures/Crosshair.png");

	auto levelMaterial = std::make_shared<Material>(asteroidShader, glm::vec3{ 0.3f, 0.35f, 1.0f }*2.0f, glm::vec3{ 1.0f, 0.5f, 0.5f });

	auto mainMaterial = std::make_shared<Material>(shader, glm::vec3{ 0.3f, 0.35f, 1.0f }, glm::vec3{ 0.1f, 0.05f, 0.05f });

	auto finishMaterial = std::make_shared<Material>(shader, glm::vec3{ 1.0f, 0.35, 0.3f }, glm::vec3{ 0.05f, 0.05f, 0.1f });

	auto lightMaterial = std::make_shared<Material>(lightShader, glm::vec3{ 0.0f, 1.0f, 1.0f }, glm::vec3{ 0.0f, 0.0f, 0.0f });

	Object<SimpleVertex> box(lightMaterial, boxVertices, SimpleVertex::Layout);

	Object<MinimalVertex> sphere;
	{
		Timer t("Loading the sphere");
		auto [vertices, indices](CreateSphere(20, 20));
		sphere = Object<MinimalVertex>(lightMaterial, vertices, MinimalVertex::Layout, indices);
	}

	// uniform buffer(s):
	// Matrices uniform block is at binding point 0:
	UniformBuffer matrixUBO(sizeof(glm::mat4) + sizeof(glm::vec4), "Matrices"); // vec4 because of padding
	{
		matrixUBO.SetBlock(*shader);
		matrixUBO.SetBlock(*asteroidShader);
		matrixUBO.SetBlock(*lightShader);
		matrixUBO.SetBlock(lightInstanced);
	}

	glm::vec3 lightDir(0.0f, 3.0f, 4.0f);
	lightDir = glm::normalize(lightDir);

	// Lights uniform block is at binding point 1
	UniformBuffer lightsUBO(480, "Lights", lightData.data());
	{
		lightsUBO.SetBlock(*shader);
		lightsUBO.SetBlock(*asteroidShader);
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
	}

	Texture crosshair;
	// Crosshair
	{
		crosshair = crosshairFuture.get()->Finish();
		crosshairShader.Use();
		crosshairShader.SetInt("crosshairTex", 7);
	}

	Shader::ClearShaderCache();

	glm::vec3 line[] = { {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f} };
	VertexBuffer lineVBO(2 * sizeof(glm::vec3), line);
	VertexBuffer linePosVBO;
	VertexArray lineVAO;
	lineVAO.AddBuffer(lineVBO, { { { GL_FLOAT, 3 } } });

	{
		const int lineCount = 10000;

		glm::mat4* linePositions = new glm::mat4[2 * lineCount + 1];

		for (int i = -lineCount / 2; i <= lineCount / 2; i++) {
			glm::mat4& model = linePositions[i + lineCount / 2];

			model = glm::scale(glm::translate(glm::mat4(1.0f), { 0.0f, 0.0f, (float)i }), { (float)lineCount, (float)lineCount, (float)lineCount });
		}

		for (int i = -lineCount / 2; i <= lineCount / 2; i++) {
			glm::mat4& model = linePositions[i + lineCount / 2 + lineCount];

			model = glm::scale(glm::rotate(glm::translate(glm::mat4(1.0f), { (float)i, 0.0f, 0.0f }), glm::half_pi<float>(), { 0.0f, 1.0f, 0.0f }), { (float)lineCount, (float)lineCount, (float)lineCount });
		}

		linePosVBO = VertexBuffer(sizeof(linePositions), linePositions);
		lineVAO.AddBuffer(linePosVBO, instanceBufferLayout);

		delete[] linePositions;
	}

	int LevelNr = 1;

	Level level("Levels/Level1.dat", asteroidShader, shader);

	float lSS = 20.0f;

	float exposure = 1.0f;

	bool normalMappingToggled = false;

	glm::vec3 spherePos(0.0f);

#if ENABLE_BLOOM
	Framebuffers::Bind(0);

	bool bloomToggled = true;

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

	Framebuffers::PostProcessingShader->Use();
	Framebuffers::PostProcessingShader->SetBool("bloom", bloomToggled);
	Framebuffers::PostProcessingShader->SetInt("bloomBlur", 1);

	Framebuffers::Bind(Framebuffers::main);
	GLuint attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers((int)bloomToggled + 1, attachments);
#endif

	bool editLevel = false;
	size_t blockEditingIndex = 0;

	{
		auto callbackFn = [&editLevel, &level]() {
			editLevel = !editLevel;
			ImGuizmo::Enable(editLevel);
			if (editLevel) {
				Camera::SetCamera<EditingCamera>(Camera::CameraOptions{ 2.5f, 0.1f, 100.0f }, Window::Get().GetAspectRatio(), glm::vec3{ 0.0f }, glm::vec3{ 0.0f, 1.0f, 0.0f }, 90.0f, 0.0f);
			}
			else {
				level.UpdateInstanceVBO();
				Camera::SetCamera<GrapplingCamera>(20.0f, 1.0f, Camera::CameraOptions{ 2.5f, 0.1f, 100.0f }, Window::Get().GetAspectRatio(), glm::vec3{ 0.0f }, glm::vec3{ 0.0f, 1.0f, 0.0f }, 90.0f, 0.0f);
			}
		};
		Window::Get().SetKey(KEY_Z, callbackFn);

		editLevel = true;
		callbackFn(); // Sets upt the camera
	}

	// only when everything is set up, do this:
	Window::Get().Maximize();

	// capture the mouse: hide it and set it to the center of the screen
	Window::Get().SetCursor(false);

	Window::Get().ResetTime();

	ti.~Timer();

	// saber.DoOpenGL();
	while (!Window::Get().ShouldClose())
	{
		StartFrame();

		// Set the current framebuffer to the correct one
		Framebuffers::Bind(Framebuffers::main);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// const glm::mat4 Proj = editLevel ? glm::ortho<float>(-100.0f, 100.0f, -100.0f, 100.0f, 0.1f, 200.0f) : Camera::Get().GetProjMatrix();
		const glm::mat4 VP = Camera::Get()->GetVPMatrix();

		// update the UBO's:
		{
			matrixUBO.SetData(0, sizeof(glm::mat4), glm::value_ptr(VP));
			matrixUBO.SetData(64, sizeof(glm::vec3), &Camera::Get()->Position);

			lightsUBO.SetData(0, 12, glm::value_ptr(lightDir));
			lightsUBO.SetData(384, 12, glm::value_ptr(Camera::Get()->Position)); // Spotlight pos
			lightsUBO.SetData(400, 12, glm::value_ptr(Camera::Get()->Front));	// Spotlight dir

			// set light direction
			ImGui::DragFloat3("Light direction", glm::value_ptr(lightDir), 0.1f, -5.0f, 5.0f);

			// set pointlight positions
			for (size_t i = 0; i < pointLightPositions.size(); i++)
				lightsUBO.SetData(64 + i * 80, 12, glm::value_ptr(pointLightPositions[i]));
		}
		// clearing
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glEnable(GL_DEPTH_TEST);

		if (editLevel) {
			level.RenderEditingMode(blockEditingIndex);
			level.RenderOneByOne();

			// Draw a grid:
			lightInstanced.Use();
			lightInstanced.SetVec3("material.diff0", { 1.0f, 1.4f, 1.4f });
			lineVAO.Bind();

			glDrawArraysInstanced(GL_LINES, 0, 2, 201);
		}
		else {
			if (level.UpdatePhysics(*(GrapplingCamera*)Camera::Get())) {
				LevelNr++;
				if (LevelNr == 2)
					level = Level("Levels/Level2.dat", asteroidShader, shader);
				if (LevelNr >= 3)
					level = Level("Levels/Level3.dat", asteroidShader, shader);
				Camera::Get()->Reset();
				((GrapplingCamera*)Camera::Get())->Release();
			}
			level.Render();
		}

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

		// depth shenanigans: has to happen after everything, but before skybox
		if (!editLevel)
		{
			float depth;
			// After everything:
			if (Window::Get().GetMouseButtonDown(1)) {
				glReadPixels(Window::Get().GetWidth() / 2, Window::Get().GetHeight() / 2, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);

				float zNorm = 2.0f * depth - 1.0f;
				float zFar = Camera::Get()->Options.ZFar;
				float zNear = Camera::Get()->Options.ZNear;
				float zView = -2.0f * zNear * zFar / ((zFar - zNear) * zNorm - zNear - zFar);

				ImGui::Text("Distance: %f", zView);

				spherePos = Camera::Get()->Position + Camera::Get()->Front * zView;

				if (zView < 80.0f)
					((GrapplingCamera*)Camera::Get())->LaunchAt(spherePos);
			}
			else
				((GrapplingCamera*)Camera::Get())->Release();

			glm::mat4 model(1.0f);
			model = glm::translate(model, ((GrapplingCamera*)Camera::Get())->GrapplingPosition());
			sphere.Draw(model, false);
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

		if (ImGui::Button("Reset"))
			Camera::Get()->Reset();

		// frameBuffer render:
		ImGui::DragFloat("exposure", &exposure, 0.01f, 0.0f, 10.0f);

#if ENABLE_BLOOM
		if (ImGui::Checkbox("Bloom", &bloomToggled))
		{
			Framebuffers::PostProcessingShader->Use();
			Framebuffers::PostProcessingShader->SetBool("bloom", bloomToggled);
			Framebuffers::PostProcessingShader->SetInt("bloomBlur", 1);

			Framebuffers::Bind(Framebuffers::main);
			GLuint attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
			glDrawBuffers((int)bloomToggled + 1, attachments);
		}
#endif
		static float crosshairSize = 0.1f;
		ImGui::DragFloat("Crosshair size", &crosshairSize, 0.01f, 0.0f, 0.5f);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		crosshairShader.Use();
		crosshairShader.SetInt("crosshairTex", 7);
		glBindTextureUnit(7, crosshair.ID);
		crosshairShader.SetVec2("uSize", { crosshairSize / Window::Get().GetAspectRatio(), crosshairSize });

		glDisable(GL_DEPTH_TEST);
		Framebuffers::VAO->Bind();
		glDrawArrays(GL_TRIANGLES, 0, 6);

		//glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);


		// renderer.Finalize();

		Framebuffers::Draw(exposure, Framebuffers::mainTex, bloomRTs[2]);
		EndFrame();
	}


	Destroy();
}