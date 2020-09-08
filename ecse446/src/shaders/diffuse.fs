#version 330 core

#define pi 3.14159265358979323846

uniform vec3 camPos;
uniform vec3 lightPos;
uniform vec3 lightIntensity;
uniform vec3 albedo;


in vec3 vNormal;
in vec3 vPos;
out vec3 color;

void main()
{
    vec3 lightDirection = (lightPos - vPos);
    float len = length(lightDirection);
    vec3 brdf = albedo / pi;
	vec3 lightShading = lightIntensity / pow(len,2);
    color = brdf * lightShading * dot(normalize(lightDirection), normalize(vNormal));

}