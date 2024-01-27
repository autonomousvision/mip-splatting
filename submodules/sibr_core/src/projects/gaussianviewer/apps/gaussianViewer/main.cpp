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
#include "projects/gaussianviewer/renderer/GaussianView.hpp" 

#include <core/renderer/DepthRenderer.hpp>
#include <core/raycaster/Raycaster.hpp>
#include <core/view/SceneDebugView.hpp>
#include <algorithm>
#include <boost/filesystem.hpp>
#include <regex>
#include <imgui/imgui_internal.h>

namespace fs = boost::filesystem;

std::string findLargestNumberedSubdirectory(const std::string& directoryPath) {
	fs::path dirPath(directoryPath);
	if (!fs::exists(dirPath) || !fs::is_directory(dirPath)) {
		std::cerr << "Invalid directory: " << directoryPath << std::endl;
		return "";
	}

	std::regex regexPattern(R"_(iteration_(\d+))_");
	std::string largestSubdirectory;
	int largestNumber = -1;

	for (const auto& entry : fs::directory_iterator(dirPath)) {
		if (fs::is_directory(entry)) {
			std::string subdirectory = entry.path().filename().string();
			std::smatch match;

			if (std::regex_match(subdirectory, match, regexPattern)) {
				int number = std::stoi(match[1]);

				if (number > largestNumber) {
					largestNumber = number;
					largestSubdirectory = subdirectory;
				}
			}
		}
	}

	return largestSubdirectory;
}


#define PROGRAM_NAME "sibr_3Dgaussian"
using namespace sibr;

std::pair<int, int> findArg(const std::string& line, const std::string& name)
{
	int start = line.find(name, 0);
	start = line.find("=", start);
	start += 1;
	int end = line.find_first_of(",)", start);
	return std::make_pair(start, end);
}

static void* User_ReadOpen(ImGuiContext*, ImGuiSettingsHandler*, const char* name)
{
	return (void*)0x1;
}

static void User_ReadLine(ImGuiContext*, ImGuiSettingsHandler* handler, void*, const char* line)
{
	int i;
	if (sscanf(line, "DontShow=%d", &i) == 1)
		if (i)
		{
			*((bool*)handler->UserData) = true;
			return;
		}
	*((bool*)handler->UserData) = false;
}

static void User_WriteAll(ImGuiContext* imgui_ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf)
{
	// Write a buffer
	// If a window wasn't opened in this session we preserve its settings
	buf->reserve(buf->size() + 96); // ballpark reserve
	buf->appendf("[UserData][UserData]\nDontShow=%d\n", *((bool*)handler->UserData) ? 1 : 0);
	buf->appendf("\n");
}

