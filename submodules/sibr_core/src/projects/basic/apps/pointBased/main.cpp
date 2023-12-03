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


#include <fstream>
#include <core/graphics/Window.hpp>
#include <core/view/MultiViewManager.hpp>
#include <projects/basic/renderer/PointBasedView.hpp>
#include <core/scene/BasicIBRScene.hpp>
#include <core/raycaster/Raycaster.hpp>
#include <core/view/SceneDebugView.hpp>

#define PROGRAM_NAME "sibr_PointBased_app"
using namespace sibr;

const char* usage = ""
	"Usage: " PROGRAM_NAME " -path <dataset-path or mesh-path>"    	                                "\n"
	;


struct PointBasedAppArgs :
	virtual BasicIBRAppArgs {
	Arg<std::string> meshPath = { "mesh", "", "mesh path" };
};


int main( int ac, char** av )
{
	{
		// Parse Commad-line Args
		CommandLineArgs::parseMainArgs(ac, av);
		PointBasedAppArgs myArgs;

		const bool doVSync = !myArgs.vsync;
		// rendering size
		uint rendering_width = myArgs.rendering_size.get()[0];
		uint rendering_height = myArgs.rendering_size.get()[1];
		// window size
		uint win_width = myArgs.win_width;
		uint win_height = myArgs.win_height;

		// Window setup
		sibr::Window        window(PROGRAM_NAME, sibr::Vector2i(50, 50), myArgs, getResourcesDirectory() + "/ulr/" + PROGRAM_NAME + ".ini");

		// Setup IBR
		BasicIBRScene::Ptr		scene;
		std::string meshPath;

		// Specify scene initlaization options
		scene = BasicIBRScene::Ptr(new BasicIBRScene(myArgs));

		// Load the texture image and provide it to the scene
		sibr::ImageRGB inputTextureImg;

		// check rendering size; if no rendering-size specified, use 1080p
		rendering_width = (rendering_width <= 0) ? 1920 : rendering_width;
		rendering_height = (rendering_height <= 0) ? 1080 : rendering_height;
		Vector2u usedResolution(rendering_width, rendering_height);

		const unsigned int sceneResWidth = usedResolution.x();
		const unsigned int sceneResHeight = usedResolution.y();

		PointBasedView::Ptr	pointbasedView(new PointBasedView(scene, sceneResWidth, sceneResHeight));

		// Raycaster.
		std::shared_ptr<sibr::Raycaster> raycaster = std::make_shared<sibr::Raycaster>();
		raycaster->init();
		raycaster->addMesh(scene->proxies()->proxy());

		// Camera handler for main view.
		sibr::InteractiveCameraHandler::Ptr generalCamera(new InteractiveCameraHandler());
		if( scene->cameras()->inputCameras().size() == 0 ) 
			generalCamera->setup(scene->proxies()->proxyPtr(), Viewport(0, 0, (float)usedResolution.x(), (float)usedResolution.y()));
		else
			generalCamera->setup(scene->cameras()->inputCameras(), Viewport(0, 0, (float)usedResolution.x(), (float)usedResolution.y()), raycaster);

		// Add views to mvm.
		MultiViewManager        multiViewManager(window, false);
		multiViewManager.addIBRSubView("Point-Based View", pointbasedView, usedResolution, ImGuiWindowFlags_ResizeFromAnySide);
		multiViewManager.addCameraForView("Point-Based View", generalCamera);

		// Top view
		const std::shared_ptr<sibr::SceneDebugView>    topView(new sibr::SceneDebugView(scene, multiViewManager.getViewport(), generalCamera, myArgs));
		multiViewManager.addSubView("Top view", topView, usedResolution);

		if (myArgs.pathFile.get() !=  "" ) {
			generalCamera->getCameraRecorder().loadPath(myArgs.pathFile.get(), usedResolution.x(), usedResolution.y());
			generalCamera->getCameraRecorder().recordOfflinePath(myArgs.outPath, multiViewManager.getIBRSubView("TM view"), "pointbasedmesh");
			if( !myArgs.noExit )
				exit(0);
		}

		while (window.isOpened())
		{
			sibr::Input::poll();
			window.makeContextCurrent();
			if (sibr::Input::global().key().isPressed(sibr::Key::Escape))
				window.close();

			multiViewManager.onUpdate(sibr::Input::global());
			multiViewManager.onRender(window);
			window.swapBuffer();
			CHECK_GL_ERROR
		}

	}

	return EXIT_SUCCESS;
}
