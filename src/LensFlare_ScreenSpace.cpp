#include "LensFlare_ScreenSpace.h"

#include <frm/def.h>
#include <frm/gl.h>
#include <frm/Framebuffer.h>
#include <frm/GlContext.h>
#include <frm/Shader.h>
#include <frm/Texture.h>

#include <apt/ArgList.h>

using namespace frm;
using namespace apt;

static LensFlare_ScreenSpace s_inst;

LensFlare_ScreenSpace::LensFlare_ScreenSpace()
	: AppBase("LensFlare_ScreenSpace")
{
}

LensFlare_ScreenSpace::~LensFlare_ScreenSpace()
{
}

bool LensFlare_ScreenSpace::init(const apt::ArgList& _args)
{
	if (!AppBase::init(_args)) {
		return false;
	}

 // scene

	int mipCount = Texture::GetMaxMipCount(m_resolution.x, m_resolution.y);
	m_txSceneColor = Texture::Create2d(m_resolution.x, m_resolution.y, GL_R11F_G11F_B10F, mipCount);
	m_txSceneColor->setName("txSceneColor");
	m_txSceneColor->setWrap(GL_CLAMP_TO_EDGE);
	m_txSceneDepth = Texture::Create2d(m_resolution.x, m_resolution.y, GL_DEPTH32F_STENCIL8);
	m_txSceneDepth->setName("txSceneDepth");
	m_txSceneDepth->setWrap(GL_CLAMP_TO_EDGE);
	m_fbScene = Framebuffer::Create(2, m_txSceneColor, m_txSceneDepth);
	m_txEnvmap = Texture::Create("textures/factory.hdr");

 // lens flare

	return true;
}

void LensFlare_ScreenSpace::shutdown()
{
 // scene
	Texture::Release(m_txSceneColor);
	Texture::Release(m_txSceneDepth);
	Framebuffer::Destroy(m_fbScene);
	Texture::Release(m_txEnvmap);
 
 // lens flare
	

	AppBase::shutdown();
}

bool LensFlare_ScreenSpace::update()
{
	if (!AppBase::update()) {
		return false;
	}

	// sample code here

	return true;
}

void LensFlare_ScreenSpace::draw()
{
	// sample code here

	AppBase::draw();
}
