#include "shaders/def.glsl"

noperspective in vec2 vUv;

uniform sampler2D txFeatures;
uniform sampler2D txLensDirt;

uniform float uGlobalBrightness;

layout(location=0) out vec3 fResult;

void main() 
{
	vec3 ret = textureLod(txFeatures, vUv, 0.0).rgb;
	ret *= textureLod(txLensDirt, vUv, 0.0).rrr;
	fResult = ret * uGlobalBrightness;
}
