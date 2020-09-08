/*
    This file is part of TinyRender, an educative PBR system.
    Designed for ECSE 446/546 Realistic Image Synthesis, McGill University.
    Copyright (c) 2018 by Derek Nowrouzezahrai and others.
*/
#version 330 core

// UNIFORMS
// ------------------------------------------------------------------
uniform sampler2D texturePosition;
uniform sampler2D textureNormal;
uniform mat4 projection;

// INPUT/OUTPUT
// ------------------------------------------------------------------
in vec2 texCoords;

out vec3 color;

// CONSTANTS
// ------------------------------------------------------------------
const int N_SAMPLES = 32;
float RADIUS = 0.14;
float BIAS = 0;

const float PI = 3.1415926535897932384626433832795;
const float INV_PI = 1.0 / PI;

// RANDOM NUMBER GENERATOR (RNG)
// ------------------------------------------------------------------
float seed;
float oldrand() { return fract( sin( seed++ ) * 43758.5453123 );}

vec4 seedvec;
float rand()
{
    //pseudo-random
    const vec4 q = vec4(1225.0,1585.0,2457.0,2098.0);
    const vec4 r = vec4(1112.0,367.0,92.0,265.0);
    const vec4 a = vec4(3423.0,2646.0,1707.0,1999.0);
    const vec4 m = vec4(4194287.0,4194277.0, 4194191.0, 4194167.0);

    vec4 beta = floor(seedvec / q);
    vec4 p = a * (seedvec- beta * q) - beta * r;
    beta = (sign(-p) + vec4(1.0)) * vec4(0.5) * m;
    seedvec = (p + beta);

    return fract(dot(seedvec / m,  vec4(1.0, -1.0, 1.0, -1.0)));
}

void setRNGSeed()
{
    float w = textureSize(texturePosition,0).x;
    float h = textureSize(texturePosition,0).y;
    seed = (h*gl_FragCoord.x / w + gl_FragCoord.y / h);
    seedvec = vec4(oldrand() * 4194304.0,
                   oldrand() * 4194304.0,
                   oldrand() * 4194304.0,
                   oldrand() * 4194304.0);
}

// UTILS
// ------------------------------------------------------------------

vec3 squareToUniformHemisphere()
{
    // generate a random direction in the hemisphere aligned to z+
    float z = rand();
    float r = sqrt(max(0.0, 1.0 - z*z));
    float phi = 2.0 * PI * rand();
    vec3 v = vec3(r * cos(phi), r * sin(phi), z);
    return v;
}

float squareToUniformHemispherePdf()
{
    return 1.0 / (2.0 * PI);
}

vec3 getTangent(vec3 normal)
{
    vec3 rvec = vec3(rand()*2.0 - 1.0, rand()*2.0 - 1.0, 0.0f); // rotate around z-axis (in tangent space)
    return normalize(rvec - normal * dot(rvec,normal)); // rotate
}

// MAIN
// ------------------------------------------------------------------
void main()
{
    setRNGSeed();

    /**
    * 1) Get the position and normal of the shading point (screen space) from the GBuffer.
    */

    // texture sample
	vec4 pos = texture(texturePosition,texCoords);
	vec4 normal = texture(textureNormal,texCoords);

    /**
    * 1) Build the shading normal's frame (TBN).
         ( use getTangent() )
    */

	vec3 tang = normalize(getTangent(normal.xyz));
	vec3 binormal = normalize(cross(normal.xyz,tang));

	mat3 TBN = mat3(tang,binormal,normal);

    /**
    For each sample:
    * 1) Get a sample direction (view space).
    * 2) Align the sample hemisphere to the shading normal.
    * 3) Place the sample at the shading point (use the RADIUS constant).
    * 4) Get the depth value at the sample's position using the depth buffer.
    *    - Project the sample to screen space ie. pixels (NDC).
    *    - Transform the sample in NDC coordinates [-1,1] to texture coordinates [0,1].
    * 5) Check for occlusion using the sample's depth value and the depth value at the sample's position.
         (use some epsilon via the BIAS constant)
    * 6) Adjust occlusion by checking the range between the depth value at the sample's position
         and the shading point's depth value ( use depthRange() ).
    */

		// The bonus has NOT been implemented

		bool bonus = false;

		color = vec3(0.0f);

		for (int i = 0; i<N_SAMPLES;i++) {

			//applying uniform hemisphere approach
			vec3 incident = squareToUniformHemisphere();
			float pdf = squareToUniformHemispherePdf();

			//calculating incident ray's depth
			float cosT = incident.z;
			vec3 localisation = TBN*incident;

			localisation = pos.xyz + localisation*RADIUS;

			//sample
			vec4 spl = vec4(localisation,1.0);

			spl = projection*spl;

			spl.xyz = spl.xyz / spl.w;

			//going from NDC space ([-1, 1]) to screen space ([0,1])
			spl.xyz = spl.xyz * 0.5;
			spl.xyz = spl.xyz + 0.5;

			float depth = texture(texturePosition,spl.xy).z;

			// why depth negative?
			// OGL uses (eye - center) as positive z direction!!!

			//applying color change
			if (depth<localisation.z+BIAS) {
				color = color + vec3(INV_PI/pdf*cosT);
			}
			else {
				color = color + vec3(0.0f);
			}
		}

		//correcting for the given number of samples
		color = color / N_SAMPLES;

}