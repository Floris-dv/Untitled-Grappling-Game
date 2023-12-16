// File for deferred shading, if I need is
#include "pch.h"
#include <glad/glad.h>

unsigned int gBuffer, gPosition, gNormal, gAlbedoSpec;
unsigned int texDepth;

void DeferredShadingSetup() {
	glGenFramebuffers(1, &gBuffer);
	Framebuffers::Bind(gBuffer);

	// position color buffer
	glGenTextures(1, &gPosition);
	glBindTexture(GL_TEXTURE_2D, gPosition);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);
	// normal color buffer
	glGenTextures(1, &gNormal);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);
	// color + specular color buffer
	glGenTextures(1, &gAlbedoSpec);
	glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedoSpec, 0);
	// tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
	unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, attachments);
	// create and attach depth buffer (renderbuffer)
	glGenTextures(1, &texDepth);
	glBindTexture(GL_TEXTURE_2D, texDepth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, DEPTH_MAP_RES, DEPTH_MAP_RES, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texDepth, 0);

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer not complete!" << std::endl;
}

void DeferredShadingDrawFrame() {
	Framebuffers::Bind(gBuffer);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Geometry pass:
	instancedGeomPassShader.use();
	instancedGeomPassShader.setVec3("viewPos", camera.Position);

	// object.DrawInstanced(instancedGeomPassShader, count, true)

	geomPassShader.use();
	glm::mat4 model(1.0f);

	geomPassShader.setMat4("model", model);
	geomPassShader.setInt("material.diffuse0", 0);
	geomPassShader.setInt("material.specular0", 1);
	geomPassShader.setInt("material.normal0", 2);
	geomPassShader.setInt("material.height0", 3);
	geomPassShader.setFloat("height_scale", 0.1f);
	geomPassShader.setVec3("viewPos", camera.Position);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, floorDiff.id);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, floorSpec.id);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, floorNorm.id);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, floorDisplacement.id);

	floorVAO.Bind();
	glDrawArrays(GL_TRIANGLES, 0, 6);
	floorVAO.UnBind();

	// Lighting pass:
	if (SHADOWS) {
		glm::mat4 v = glm::lookAt(lightDir, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

		glm::mat4 p = glm::ortho(-lSS, lSS, -lSS, lSS, near_plane, far_plane);

		dirLightSpaceMatrix = p * v;

		glViewport(0, 0, DEPTH_MAP_RES, DEPTH_MAP_RES);

		Shadows::Bind(Shadows::dirMap, Shadows::Shaders::iDirLight);

		// for the dir light:
		{
			Shadows::Shaders::iDirLight->setMat4("lightSpaceMatrix", dirLightSpaceMatrix);

			// render the backpack
			backpack.DrawInstanced(*Shadows::Shaders::iDirLight, (unsigned int)objectPositions.size(), false);
		}

		glViewport(0, 0, WIDTH, HEIGHT);

		Framebuffers::Bind(Framebuffers::mainTex);

		glActiveTexture(GL_TEXTURE14);
		glBindTexture(GL_TEXTURE_2D, Shadows::dirMap);
	} // SHADOWS

	glDisable(GL_DEPTH_TEST);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gPosition);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);

	Framebuffers::Bind(pingpongFBO[1]);
	glClear(GL_COLOR_BUFFER_BIT);
	Framebuffers::Bind(pingpongFBO[0]);
	glClear(GL_COLOR_BUFFER_BIT);

	lightPassShader.use();
	lightPassShader.setVec3("viewPos", camera.Position);
	if (SHADOWS)
		lightPassShader.setMat4("lightSpaceMatrix", dirLightSpaceMatrix);
	Framebuffers::VAO->Bind();
	glDrawArrays(GL_TRIANGLES, 0, 6);

	bool horizontal = true;
	pointLightPassShader.use();
	pointLightPassShader.setInt("total", pointLightPositions.size());
	pointLightPassShader.setVec3("viewPos", camera.Position);

	for (unsigned int i = 0; i < pointLightPositions.size(); i++)
	{
		// pointLights
		if (i < SHADOWS) {
			glViewport(0, 0, DEPTH_MAP_RES, DEPTH_MAP_RES);
			Shadows::Bind(Shadows::pointMap, Shadows::Shaders::pointLight);

			// set up pointLightSpaceMatrix
			glm::mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, near_plane, far_plane);

			Shadows::Shaders::iPointLight->use();

			pointLightSpaceMatrix[0] = projection *
				glm::lookAt(pointLightPositions[i], pointLightPositions[i] + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
			pointLightSpaceMatrix[1] = projection *
				glm::lookAt(pointLightPositions[i], pointLightPositions[i] + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
			pointLightSpaceMatrix[2] = projection *
				glm::lookAt(pointLightPositions[i], pointLightPositions[i] + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			pointLightSpaceMatrix[3] = projection *
				glm::lookAt(pointLightPositions[i], pointLightPositions[i] + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
			pointLightSpaceMatrix[4] = projection *
				glm::lookAt(pointLightPositions[i], pointLightPositions[i] + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
			pointLightSpaceMatrix[5] = projection *
				glm::lookAt(pointLightPositions[i], pointLightPositions[i] + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f));

			// render the rocks
			Shadows::Shaders::iPointLight->setVec3("lightPos", pointLightPositions[i]);
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
			backpack.DrawInstanced(*Shadows::Shaders::iPointLight, (unsigned int)objectPositions.size(), false);

			glActiveTexture(GL_TEXTURE15);
			glBindTexture(GL_TEXTURE_CUBE_MAP, Shadows::pointMap);

			glViewport(0, 0, WIDTH, HEIGHT);
		}

		Framebuffers::Bind(pingpongFBO[horizontal]);

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, pingpongBuffer[!horizontal]);
		glm::mat4 model(1.0f);
		const float constant = 1.0f;
		const float linear = 0.09f;
		const float quadratic = 0.032f;
		const float lightMax = 1.0f;
		model = glm::translate(model, pointLightPositions[i]);
		model = glm::scale(model, glm::vec3(
			(-linear + std::sqrt(linear * linear - 4.0f * quadratic * (constant - (256.0f / 5.0f) * lightMax)))
			/ (2 * quadratic)));
		pointLightPassShader.use();
		pointLightPassShader.setMat4("model", model);
		pointLightPassShader.setInt("index", pointLightPositions.size() - i);
		pointLightPassShader.setVec3("light.position", pointLightPositions[i]);
		sphere.Draw(pointLightPassShader, false);
		horizontal = !horizontal;
	}

	Framebuffers::Bind(Framebuffers::main);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, pingpongBuffer[!horizontal]);

	glEnable(GL_DEPTH_TEST);
	bloomPassShader.use();
	bloomPassShader.setInt("screen", 0);

	Framebuffers::VAO->Bind();
	glDrawArrays(GL_TRIANGLES, 0, 6);
	Framebuffers::VAO->UnBind();

	glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
	glBlitFramebuffer(0, 0, WIDTH, HEIGHT, 0, 0, WIDTH, HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

	// draw pointlights
	{
		Profiler p("pointLights drawing");

		lightShader.use();
		for (int i = 0; i < pointLightPositions.size(); i++) {
			glm::mat4 model(1.0f);
			model = glm::translate(model, pointLightPositions[i]);
			model = glm::scale(model, glm::vec3(0.1f));
			lightShader.setMat4("model", model);
			lightShader.setVec3("lightColor", 2.0f, 2.0f, 2.0f);
			box.Draw(lightShader, false);
		}
	}

	// Last, draw the skybox:
	{
		Profiler p("skyBox drawing");

		glDisable(GL_CULL_FACE); // disable face culling: this is because the vertex buffer I use (lightVAO) is focused on the outside, not on the inside, and it's not supposed to cull any faces anyway
		glDepthFunc(GL_LEQUAL); // change depth function so depth test passes when values are equal to depth buffer's content (as it's filled with ones as default)
		skyBoxShader.use();

		glm::mat4 unTranslatedView = glm::mat4(glm::mat3(view));

		skyBoxShader.setMat4("ProjunTranslatedView", projection * unTranslatedView);

		glBindTexture(GL_TEXTURE_CUBE_MAP, skyBoxTex);
		box.Draw(skyBoxShader, false);
		glDepthFunc(GL_LESS); // set depth function back to default

		glEnable(GL_CULL_FACE);
	}
}