/*
 * Copyright (C) 2020, Inria
 * GRAPHDECO research group, https://team.inria.fr/graphdeco
 * All rights reserved.
 *
 * This software is free for non-commercial, research and evaluation use 
 * under the terms of the LICENSE.md file.
 *
 * For inquiries contact sibr@inria.fr and/or George.Drettakis@inria.fr
 */


#include <projects/basic/renderer/TexturedMeshView.hpp>
#include <core/graphics/GUI.hpp>

sibr::TexturedMeshView::TexturedMeshView(const sibr::BasicIBRScene::Ptr & ibrScene, uint render_w, uint render_h) :
	_scene(ibrScene),
	sibr::ViewBase(render_w, render_h)
{
	const uint w = render_w;
	const uint h = render_h;

	//  Renderers.
	_textureRenderer.reset(new TexturedMeshRenderer());
	_poissonRenderer.reset(new PoissonRenderer(w, h));
	_poissonRenderer->enableFix() = true;

	// Rendertargets.
	_poissonRT.reset(new RenderTargetRGBA(w, h, SIBR_CLAMP_UVS));
	_blendRT.reset(new RenderTargetRGBA(w, h, SIBR_CLAMP_UVS));
}

void sibr::TexturedMeshView::setScene(const sibr::BasicIBRScene::Ptr & newScene) {
	_scene = newScene;
	const uint w = getResolution().x();
	const uint h = getResolution().y();

	_textureRenderer.reset(new TexturedMeshRenderer());
}

void sibr::TexturedMeshView::onRenderIBR(sibr::IRenderTarget & dst, const sibr::Camera & eye)
{
	// Perform ULR rendering, either directly to the destination RT, or to the intermediate RT when poisson blending is enabled.
	glViewport(0, 0, dst.w(), dst.h());
	dst.clear();
	_textureRenderer->process(
			_scene->proxies()->proxy(),
			eye, _scene->inputMeshTextures()->handle(), 
		_poissonBlend ? *_blendRT : dst, false);

	// Perform Poisson blending if enabled and copy to the destination RT.
	if (_poissonBlend) {
		_poissonRenderer->process(_blendRT, _poissonRT);
		blit(*_poissonRT, dst);
	}

}

void sibr::TexturedMeshView::onUpdate(Input & input)
{
}

void sibr::TexturedMeshView::onGUI()
{
	if (ImGui::Begin("Textured Mesh Renderer Settings")) {

		// Poisson settings.
		ImGui::Checkbox("Poisson ", &_poissonBlend); ImGui::SameLine();
		ImGui::Checkbox("Poisson fix", &_poissonRenderer->enableFix());

	}
	ImGui::End();
}

