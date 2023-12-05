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


#include <fstream>
#include <core/graphics/Window.hpp>
#include <core/view/MultiViewManager.hpp>
#include <core/system/String.hpp>
#include "projects/remote/renderer/RemotePointView.hpp" 
#include <core/renderer/DepthRenderer.hpp>
#include <core/raycaster/Raycaster.hpp>
#include <core/view/SceneDebugView.hpp>
#include <algorithm>

#define PROGRAM_NAME "SIBR Remote Gaussian Viewer"
using namespace sibr;

void resetScene(RemoteAppArgs myArgs,
	int rendering_width,
	int rendering_height,
	BasicIBRScene::Ptr& scene,
	RemotePointView::Ptr&	pointBasedView,
	std::shared_ptr<sibr::SceneDebugView>& topView,
	MultiViewManager& multiViewManager
	)
{
	if (multiViewManager.numSubViews() > 0)
	{
		multiViewManager.removeSubView("Point view");
		multiViewManager.removeSubView("Top view");
	}

	BasicIBRScene::SceneOptions myOpts;
	myOpts.renderTargets = myArgs.loadImages;
	myOpts.mesh = true;
	myOpts.images = myArgs.loadImages;
	myOpts.cameras = true;
	myOpts.texture = false;

	try
	{
		scene.reset(new BasicIBRScene(myArgs, myOpts));
	}
	catch (std::exception& ex)
	{
		SIBR_ERR << "Problem loading model info from input path " << myArgs.dataset_path
			<< ". Consider overriding path to model directory using --path.";
	}

	// Setup the scene: load the proxy, create the texture arrays.
	const uint flags = SIBR_GPU_LINEAR_SAMPLING | SIBR_FLIP_TEXTURE;

	// Fix rendering aspect ratio if user provided rendering size
	float divider = scene->cameras()->inputCameras()[0]->w() / std::min(1200.f, (float)scene->cameras()->inputCameras()[0]->w());
	uint scene_width = scene->cameras()->inputCameras()[0]->w();
	uint scene_height = scene->cameras()->inputCameras()[0]->h();
	float scene_aspect_ratio = scene_width * 1.0f / scene_height;
	float rendering_aspect_ratio = rendering_width * 1.0f / rendering_height;

	if ((rendering_width > 0) && !myArgs.force_aspect_ratio) {
		if (abs(scene_aspect_ratio - rendering_aspect_ratio) > 0.001f) {
			if (scene_width > scene_height) {
				rendering_height = rendering_width / scene_aspect_ratio;
			}
			else {
				rendering_width = rendering_height * scene_aspect_ratio;
			}
		}
	}

	// check rendering size
	rendering_width = (rendering_width <= 0) ? scene->cameras()->inputCameras()[0]->w() / divider : rendering_width;
	rendering_height = (rendering_height <= 0) ? scene->cameras()->inputCameras()[0]->h() / divider : rendering_height;
	Vector2u usedResolution(rendering_width, rendering_height);

	const unsigned int sceneResWidth = usedResolution.x();
	const unsigned int sceneResHeight = usedResolution.y();

	pointBasedView->setScene(scene);
	pointBasedView->setResolution({ sceneResWidth, sceneResHeight });

	// Raycaster.
	std::shared_ptr<sibr::Raycaster> raycaster = std::make_shared<sibr::Raycaster>();
	raycaster->init();
	raycaster->addMesh(scene->proxies()->proxy());

	// Camera handler for main view.
	sibr::InteractiveCameraHandler::Ptr generalCamera(new InteractiveCameraHandler());
	generalCamera->setup(scene->cameras()->inputCameras(), Viewport(0, 0, (float)usedResolution.x(), (float)usedResolution.y()), nullptr);

	// Top view
	topView.reset(new sibr::SceneDebugView(scene, generalCamera, myArgs));
	multiViewManager.addSubView("Top view", topView, usedResolution);
	topView->active(false);

	multiViewManager.addIBRSubView("Point view", pointBasedView, { sceneResWidth, sceneResHeight }, ImGuiWindowFlags_NoBringToFrontOnFocus);
	multiViewManager.addCameraForView("Point view", generalCamera);

	CHECK_GL_ERROR;

	// save images
	generalCamera->getCameraRecorder().setViewPath(pointBasedView, myArgs.dataset_path.get());
	if (myArgs.pathFile.get() != "") {
		generalCamera->getCameraRecorder().loadPath(myArgs.pathFile.get(), usedResolution.x(), usedResolution.y());
		generalCamera->getCameraRecorder().recordOfflinePath(myArgs.outPath, multiViewManager.getIBRSubView("Point view"), "");
		if (!myArgs.noExit)
			exit(0);
	}
}

int main(int ac, char** av) {

	// Parse Command-line Args
	CommandLineArgs::parseMainArgs(ac, av);
	RemoteAppArgs myArgs;
	myArgs.displayHelpIfRequired();

	if(!myArgs.dataset_path.isInit() && myArgs.pathShort.isInit())
		myArgs.dataset_path = myArgs.pathShort.get();

	const bool doVSync = !myArgs.vsync;
	// rendering size
	uint rendering_width = myArgs.rendering_size.get()[0];
	uint rendering_height = myArgs.rendering_size.get()[1];
	
	// window size
	uint win_width = rendering_width; // myArgs.win_width;
	uint win_height = rendering_height; // myArgs.win_height;

	// Window setup
	sibr::Window		window(PROGRAM_NAME, sibr::Vector2i(50, 50), myArgs, getResourcesDirectory() + "/remote/" + PROGRAM_NAME + ".ini");

	// Add views to mvm.
	MultiViewManager        multiViewManager(window, false);
	BasicIBRScene::Ptr		scene;
	RemotePointView::Ptr	remoteView(new RemotePointView(myArgs.ip, myArgs.port));
	std::shared_ptr<sibr::SceneDebugView> topView;
	
	std::string currentName;

	bool pathOverride = myArgs.dataset_path.isInit();
	if (pathOverride)
	{
		resetScene(myArgs, rendering_width, rendering_height, scene, remoteView, topView, multiViewManager);
	}

	// Main looooooop.
	while (window.isOpened()) 
	{
		if (!pathOverride && remoteView->sceneName() != "" && remoteView->sceneName() != currentName)
		{
			currentName = remoteView->sceneName();
			myArgs.dataset_path = currentName;
			resetScene(myArgs, rendering_width, rendering_height, scene, remoteView, topView, multiViewManager);
		}

		sibr::Input::poll();
		window.makeContextCurrent();
		if (sibr::Input::global().key().isPressed(sibr::Key::Escape)) {
			window.close();
		}

		multiViewManager.onUpdate(sibr::Input::global());
		multiViewManager.onRender(window);

		window.swapBuffer();
		CHECK_GL_ERROR;
	}

	return EXIT_SUCCESS;
}
