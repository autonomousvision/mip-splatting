# Copyright (C) 2023, Inria
# GRAPHDECO research group, https://team.inria.fr/graphdeco
# All rights reserved.
# 
# This software is free for non-commercial, research and evaluation use 
# under the terms of the LICENSE.md file.
# 
# For inquiries contact sibr@inria.fr and/or George.Drettakis@inria.fr



project(SIBR_remoteGaussian_app)

file(GLOB SOURCES "*.cpp" "*.h" "*.hpp")
source_group("Source Files" FILES ${SOURCES})

file(GLOB RESOURCES "resources/*.ini")
source_group("Resources Files" FILES ${RESOURCES})

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME}

	${Boost_LIBRARIES}
	${ASSIMP_LIBRARIES}
	${GLEW_LIBRARIES}
	${OPENGL_LIBRARIES}
	${OpenCV_LIBRARIES}
	OpenMP::OpenMP_CXX
	sibr_remote
	sibr_view
	sibr_assets
	sibr_renderer
	sibr_basic
)
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "projects/remote/apps")

## High level macro to install in an homogen way all our ibr targets
include(install_runtime)
ibr_install_target(${PROJECT_NAME}
    INSTALL_PDB                         ## mean install also MSVC IDE *.pdb file (DEST according to target type)
	RESOURCES  	${RESOURCES}
	RSC_FOLDER 	"remote"
    STANDALONE  ${INSTALL_STANDALONE}   ## mean call install_runtime with bundle dependencies resolution
    COMPONENT   ${PROJECT_NAME}_install ## will create custom target to install only this project
)
