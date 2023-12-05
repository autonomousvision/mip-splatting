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


#version 430

uniform mat4 MVP;
uniform float alpha_limit;
uniform int stage;

layout (std430, binding = 0) buffer BoxCenters {
    float centers[];
};
layout (std430, binding = 1) buffer Rotations {
    vec4 rots[];
};
layout (std430, binding = 2) buffer Scales {
    float scales[];
};
layout (std430, binding = 3) buffer Alphas {
    float alphas[];
};
layout (std430, binding = 4) buffer Colors {
    float colors[];
};

mat3 quatToMat3(vec4 q) {
  float qx = q.y;
  float qy = q.z;
  float qz = q.w;
  float qw = q.x;

  float qxx = qx * qx;
  float qyy = qy * qy;
  float qzz = qz * qz;
  float qxz = qx * qz;
  float qxy = qx * qy;
  float qyw = qy * qw;
  float qzw = qz * qw;
  float qyz = qy * qz;
  float qxw = qx * qw;

  return mat3(
    vec3(1.0 - 2.0 * (qyy + qzz), 2.0 * (qxy - qzw), 2.0 * (qxz + qyw)),
    vec3(2.0 * (qxy + qzw), 1.0 - 2.0 * (qxx + qzz), 2.0 * (qyz - qxw)),
    vec3(2.0 * (qxz - qyw), 2.0 * (qyz + qxw), 1.0 - 2.0 * (qxx + qyy))
  );
}

const vec3 boxVertices[8] = vec3[8](
    vec3(-1, -1, -1),
    vec3(-1, -1,  1),
    vec3(-1,  1, -1),
    vec3(-1,  1,  1),
    vec3( 1, -1, -1),
    vec3( 1, -1,  1),
    vec3( 1,  1, -1),
    vec3( 1,  1,  1)
);

const int boxIndices[36] = int[36](
    0, 1, 2, 1, 3, 2,
    4, 6, 5, 5, 6, 7,
    0, 2, 4, 4, 2, 6,
    1, 5, 3, 5, 7, 3,
    0, 4, 1, 4, 5, 1,
    2, 3, 6, 3, 7, 6
);

out vec3 worldPos;
out vec3 ellipsoidCenter;
out vec3 ellipsoidScale;
out mat3 ellipsoidRotation;
out vec3 colorVert;
out float alphaVert;
out flat int boxID;

void main() {
	boxID = gl_InstanceID;
    ellipsoidCenter = vec3(centers[3 * boxID + 0], centers[3 * boxID + 1], centers[3 * boxID + 2]);
    float a = alphas[boxID];
	alphaVert = a;
	ellipsoidScale = vec3(scales[3 * boxID + 0], scales[3 * boxID + 1], scales[3 * boxID + 2]);
	ellipsoidScale = 2 * ellipsoidScale;

	vec4 q = rots[boxID];
	ellipsoidRotation = transpose(quatToMat3(q));

    int vertexIndex = boxIndices[gl_VertexID];
    worldPos = ellipsoidRotation * (ellipsoidScale * boxVertices[vertexIndex]);
    worldPos += ellipsoidCenter;

	float r = colors[boxID * 48 + 0] * 0.2 + 0.5;
	float g = colors[boxID * 48 + 1] * 0.2 + 0.5;
	float b = colors[boxID * 48 + 2] * 0.2 + 0.5;

	colorVert = vec3(r, g, b);
	
	if((stage == 0 && a < alpha_limit) || (stage == 1 && a >= alpha_limit))
	 	gl_Position = vec4(0,0,0,0);
	else
    	gl_Position = MVP * vec4(worldPos, 1);
}