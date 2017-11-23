#include "shaders/def.glsl"
#include "shaders/WindowFunctions.glsl"

#define GHOST_TINT_PER_SAMPLE          1  // Apply txGhostGradientColor inside the sample loop instead of at the end.
#define DISABLE_HALO_ASPECT_RATIO      0  // Code is simpler/cheaper without this, but the halo shape is fixed.
#define DISABLE_CHROMATIC_ABERRATION   0  // Takes 3x fewer samples.

noperspective in vec2 vUv;

uniform sampler2D txSceneColor;

uniform float uDownsample; // lod index

uniform int       uGhostCount;
uniform float     uGhostSpacing;
uniform float     uGhostThreshold;
uniform sampler2D txGhostColorGradient;

uniform float     uHaloRadius;
uniform float     uHaloThickness;
uniform float     uHaloThreshold;
uniform float     uHaloAspectRatio;

uniform float     uChromaticAberration;

layout(location=0) out vec3 fResult;

vec3 ApplyThreshold(in vec3 _rgb, in float _threshold)
{
	return max(_rgb - vec3(_threshold), vec3(0.0));
}

vec3 SampleSceneColor(in vec2 _uv)
{
#if DISABLE_CHROMATIC_ABERRATION
	return textureLod(txSceneColor, _uv, uDownsample).rgb;
#else
	vec2 offset = normalize(vec2(0.5) - _uv) * uChromaticAberration;
	return vec3(
		textureLod(txSceneColor, _uv + offset, uDownsample).r,
		textureLod(txSceneColor, _uv, uDownsample).g,
		textureLod(txSceneColor, _uv - offset, uDownsample).b
		);
#endif
}

vec3 SampleGhosts(in vec2 _uv, in float _threshold)
{
	vec3 ret = vec3(0.0);
	vec2 ghostVec = (vec2(0.5) - _uv) * uGhostSpacing;
	for (int i = 0; i < uGhostCount; ++i) {
	 // sample scene color
		vec2 suv = fract(_uv + ghostVec * vec2(i));
		vec3 s = SampleSceneColor(suv);
		s = ApplyThreshold(s, _threshold);
		
	 // tint/weight
		float distanceToCenter = distance(suv, vec2(0.5));
		#if GHOST_TINT_PER_SAMPLE
			s *= textureLod(txGhostColorGradient, vec2(distanceToCenter, 0.5), 0.0).rgb; // incorporate weight into tint gradient
		#else
			float weight = 1.0 - smoothstep(0.0, 0.75, distanceToCenter); // analytical weight
			s *= weight;
		#endif

		ret += s;
	}
	#if !GHOST_TINT_PER_SAMPLE
		ret *= textureLod(txGhostColorGradient, vec2(distance(_uv, vec2(0.5)), 0.5), 0.0).rgb;
	#endif

	return ret;
}

vec3 SampleHalo(in vec2 _uv, in float _radius, in float _aspectRatio, in float _threshold)
{
	vec2 haloVec = vec2(0.5) - _uv;
	#if DISABLE_HALO_ASPECT_RATIO
		haloVec = normalize(haloVec);
		float haloWeight = distance(_uv, vec2(0.5));
	#else
		haloVec.x /= _aspectRatio;
		haloVec = normalize(haloVec);
		haloVec.x *= _aspectRatio;
		vec2 wuv = (_uv - vec2(0.5, 0.0)) / vec2(_aspectRatio, 1.0) + vec2(0.5, 0.0);
		float haloWeight = distance(wuv, vec2(0.5));
	#endif
	haloVec *= _radius;
	haloWeight = Window_Cubic(haloWeight, _radius, uHaloThickness);
	return ApplyThreshold(SampleSceneColor(_uv + haloVec), _threshold) * haloWeight;
}

void main()
{
	vec2 uv = vec2(1.0) - vUv; // flip the texture coordinates
	vec3 ret = vec3(0.0);

	ret += SampleGhosts(uv, uGhostThreshold);
	ret += SampleHalo(uv, uHaloRadius, uHaloAspectRatio, uHaloThreshold);

	
	fResult = ret;
}