int main(int ac, char** av) 
{
	// Parse Command-line Args
	CommandLineArgs::parseMainArgs(ac, av);
	GaussianAppArgs myArgs;
	myArgs.displayHelpIfRequired();
	
	if(!myArgs.modelPath.isInit() && myArgs.modelPathShort.isInit())
		myArgs.modelPath = myArgs.modelPathShort.get();
	if(!myArgs.dataset_path.isInit() && myArgs.pathShort.isInit())
		myArgs.dataset_path = myArgs.pathShort.get();

	int device = myArgs.device;

	// rendering size
	uint rendering_width = myArgs.rendering_size.get()[0];
	uint rendering_height = myArgs.rendering_size.get()[1];
	
	// window size
	uint win_width = rendering_width; // myArgs.win_width;
	uint win_height = rendering_height; // myArgs.win_height;

	const char* toload = myArgs.modelPath.get().c_str();

	// Window setup
	sibr::Window		window(PROGRAM_NAME, sibr::Vector2i(50, 50), myArgs, getResourcesDirectory() + "/gaussians/" + PROGRAM_NAME + ".ini");

	bool messageRead = false;
	ImGuiSettingsHandler ini_handler;
	ini_handler.TypeName = "UserData";
	ini_handler.UserData = &messageRead;
	ini_handler.TypeHash = ImHash("UserData", 0, 0);
	ini_handler.ReadOpenFn = User_ReadOpen;
	ini_handler.ReadLineFn = User_ReadLine;
	ini_handler.WriteAllFn = User_WriteAll;
	ImGui::GetCurrentContext()->SettingsHandlers.push_back(ini_handler);
	window.loadSettings();

	std::string cfgLine;
	std::ifstream cfgFile(myArgs.modelPath.get() + "/cfg_args");
	if (!cfgFile.good())
	{
		SIBR_ERR << "Could not find config file 'cfg_args' at " << myArgs.modelPath.get();
	}
	std::getline(cfgFile, cfgLine);

	if (!myArgs.dataset_path.isInit())
	{
		auto rng = findArg(cfgLine, "source_path");
		myArgs.dataset_path = cfgLine.substr(rng.first + 1, rng.second - rng.first - 2);
	}

	auto rng = findArg(cfgLine, "sh_degree");
	int sh_degree = std::stoi(cfgLine.substr(rng.first, rng.second - rng.first));

	rng = findArg(cfgLine, "white_background");
	bool white_background = cfgLine.substr(rng.first, rng.second - rng.first).find("True") != -1;

	BasicIBRScene::SceneOptions myOpts;
	myOpts.renderTargets = myArgs.loadImages;
	myOpts.mesh = true;
	myOpts.images = myArgs.loadImages;
	myOpts.cameras = true;
	myOpts.texture = false;

	BasicIBRScene::Ptr scene;
	try
	{
		scene.reset(new BasicIBRScene(myArgs, myOpts));
	}
	catch (...)
	{
		SIBR_LOG << "Did not find specified input folder, loading from model path" << std::endl;
		myArgs.dataset_path = myArgs.modelPath.get();
		scene.reset(new BasicIBRScene(myArgs, myOpts));
	}

	std::string plyfile = myArgs.modelPath.get();
	if (plyfile.back() != '/')
		plyfile += "/";
	plyfile += "point_cloud";
	if (!myArgs.iteration.isInit())
	{
		plyfile += "/" + findLargestNumberedSubdirectory(plyfile) + "/point_cloud_sibr.ply";
	}
	else
	{
		plyfile += "/iteration_" + myArgs.iteration.get() + "/point_cloud_sibr.ply";
	}

	// Setup the scene: load the proxy, create the texture arrays.
	const uint flags = SIBR_GPU_LINEAR_SAMPLING | SIBR_FLIP_TEXTURE;

	// Fix rendering aspect ratio if user provided rendering size
	uint scene_width = scene->cameras()->inputCameras()[0]->w();
	uint scene_height = scene->cameras()->inputCameras()[0]->h();
	float scene_aspect_ratio = scene_width * 1.0f / scene_height;
	float rendering_aspect_ratio = rendering_width * 1.0f / rendering_height;

	rendering_width = (rendering_width <= 0) ? std::min(1200U, scene_width) : rendering_width;
	rendering_height = (rendering_height <= 0) ? std::min(1200U, scene_width) / scene_aspect_ratio : rendering_height;
	if ((rendering_width > 0) && !myArgs.force_aspect_ratio ) {
		if (abs(scene_aspect_ratio - rendering_aspect_ratio) > 0.001f) {
			if (scene_width > scene_height) {
				rendering_height = rendering_width / scene_aspect_ratio;
			} 
			else {
				rendering_width = rendering_height * scene_aspect_ratio;
			}
		}
	}
	Vector2u usedResolution(rendering_width, rendering_height);

	const unsigned int sceneResWidth = usedResolution.x();
	const unsigned int sceneResHeight = usedResolution.y();

	// Create the ULR view.
	GaussianView::Ptr	gaussianView(new GaussianView(scene, sceneResWidth, sceneResHeight, plyfile.c_str(), &messageRead, sh_degree, white_background, !myArgs.noInterop, device));

	// Raycaster.
	std::shared_ptr<sibr::Raycaster> raycaster = std::make_shared<sibr::Raycaster>();
	raycaster->init();
	raycaster->addMesh(scene->proxies()->proxy());

	// Camera handler for main view.
	sibr::InteractiveCameraHandler::Ptr generalCamera(new InteractiveCameraHandler());
	generalCamera->setup(scene->cameras()->inputCameras(), Viewport(0, 0, (float)usedResolution.x(), (float)usedResolution.y()), nullptr);

	// Add views to mvm.
	MultiViewManager        multiViewManager(window, false);

	if (myArgs.rendering_mode == 1) 
		multiViewManager.renderingMode(IRenderingMode::Ptr(new StereoAnaglyphRdrMode()));
	
	multiViewManager.addIBRSubView("Point view", gaussianView, usedResolution, ImGuiWindowFlags_ResizeFromAnySide | ImGuiWindowFlags_NoBringToFrontOnFocus);
	multiViewManager.addCameraForView("Point view", generalCamera);

	// Top view
	const std::shared_ptr<sibr::SceneDebugView> topView(new sibr::SceneDebugView(scene, generalCamera, myArgs));
	multiViewManager.addSubView("Top view", topView, usedResolution);
	CHECK_GL_ERROR;
	topView->active(false);

	// save images
	generalCamera->getCameraRecorder().setViewPath(gaussianView, myArgs.dataset_path.get());
	if (myArgs.pathFile.get() !=  "" ) 
	{
		generalCamera->getCameraRecorder().loadPath(myArgs.pathFile.get(), usedResolution.x(), usedResolution.y());
		generalCamera->getCameraRecorder().recordOfflinePath(myArgs.outPath, multiViewManager.getIBRSubView("Point view"), "");
		if( !myArgs.noExit )
			exit(0);
	}

	// Main looooooop.
	while (window.isOpened()) 
	{
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
