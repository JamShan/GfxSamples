#include "shaders/def.glsl"

noperspective in vec2 vUv;

uniform sampler2D txFeatures;
uniform sampler2D txLensDirt;
uniform sampler2D txStarburst;

uniform float uGlobalBrightness;
uniform float uStarburstOffset;

layout(location=0) out vec3 fResult;

void main() 
{
 // starburst
	vec2 centerVec = vUv - vec2(0.5);
	float d = length(centerVec);
	float radial = acos(centerVec.x / d);
	float mask = 
		  texture(txStarburst, vec2(radial + uStarburstOffset * 1.0, 0.0)).r
		* texture(txStarburst, vec2(radial - uStarburstOffset * 0.5, 0.0)).r
		;
	mask = saturate(mask + (1.0 - smoothstep(0.0, 0.3, d)));
	
 // lens dirt
	mask *= textureLod(txLensDirt, vUv, 0.0).r;
	
	fResult = textureLod(txFeatures, vUv, 0.0).rgb * mask * uGlobalBrightness;
}
