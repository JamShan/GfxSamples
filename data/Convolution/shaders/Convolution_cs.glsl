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
	layout(std140) uniform _bfKernel
	{
		float uWeights[KERNEL_SIZE * KERNEL_SIZE];
		vec2  uOffsets[KERNEL_SIZE * KERNEL_SIZE];
	};
#else
	layout(std140) uniform _bfKernel
	{
		float uWeights[KERNEL_SIZE];
		float uOffsets[KERNEL_SIZE];
	};
	uniform ivec2 uDirection;
#endif

void main()
{
	ivec2 txSize = ivec2(imageSize(txDst).xy);
	if (any(greaterThanEqual(gl_GlobalInvocationID.xy, txSize))) {
		return;
	}
	ivec2 iuv = ivec2(gl_GlobalInvocationID.xy);
	
	vec4 ret = vec4(0.0);
	
	#if (MODE == Mode_2d)
	#elif (MODE == Mode_Separable)
	#elif (MODE == Mode_SeparableBilinear)
	
	#endif
	
	imageStore(txDst, ivec2(gl_GlobalInvocationID.xy), ret);
}