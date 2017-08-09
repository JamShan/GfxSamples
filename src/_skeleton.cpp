#include "_skeleton.h"

#include <frm/def.h>
#include <frm/gl.h>
#include <frm/Buffer.h>
#include <frm/Framebuffer.h>
#include <frm/GlContext.h>
#include <frm/Mesh.h>
#include <frm/Shader.h>
#include <frm/Texture.h>
#include <frm/Window.h>

#include <apt/ArgList.h>

using namespace frm;
using namespace apt;

static _skeleton s_inst;

_skeleton::_skeleton()
	: AppBase("_skeleton") 
{
	PropertyGroup& propGroup = m_props.addGroup("_skeleton");
	//                  name             default            min     max    storage
	//propGroup.addFloat  ("Float",        0.0f,              0.0f,   1.0f,  &foo);
}

_skeleton::~_skeleton()
{
}

bool _skeleton::init(const apt::ArgList& _args)
{
	if (!AppBase::init(_args)) {
		return false;
	}

	// sample code here

	return true;
}

void _skeleton::shutdown()
{
	// sample code here

	AppBase::shutdown();
}

bool _skeleton::update()
{
	if (!AppBase::update()) {
		return false;
	}

	// sample code here

	return true;
}

void _skeleton::draw()
{
	// sample code here

	AppBase::draw();
}
