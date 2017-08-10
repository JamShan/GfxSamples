#include "shaders/def.glsl"

uniform sampler2D txSrc;
uniform image2D writeonly txDst;

uniform int uSrcLevel;

void main()
{
	ivec2 txSize = ivec2(imageSize(txDst).xy);
	if (any(greaterThanEqual(gl_GlobalInvocationID.xy, txSize))) {
		return;
	}
	vec2 uv = vec2(gl_GlobalInvocationID.xy) / vec2(txSize) + 0.5 / vec2(txSize);

	vec4 ret = textureLod(txSrc, uv, uSrcLevel);
	
	imageStore(txDst, ivec2(gl_GlobalInvocationID.xy), ret);
}
