#version 450

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragUv;
layout(location = 2) in vec3 fragPos;

layout (set = 0, binding = 1) uniform sampler2D normalsTexture;

vec3 getNormal()
{
	// Perturb normal, see http://www.thetenthplanet.de/archives/1180
	vec3 tangentNormal = texture(normalsTexture, fragUv).xyz * 2.0 - 1.0;

	vec3 q1 = dFdx(fragPos);
	vec3 q2 = dFdy(fragPos);
	vec2 st1 = dFdx(fragUv);
	vec2 st2 = dFdy(fragUv);

	vec3 N = normalize(fragNormal);
	vec3 T = normalize(q1 * st2.t - q2 * st1.t);
	vec3 B = -normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);

	return normalize(TBN * tangentNormal);
}

void main() {
    outColor = vec4(getNormal(), 1.0);
}