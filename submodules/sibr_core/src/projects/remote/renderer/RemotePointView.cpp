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

#include <projects/remote/renderer/RemotePointView.hpp>
#include <core/graphics/GUI.hpp>
#include <thread>
#include <boost/asio.hpp>

constexpr char* jResX = "resolution_x";
constexpr char* jResY = "resolution_y";
constexpr char* jFovY = "fov_y";
constexpr char* jFovX = "fov_x";
constexpr char* jZFar = "z_far";
constexpr char* jZNear = "z_near";
constexpr char* jTrain = "train";
constexpr char* jViewMat = "view_matrix";
constexpr char* jViewProjMat = "view_projection_matrix";
constexpr char* jScalingModifier = "scaling_modifier";
constexpr char* jSHsPython = "shs_python";
constexpr char* jRotScalePython = "rot_scale_python";
constexpr char* jKeepAlive = "keep_alive";

void sibr::RemotePointView::send_receive()
{
	while (keep_running)
	{
		SIBR_LOG << "Trying to connect..." << std::endl;
		try
		{
			boost::asio::io_service ioservice;
			boost::asio::ip::tcp::socket sock(ioservice);
			boost::asio::ip::address addr = boost::asio::ip::address::from_string(_ip);
			boost::asio::ip::tcp::endpoint contact(addr, _port);

			boost::system::error_code ec;
			do
			{
				sock.connect(contact, ec);
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			} while (keep_running && ec.failed());

			SIBR_LOG << "Connected!" << std::endl;
			while (keep_running)
			{
				{
					std::lock_guard<std::mutex> lg(_renderDataMutex);

					// Serialize our arbitrary data to something simple, yet convenient for both sides
					json sendData;
					sendData[jTrain] = _doTrainingBool ? 1 : 0;
					sendData[jSHsPython] = _doSHsPython ? 1 : 0;
					sendData[jRotScalePython] = _doRotScalePython ? 1 : 0;
					sendData[jScalingModifier] = _scalingModifier;
					sendData[jResX] = _remoteInfo.imgResolution.x();
					sendData[jResY] = _remoteInfo.imgResolution.y();
					sendData[jFovY] = _remoteInfo.fovy;
					sendData[jFovX] = _remoteInfo.fovx;
					sendData[jZFar] = _remoteInfo.zfar;
					sendData[jZNear] = _remoteInfo.znear;
					sendData[jKeepAlive] = _keepAlive ? 1 : 0;
					sendData[jViewMat] = std::vector<float>((float*)&_remoteInfo.view, ((float*)&_remoteInfo.view) + 16);
					sendData[jViewProjMat] = std::vector<float>((float*)&_remoteInfo.viewProj, ((float*)&_remoteInfo.viewProj) + 16);

					std::string message = sendData.dump();
					uint32_t messageLength = message.size();
					boost::asio::write(sock, boost::asio::buffer(&messageLength, sizeof(uint32_t)));
					boost::asio::write(sock, boost::asio::buffer(message.c_str(), messageLength));
				}

				uint32_t bytes_to_receive = _remoteInfo.imgResolution.x() * _remoteInfo.imgResolution.y() * 3;
				if (bytes_to_receive > 0)
				{
					std::lock_guard<std::mutex> ilg(_imageDataMutex);
					_imageData.resize(bytes_to_receive);
					boost::asio::read(sock, boost::asio::buffer(_imageData.data(), _imageData.size()));
					{
						std::lock_guard<std::mutex> lg(_renderDataMutex);
						_timestampReceived = _timestampRequested;
					}
					_imageDirty = true;
				}
				uint32_t sceneLength;
				boost::asio::read(sock, boost::asio::buffer(&sceneLength, sizeof(uint32_t)));
				std::vector<char> sceneName(sceneLength);
				boost::asio::read(sock, boost::asio::buffer(sceneName.data(), sceneLength));
				sceneName.push_back(0);
				current_scene = std::string(sceneName.data());
			}
		}
		catch (...)
		{
			SIBR_LOG << "Connection dropped" << std::endl;
		}
	}
}

