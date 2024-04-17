/*
 * Copyright (C) 2023, Inria
 * GRAPHDECO research group, https://team.inria.fr/graphdeco
 * All rights reserved.
 *
 * This software is free for non-commercial, research and evaluation use 
 * under the terms of the LICENSE.md file.
 *
 * For inquiries contact sibr@inria.fr and/or George.Drettakis@inria.fr
 */



#include "core/graphics/Texture.hpp"
#include "GaussianSurfaceRenderer.hpp"

namespace sibr { 

	GaussianData::GaussianData(int num_gaussians, float* mean_data, float* rot_data, float* scale_data, float* alpha_data, float* color_data)
	{
		_num_gaussians = num_gaussians;
		glCreateBuffers(1, &meanBuffer);
		glCreateBuffers(1, &rotBuffer);
		glCreateBuffers(1, &scaleBuffer);
		glCreateBuffers(1, &alphaBuffer);
		glCreateBuffers(1, &colorBuffer);
		glNamedBufferStorage(meanBuffer, num_gaussians * 3 * sizeof(float), mean_data, 0);
		glNamedBufferStorage(rotBuffer, num_gaussians * 4 * sizeof(float), rot_data, 0);
		glNamedBufferStorage(scaleBuffer, num_gaussians * 3 * sizeof(float), scale_data, 0);
		glNamedBufferStorage(alphaBuffer, num_gaussians * sizeof(float), alpha_data, 0);
		glNamedBufferStorage(colorBuffer, num_gaussians * sizeof(float) * 48, color_data, 0);
	}

	void GaussianData::render(int G) const
	{
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, meanBuffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, rotBuffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, scaleBuffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, alphaBuffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, colorBuffer);
		glDrawArraysInstanced(GL_TRIANGLES, 0, 36, G);
	}

	GaussianSurfaceRenderer::GaussianSurfaceRenderer( void )
	{
		_shader.init("GaussianSurface",
			sibr::loadFile(sibr::getShadersDirectory("gaussian") + "/gaussian_surface.vert"),
			sibr::loadFile(sibr::getShadersDirectory("gaussian") + "/gaussian_surface.frag"));

		_paramCamPos.init(_shader, "rayOrigin");
		_paramMVP.init(_shader,"MVP");
		_paramLimit.init(_shader, "alpha_limit");
		_paramStage.init(_shader, "stage");

		glCreateTextures(GL_TEXTURE_2D, 1, &idTexture);
		glTextureParameteri(idTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(idTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glCreateTextures(GL_TEXTURE_2D, 1, &colorTexture);
		glTextureParameteri(colorTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(colorTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glCreateFramebuffers(1, &fbo);
		glCreateRenderbuffers(1, &depthBuffer);

		makeFBO(800, 800);

		clearProg = glCreateProgram();
		const char* clearShaderSrc = R"(
			#version 430

			layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

			layout(std430, binding = 0) buffer IntArray {
				int arr[];
			};

			layout(location = 0) uniform int size;

			void main() {
				uint index = gl_GlobalInvocationID.x;
				if (index < size) {
					arr[index] = 0;
				}
			} 
			)";
		clearShader = glCreateShader(GL_COMPUTE_SHADER);
		glShaderSource(clearShader, 1, &clearShaderSrc, nullptr);
		glAttachShader(clearProg, clearShader);
		glLinkProgram(clearProg);
	}

	void GaussianSurfaceRenderer::makeFBO(int w, int h)
	{
		resX = w;
		resY = h;

		glBindTexture(GL_TEXTURE_2D, idTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, resX, resY, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, 0);

		glBindTexture(GL_TEXTURE_2D, colorTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, resX, resY, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		glBindTexture(GL_TEXTURE_2D, 0);

		glNamedRenderbufferStorage(depthBuffer, GL_DEPTH_COMPONENT, resX, resY);

		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, idTexture, 0);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);
	}

	int	GaussianSurfaceRenderer::process(int G, const GaussianData& mesh, const Camera& eye, IRenderTarget& target, float limit, sibr::Mesh::RenderMode mode, bool backFaceCulling)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);

		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		if (target.w() != resX || target.h() != resY)
		{
			makeFBO(target.w(), target.h());
		}

		// Solid pass
		GLuint drawBuffers[2];
		drawBuffers[0] = GL_COLOR_ATTACHMENT0;
		drawBuffers[1] = GL_COLOR_ATTACHMENT1;
		glDrawBuffers(2, drawBuffers);

		glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
		_shader.begin();
		_paramMVP.set(eye.viewproj());
		_paramCamPos.set(eye.position());
		_paramLimit.set(limit);
		_paramStage.set(0);
		mesh.render(G);

		// Simple additive blendnig (no order)
		glDrawBuffers(1, drawBuffers);
		glDepthMask(GL_FALSE);
		glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		_paramStage.set(1);
		mesh.render(G);

		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);

		_shader.end();

		glReadBuffer(GL_COLOR_ATTACHMENT0);
		glBlitNamedFramebuffer(
			fbo, target.fbo(),
			0, 0, resX, resY,
			0, 0, resX, resY,
			GL_COLOR_BUFFER_BIT, GL_NEAREST);

		return 0;
	}

} /*namespace sibr*/ 
