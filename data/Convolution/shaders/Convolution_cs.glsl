#include "shaders/def.glsl"

uniform sampler2D txSrc;
uniform image2D writeonly txDst;

#define Type_Box                0
#define Type_Gaussian           1

#define Mode_2d                 0
#define Mode_Separable          1
#define Mode_SeparableBilinear  2

#ifndef TYPE
	#error TYPE not defined
#endif
#ifndef MODE
	#error MODE not defined
#endif
#ifndef KERNEL_SIZE
	#error KERNEL_SIZE not defined
#endif


#if (MODE == Mode_2d)
	#define OffsetType vec2
	#define GetOffset(iuv, i) (iuv + uOffsets[i])

#else
	#define OffsetType float
	uniform vec2 uDirection;
	#define GetOffset(iuv, i) (iuv + vec2(uOffsets[i]) * uDirection)
#endif
#define GetWeight(_i) (uWeights[_i])

uniform float      uWeights[KERNEL_SIZE];
uniform OffsetType uOffsets[KERNEL_SIZE];

void main()
{
	ivec2 txSize = ivec2(imageSize(txDst).xy);
	if (any(greaterThanEqual(gl_GlobalInvocationID.xy, txSize))) {
		return;
	}
	vec2 iuv = vec2(gl_GlobalInvocationID.xy) + vec2(0.5);
	vec2 texelSize = 1.0 / vec2(txSize);

	vec4 ret = vec4(0.0);
	for (int i = 0; i < KERNEL_SIZE; ++i) {
		ret += textureLod(txSrc, GetOffset(iuv, i) * texelSize, 0.0) * GetWeight(i);
	}

	imageStore(txDst, ivec2(gl_GlobalInvocationID.xy), ret);
}
