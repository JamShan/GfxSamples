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

static App_skeleton3d s_inst;

bool App_skeleton3d::init(const apt::ArgList& _args)
{
	if (!AppBase::init(_args)) {
		return false;
	}

	// sample code here

	return true;
}

void App_skeleton3d::shutdown()
{
	// sample code here

	AppBase::shutdown();
}

bool App_skeleton3d::update()
{
	if (!AppBase::update()) {
		return false;
	}

	// sample code here

	return true;
}

void App_skeleton3d::draw()
{
	// sample code here

	AppBase::draw();
}
