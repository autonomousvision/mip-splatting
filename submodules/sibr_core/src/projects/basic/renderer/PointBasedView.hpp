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


#pragma once

# include "Config.hpp"
# include <core/system/Config.hpp>
# include <core/graphics/Mesh.hpp>
# include <core/view/ViewBase.hpp>
# include <core/renderer/CopyRenderer.hpp>
# include <core/renderer/PointBasedRenderer.hpp>
# include <core/scene/BasicIBRScene.hpp>
# include <core/renderer/PoissonRenderer.hpp>

namespace sibr { 

	/**
	 * \class PointBasedView
	 * \brief Wrap a Textured Mesh renderer with additional parameters and information.
	 */
	class SIBR_EXP_ULR_EXPORT PointBasedView : public sibr::ViewBase
	{
		SIBR_CLASS_PTR(PointBasedView);

	public:

		/**
		 * Constructor
		 * \param ibrScene The scene to use for rendering.
		 * \param render_w rendering width
		 * \param render_h rendering height
		 */
		PointBasedView(const sibr::BasicIBRScene::Ptr& ibrScene, uint render_w, uint render_h);

		/** Replace the current scene.
		 *\param newScene the new scene
		 **/
		void setScene(const sibr::BasicIBRScene::Ptr & newScene);

		/**
		 * Perform rendering. Called by the view manager or rendering mode.
		 * \param dst The destination rendertarget.
		 * \param eye The novel viewpoint.
		 */
		void onRenderIBR(sibr::IRenderTarget& dst, const sibr::Camera& eye) override;

		/**
		 * Update inputs (do nothing).
		 * \param input The input state.
		 */
		void onUpdate(Input& input) override;

		/**
		 * Update the GUI.
		 */
		void onGUI() override;

		/// \return a reference to the renderer.
		const PointBasedRenderer::Ptr & getPointBasedRenderer() const { return _pointBasedRenderer; }

		/// \return a reference to the scene
		const BasicIBRScene::Ptr & getScene() const { return _scene; }

	protected:

		
		std::shared_ptr<sibr::BasicIBRScene> _scene;
		PointBasedRenderer::Ptr			 _pointBasedRenderer;
											 
											 
	};

} /*namespace sibr*/ 
