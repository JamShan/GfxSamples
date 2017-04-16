#include "shaders/def.glsl"

noperspective in vec2 vUv;

uniform sampler2D txSceneColor;

uniform float uDownsample; // lod index
uniform int   uGhostCount;
uniform float uGhostSpacing;

layout(location=0) out vec3 fResult;

void main() 
{
	vec2 uv = -vUv + 1.0;
	vec2 ghostVec = (vec2(0.5) - uv) * uGhostSpacing;
	
	vec3 ret = vec3(0.0);
	for (int i = 0; i < uGhostCount; ++i) {
		vec2 offset = fract(uv + ghostVec * vec2(i));
		
		vec3 s = textureLod(txSceneColor, offset, uDownsample).rgb;
		s = (s - 0.5) * 0.1;
		
		float weight = length(vec2(0.5) - offset) / length(vec2(0.5));
		weight = Remap(0.1, 0.5, weight);
	
		ret += s * weight;
	}
	
	fResult = ret;
}
