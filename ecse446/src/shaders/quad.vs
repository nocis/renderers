/*
    This file is part of TinyRender, an educative PBR system.
    Designed for ECSE 446/546 Realistic Image Synthesis, McGill University.

    Copyright (c) 2018 by Derek Nowrouzezahrai and others.
*/
#version 330 core

layout(location = 1) in vec2 uv;
layout(location = 0) in vec3 position;

out vec2 texCoords;

void main() {
    // pass the uv
	gl_Position = vec4(position, 1.0);
	texCoords = uv;
}