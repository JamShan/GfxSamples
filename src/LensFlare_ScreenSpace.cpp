#include "LensFlare_ScreenSpace.h"

#include <frm/def.h>
#include <frm/gl.h>
#include <frm/Framebuffer.h>
#include <frm/GlContext.h>
#include <frm/Profiler.h>
#include <frm/Property.h>
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
	PropertyGroup& propGroup = m_props.addGroup("Lens Flare");
	//                 name                     default        min     max      storage
	propGroup.addInt  ("Downsample",            1,             0,      8,       &m_downsample);
	propGroup.addInt  ("Ghost Count",           4,             0,      32,      &m_downsample);
	propGroup.addFloat("Ghost Spacing",         0.1f,          0.0f,   2.0f,    &m_ghostSpacing);

	m_colorCorrection.setProps(m_props);
}

LensFlare_ScreenSpace::~LensFlare_ScreenSpace()
{
}

bool LensFlare_ScreenSpace::init(const apt::ArgList& _args)
{
	if (!AppBase::init(_args)) {
		return false;
	}

	m_shEnvMap = Shader::CreateVsFs("shaders/Envmap_vs.glsl", "shaders/Envmap_fs.glsl", "ENVMAP_SPHERE\0");
	m_shDownsample = Shader::CreateCs("shaders/Downsample_cs.glsl", 8, 8);
	m_shFeatures = Shader::CreateVsFs("shaders/Basic_vs.glsl", "shaders/Features_fs.glsl");

	bool ret = m_shEnvMap && m_shFeatures;

	ret &= m_colorCorrection.init();
	ret &= initScene();
	ret &= initLensFlare();	

	return ret;
}

void LensFlare_ScreenSpace::shutdown()
{
	shutdownScene();
	shutdownLensFlare();
	m_colorCorrection.shutdown();

	AppBase::shutdown();
}

bool LensFlare_ScreenSpace::update()
{
	if (!AppBase::update()) {
		return false;
	}

	bool reinit = false;
	ImGui::Begin("Lens Flare");
		reinit |= ImGui::SliderInt("Downsample", &m_downsample, 0, m_txSceneColor->getMipCount() - 1);

		ImGui::Spacing();
		ImGui::SliderInt("Ghost Count", &m_ghostCount, 0, 32);
		ImGui::SliderFloat("Ghost Spacing", &m_ghostSpacing, 0.0f, 2.0f);
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
	}
	 
 // downsample
	{	AUTO_MARKER("Downsample");
			

	#if 0
		m_txSceneColor->generateMipmap();

	#else		
		const int localX = m_shDownsample->getLocalSize().x;
		const int localY = m_shDownsample->getLocalSize().y;
		int w = m_txSceneColor->getWidth() >> 1;
		int h = m_txSceneColor->getHeight() >> 1;
		int lvl = 0;
		m_txSceneColor->setMinFilter(GL_LINEAR_MIPMAP_NEAREST); // no filtering between mips
		while (w >= 1 && h >= 1) {
			ctx->setShader  (m_shDownsample); // force reset bindings
			ctx->setUniform ("uSrcLevel", lvl);
			ctx->bindTexture("txSrc", m_txSceneColor);
			ctx->bindImage  ("txDst", m_txSceneColor, GL_WRITE_ONLY, ++lvl);
			ctx->dispatch(
				max((w + localX - 1) / localX, 1),
				max((h + localY - 1) / localY, 1)
				);
			glAssert(glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT));
			w = w >> 1;
			h = h >> 1;
		}
		m_txSceneColor->setMinFilter(GL_LINEAR_MIPMAP_LINEAR);
	#endif
	}

 // lens flare
	{	AUTO_MARKER("Lens Flare");
		{	AUTO_MARKER("Features");
			ctx->setFramebufferAndViewport(m_fbFeatures);
			ctx->setShader(m_shFeatures);
			ctx->bindTexture(m_txSceneColor);
			ctx->setUniform("uDownsample",   (float)m_downsample);
			ctx->setUniform("uGhostCount",   m_ghostCount);
			ctx->setUniform("uGhostSpacing", m_ghostSpacing);
			ctx->drawNdcQuad();
		}
	}
	
	//ctx->blitFramebuffer(m_fbFeatures, nullptr);
	m_colorCorrection.draw(ctx, m_txSceneColor, nullptr);
	
	AppBase::draw();
}

bool LensFlare_ScreenSpace::initScene()
{
	shutdownScene();

	int mipCount = Texture::GetMaxMipCount(m_resolution.x, m_resolution.y);
	m_txSceneColor = Texture::Create2d(m_resolution.x, m_resolution.y, GL_R11F_G11F_B10F, mipCount);
	m_txSceneColor->setName("txSceneColor");
	m_txSceneColor->setWrap(GL_CLAMP_TO_EDGE);
	m_txSceneColor->setMinFilter(GL_LINEAR_MIPMAP_NEAREST);
	m_txSceneDepth = Texture::Create2d(m_resolution.x, m_resolution.y, GL_DEPTH32F_STENCIL8);
	m_txSceneDepth->setName("txSceneDepth");
	m_txSceneDepth->setWrap(GL_CLAMP_TO_EDGE);
	m_fbScene = Framebuffer::Create(2, m_txSceneColor, m_txSceneDepth);
	m_txEnvmap = Texture::Create("textures/factory.env.hdr");
	
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
	sz.x = max(sz.x >> m_downsample, 1);
	sz.y = max(sz.y >> m_downsample, 1);
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
