#include "shaders/def.glsl"
#include "shaders/Camera.glsl"

layout(location=0) in vec3 aPosition;

uniform mat4 uWorld;

void main() 
{
	vec4 P = uWorld * vec4(aPosition, 1.0);
	gl_Position = bfCamera.m_viewProj * P;
}
