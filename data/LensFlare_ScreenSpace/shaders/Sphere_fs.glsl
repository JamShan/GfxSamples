#include "shaders/def.glsl"

uniform vec3 uColor;

layout(location=0) out vec3 fResult;

void main() 
{
	fResult = uColor;
}