sibr::RemotePointView::RemotePointView(std::string ip, uint port) : sibr::ViewBase(0, 0),
_ip(ip), _port(port)
{
	_pointbasedrenderer.reset(new PointBasedRenderer());
	_copyRenderer.reset(new CopyRenderer());
	_copyRenderer->flip() = true;

	glCreateTextures(GL_TEXTURE_2D, 1, &_imageTexture);
	glTextureParameteri(_imageTexture, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTextureParameteri(_imageTexture, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	glTextureParameteri(_imageTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(_imageTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	_networkThread = std::make_unique<std::thread>(&RemotePointView::send_receive, this);
}

void sibr::RemotePointView::setScene(const sibr::BasicIBRScene::Ptr & newScene) {
	_scene = newScene;

	// Tell the scene we are a priori using all active cameras.
	std::vector<uint> imgs_ulr;
	const auto & cams = newScene->cameras()->inputCameras();
	for (size_t cid = 0; cid < cams.size(); ++cid) {
		if (cams[cid]->isActive()) {
			imgs_ulr.push_back(uint(cid));
		}
	}
	_scene->cameras()->debugFlagCameraAsUsed(imgs_ulr);
}

void sibr::RemotePointView::onRenderIBR(sibr::IRenderTarget & dst, const sibr::Camera & eye)
{
	if (!_scene)
		return;

	bool preview = false;
	{
		std::lock_guard<std::mutex> lg(_renderDataMutex);
		if (eye.view() != _remoteInfo.view || eye.viewproj() != _remoteInfo.viewProj)
		{
			_remoteInfo.view = eye.view();
			_remoteInfo.viewProj = eye.viewproj();
			_remoteInfo.fovy = eye.fovy();
			_remoteInfo.fovx = 2 * atan(tan(eye.fovy() * 0.5) * eye.aspect());
			_remoteInfo.znear = eye.znear();
			_remoteInfo.zfar = eye.zfar();
			_timestampRequested++;
		}
		if (_resolution != _remoteInfo.imgResolution)
		{
			_remoteInfo.imgResolution = _resolution;
			_imageResize = true;
			_timestampRequested++;
		}
		preview = _timestampReceived != _timestampRequested;
	}

	if (_showSfM || _timestampReceived == 0 || (preview && _renderSfMInMotion))
	{
		_pointbasedrenderer->process(_scene->proxies()->proxy(), eye, dst);
	}
	else
	{
		{
			std::lock_guard<std::mutex> ilg(_imageDataMutex);
			if (_imageResize)
			{
				glBindTexture(GL_TEXTURE_2D, _imageTexture);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, _resolution.x(), _resolution.y(), 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
				glBindTexture(GL_TEXTURE_2D, 0);
				_imageResize = false;
			}
			if (_imageDirty && _imageData.size() == 3 * _resolution.x() * _resolution.y())
			{
				glTextureSubImage2D(_imageTexture, 0, 0, 0, _resolution.x(), _resolution.y(), GL_RGB, GL_UNSIGNED_BYTE, _imageData.data());
				_imageDirty = false;
			}
		}
		_copyRenderer->process(_imageTexture, dst);
	}
}

void sibr::RemotePointView::onGUI()
{
	const std::string guiName = "Remote Viewer Settings (" + name() + ")";
	if (ImGui::Begin(guiName.c_str())) 
	{
		ImGui::Checkbox("Show Input Points", &_showSfM);
		ImGui::Checkbox("Show Input Points during Motion", &_renderSfMInMotion);
		ImGui::Checkbox("Train", &_doTrainingBool);
		ImGui::Checkbox("SHs Python", &_doSHsPython);
		ImGui::Checkbox("Rot-Scale Python", &_doRotScalePython);
		ImGui::Checkbox("Keep model alive (after training)", &_keepAlive);
		ImGui::SliderFloat("Scaling Modifier", &_scalingModifier, 0.001f, 1.0f);
	}
	ImGui::End();
}

sibr::RemotePointView::~RemotePointView()
{
	keep_running = false;
	_networkThread->join();
}
