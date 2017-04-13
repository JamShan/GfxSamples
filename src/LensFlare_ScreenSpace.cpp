#include "LensFlare_ScreenSpace.h"

#include <frm/def.h>
#include <frm/gl.h>
#include <frm/Framebuffer.h>
#include <frm/GlContext.h>
#include <frm/Profiler.h>
#include <frm/Shader.h>
#include <frm/Texture.h>

#include <apt/ArgList.h>
#include <apt/Image.h>

using namespace frm;
using namespace apt;

static LensFlare_ScreenSpace s_inst;

LensFlare_ScreenSpace::LensFlare_ScreenSpace()
	: AppBase("LensFlare_ScreenSpace")
{
	AppPropertyGroup& props = m_properties.addGroup("LensFlare_ScreenSpace");
	//                              name                  display name                   default              min    max    hidden
	m_downsample   = props.addInt  ("Downsample",         "Downsample",                  0,                   0,     32,    false);
	m_ghostCount   = props.addInt  ("GhostCount",         "Ghost Count",                 4,                   0,     32,    false);
	m_ghostSpacing = props.addFloat("GhostSpacing",       "Ghost Spacing",               0.1f,                0.0f,  2.0f,  false);
}

LensFlare_ScreenSpace::~LensFlare_ScreenSpace()
{
}

bool LensFlare_ScreenSpace::init(const apt::ArgList& _args)
{
	if (!AppBase::init(_args)) {
		return false;
	}

	m_shEnvMap = Shader::CreateVsFs("shaders/Envmap_vs.glsl", "shaders/Envmap_fs.glsl", "ENVMAP_CUBE\0");
	m_shColorCorrection = Shader::CreateVsFs("shaders/Basic_vs.glsl", "shaders/ColorCorrection_fs.glsl");

	m_shFeatures = Shader::CreateVsFs("shaders/Basic_vs.glsl", "shaders/Features_fs.glsl");

	bool ret = m_shEnvMap && m_shFeatures;

	ret &= initScene();
	ret &= initLensFlare();	

	return ret;
}

void LensFlare_ScreenSpace::shutdown()
{
	shutdownScene();
	shutdownLensFlare();

	AppBase::shutdown();
}

bool LensFlare_ScreenSpace::update()
{
	if (!AppBase::update()) {
		return false;
	}

	bool reinit = false;
	ImGui::Begin("Lens Flare");
		reinit |= ImGui::SliderInt("Downsample", m_downsample, 0, m_txSceneColor->getMipCount() - 1);

		ImGui::Spacing();
		ImGui::SliderInt("Ghost Count", m_ghostCount, 0, 32);
		ImGui::SliderFloat("Ghost Spacing", m_ghostSpacing, 0.0f, 2.0f);
	ImGui::End();
	if (reinit) {
		initLensFlare();
	}

	return true;
}

void LensFlare_ScreenSpace::draw()
{
	GlContext* ctx = GlContext::GetCurrent();
	Camera* cam = Scene::GetDrawCamera();

 // scene
	{	AUTO_MARKER("Scene");
		ctx->setFramebufferAndViewport(m_fbScene);
		ctx->setShader(m_shEnvMap);
		ctx->bindTexture("txEnvmap", m_txEnvmap);
		ctx->drawNdcQuad(cam);
	
	// \todo perform this manually (compute shader?) - experiment with average vs. max luminance
		AUTO_MARKER("Downsample");
		m_txSceneColor->generateMipmap();
	}

 // lens flare
	{	AUTO_MARKER("Lens Flare");
		{	AUTO_MARKER("Features");
			ctx->setFramebufferAndViewport(m_fbFeatures);
			ctx->setShader(m_shFeatures);
			ctx->bindTexture(m_txSceneColor);
			ctx->setUniform("uDownsample",   *m_downsample);
			ctx->setUniform("uGhostCount",   *m_ghostCount);
			ctx->setUniform("uGhostSpacing", *m_ghostSpacing);
			ctx->drawNdcQuad();
		}
	}
	
 // color correction
	{	AUTO_MARKER("Color Correction");

		static float m_exposure   = 0.0f; // f-stops, do exp2(exposure) before sending to shader
		static vec3  m_tint       = vec3(0.85f, 1.0f, 0.85f);
		static float m_saturation = 1.02f;
		static float m_contrast   = 1.2f;
		ImGui::SliderFloat("Exposure", &m_exposure, -12.0f, 12.0f);
		ImGui::ColorEdit3("Tint", &m_tint.x);
		ImGui::SliderFloat("Saturation", &m_saturation, 0.0f, 4.0f);
		ImGui::SliderFloat("Contrast", &m_contrast, 0.0f, 2.0f);
		ctx->setFramebufferAndViewport(0);
		ctx->setShader(m_shColorCorrection);
		ctx->setUniform("uExposure", exp2(m_exposure));
		ctx->setUniform("uTint", m_tint);
		ctx->setUniform("uSaturation", m_saturation);
		ctx->setUniform("uContrast", m_contrast);
		ctx->bindTexture("txInput", m_txSceneColor);
		ctx->drawNdcQuad();
	}
	
	AppBase::draw();
}

bool LensFlare_ScreenSpace::initScene()
{
	shutdownScene();

	int mipCount = Texture::GetMaxMipCount(m_resolution.x, m_resolution.y);
	m_txSceneColor = Texture::Create2d(m_resolution.x, m_resolution.y, GL_R11F_G11F_B10F, mipCount);
	m_txSceneColor->setName("txSceneColor");
	m_txSceneColor->setWrap(GL_CLAMP_TO_EDGE);
	m_txSceneColor->setMagFilter(GL_LINEAR_MIPMAP_NEAREST);
	m_txSceneDepth = Texture::Create2d(m_resolution.x, m_resolution.y, GL_DEPTH32F_STENCIL8);
	m_txSceneDepth->setName("txSceneDepth");
	m_txSceneDepth->setWrap(GL_CLAMP_TO_EDGE);
	m_fbScene = Framebuffer::Create(2, m_txSceneColor, m_txSceneDepth);
	//m_txEnvmap = Texture::CreateCubemap2x3("textures/diacourt_cube2x3.hdr");
	m_txEnvmap = Texture::Create("textures/WoodenDoor_ref.hdr");
	Texture::ConvertSphereToCube(*m_txEnvmap, 1024);

	bool ret = m_txSceneColor && m_txSceneDepth && m_txEnvmap;

	return ret;
}

void LensFlare_ScreenSpace::shutdownScene()
{
	Texture::Release(m_txSceneColor);
	Texture::Release(m_txSceneDepth);
	Framebuffer::Destroy(m_fbScene);
	Texture::Release(m_txEnvmap);
}


bool LensFlare_ScreenSpace::initLensFlare()
{
	shutdownLensFlare();

	ivec2 sz = ivec2(m_txSceneColor->getWidth(), m_txSceneColor->getHeight());
	sz.x = max(sz.x >> *m_downsample, 1);
	sz.y = max(sz.y >> *m_downsample, 1);
	m_txFeatures = Texture::Create2d(sz.x, sz.y, m_txSceneColor->getFormat());
	m_txFeatures->setName("txFeatures");
	m_txFeatures->setWrap(GL_CLAMP_TO_EDGE);
	m_fbFeatures = Framebuffer::Create(1, m_txFeatures);

	bool ret = m_txFeatures != nullptr;

	return ret;
}

void LensFlare_ScreenSpace::shutdownLensFlare()
{
	Texture::Release(m_txFeatures);
	Framebuffer::Destroy(m_fbFeatures);
}
