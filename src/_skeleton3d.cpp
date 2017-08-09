#include "_skeleton3d.h"

#include <frm/def.h>
#include <frm/gl.h>
#include <frm/Buffer.h>
#include <frm/Camera.h>
#include <frm/Framebuffer.h>
#include <frm/GlContext.h>
#include <frm/Mesh.h>
#include <frm/Scene.h>
#include <frm/Shader.h>
#include <frm/Texture.h>
#include <frm/Window.h>
#include <frm/XForm.h>

#include <apt/ArgList.h>

using namespace frm;
using namespace apt;

static _skeleton3d s_inst;

_skeleton3d::_skeleton3d()
	: AppBase("_skeleton3d") 
{
	PropertyGroup& propGroup = m_props.addGroup("_skeleton3d");
	//                  name             default            min     max    storage
	//propGroup.addFloat  ("Float",        0.0f,              0.0f,   1.0f,  &foo);
}

_skeleton3d::~_skeleton3d()
{
}

bool _skeleton3d::init(const apt::ArgList& _args)
{
	if (!AppBase::init(_args)) {
		return false;
	}

	// sample code here

	return true;
}

void _skeleton3d::shutdown()
{
	// sample code here

	AppBase::shutdown();
}

bool _skeleton3d::update()
{
	if (!AppBase::update()) {
		return false;
	}

	// sample code here

	return true;
}

void _skeleton3d::draw()
{
	// sample code here

	AppBase::draw();
}
