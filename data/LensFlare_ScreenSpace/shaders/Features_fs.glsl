#include "shaders/def.glsl"

#define GHOST_COLOR_PER_SAMPLE         1  // Apply txGhostGradientColor inside the sample loop instead of at the end.
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
uniform float     uHaloThreshold;
uniform float     uHaloAspectRatio;

uniform float     uChromaticAberration;

layout(location=0) out vec3 fResult;

// http://www.iquilezles.org/www/articles/functions/functions.htm
float cubicPulse( float x, float c, float w )
{
    x = abs(x - c);
    if( x>w ) return 0.0;
    x /= w;
    return 1.0 - x*x*(3.0-2.0*x);
}

vec3 ApplyThreshold(in vec3 _rgb, in float _threshold)
{
	return _rgb * smoothstep(vec3(_threshold), vec3(_threshold + 4.0), _rgb);
}

float GetUvDistanceToCenter(in vec2 _uv)
{
#error TODO Is the divide necessary?
	return length(vec2(0.5) - _uv) / length(vec2(0.5));
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

vec3 GenerateGhosts(in vec2 _uv, in float _threshold)
{
	vec3 ret = vec3(0.0);
	vec2 ghostVec = (vec2(0.5) - _uv) * uGhostSpacing;
	for (int i = 0; i < uGhostCount; ++i) {
		vec2 offset = fract(_uv + ghostVec * vec2(i));

		vec3 s = SampleSceneColor(offset);
		s = ApplyThreshold(s, _threshold);

		float distanceToCenter = GetUvDistanceToCenter(offset);

	 // reduce contributions from samples at the screen edge
	 // \todo power function is nicer here, need a cheap fit
		float weight = 1.0 - smoothstep(0.0, 0.75, distanceToCenter);

		#if GHOST_COLOR_PER_SAMPLE
			s *= textureLod(txGhostColorGradient, vec2(distanceToCenter, 0.5), 0.0).rgb;
		#endif

		ret += s * weight;
	}
	#if !GHOST_COLOR_PER_SAMPLE
		ret *= textureLod(txGhostColorGradient, vec2(GetUvDistanceToCenter(_uv), 0.5), 0.0).rgb;
	#endif

	return ret;
}

vec3 GenerateHalo(in vec2 _uv, in float _radius, in float _aspectRatio, in float _threshold)
{
	vec2 haloVec = vec2(0.5) - _uv;
	#if DISABLE_HALO_ASPECT_RATIO
		haloVec = normalize(haloVec);
		float haloWeight = GetUvDistanceToCenter(_uv);
	#else
		haloVec.x /= _aspectRatio;
		haloVec = normalize(haloVec);
		haloVec.x *= _aspectRatio;
		float haloWeight = GetUvDistanceToCenter((_uv - vec2(0.5, 0.0)) / vec2(_aspectRatio, 1.0) + vec2(0.5, 0.0));
	#endif
	haloVec *= _radius;
	haloWeight = cubicPulse(haloWeight, _radius, 0.3); // \todo parameterize thickness

	return ApplyThreshold(SampleSceneColor(_uv + haloVec), _threshold) * haloWeight;
}

void main()
{
	vec2 uv = vec2(1.0) - vUv;
	vec3 ret = vec3(0.0);

	ret += GenerateGhosts(uv, uGhostThreshold);
	ret += GenerateHalo(uv, uHaloRadius, uHaloAspectRatio, uHaloThreshold);

	fResult = ret;
}
