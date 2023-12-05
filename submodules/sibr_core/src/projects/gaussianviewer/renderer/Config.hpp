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


#pragma once

# include <core/system/Config.hpp>
# include <core/system/CommandLineArgs.hpp>

# ifdef SIBR_OS_WINDOWS
#  ifdef SIBR_STATIC_DEFINE
#    define SIBR_EXPORT
#    define SIBR_NO_EXPORT
#  else
#    ifndef SIBR_EXP_ULR_EXPORT
#      ifdef SIBR_EXP_ULR_EXPORTS
/* We are building this library */
#        define SIBR_EXP_ULR_EXPORT __declspec(dllexport)
#      else
/* We are using this library */
#        define SIBR_EXP_ULR_EXPORT __declspec(dllimport)
#      endif
#    endif
#    ifndef SIBR_NO_EXPORT
#      define SIBR_NO_EXPORT
#    endif
#  endif
# else
#  define SIBR_EXP_ULR_EXPORT
# endif

namespace sibr {

	/// Arguments for all ULR applications.
	struct GaussianAppArgs :
		virtual BasicIBRAppArgs {
		RequiredArg<std::string> modelPath = { "model-path", "Model directory" };
		RequiredArg<std::string> modelPathShort = { "m", "Model directory" };
		RequiredArg<std::string> iteration = { "iteration", "Iteration to load from model" };
		RequiredArg<std::string> pathShort = {"s", "path to the dataset root"};
		Arg<int> device = {"device", 0, "CUDA device index"};
		Arg<bool> loadImages = { "load_images", "Whether or not to load images for scene overview."};
		Arg<bool> noInterop = { "no_interop", "Don't try to use interop (may be required for unconventional OpenGL setups, like WSL)" };
	};

}

