#include "LensFlare_ScreenSpace.h"

#include <frm/def.h>
#include <frm/gl.h>
#include <frm/Framebuffer.h>
#include <frm/GlContext.h>
#include <frm/Mesh.h>
#include <frm/MeshData.h>
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
	//                  name                     default        min     max      storage
	propGroup.addInt   ("Downsample",            1,             0,      8,       &m_downsample);
	propGroup.addInt   ("Ghost Count",           4,             0,      32,      &m_ghostCount);
	propGroup.addFloat ("Ghost Spacing",         0.1f,          0.0f,   2.0f,    &m_ghostSpacing);
	propGroup.addFloat ("Ghost Threshold",       2.0f,          0.0f,   20.0f,   &m_ghostThreshold);
	propGroup.addFloat ("Halo Radius",           0.6f,          0.0f,   2.0f,    &m_haloRadius);
	propGroup.addFloat ("Halo Threshold",        2.0f,          0.0f,   20.0f,   &m_haloThreshold);
	propGroup.addFloat ("Halo Aspect Ratio",     1.0f,          0.0f,   2.0f,    &m_haloAspectRatio);
	propGroup.addFloat ("Chromatic Aberration",  0.01f,         0.0f,   0.2f,    &m_chromaticAberration);
	propGroup.addInt   ("Blur Size",             16,            1,      64,      &m_blurSize);
	propGroup.addFloat ("Blur Step",             1.5f,          1.0f,   4.0f,    &m_blurStep);
	propGroup.addFloat ("Global Brightness",     0.01f,         0.0f,   1.0f,    &m_globalBrightness);
	propGroup.addBool  ("Lens Flare Only",       false,                          &m_showLensFlareOnly);
	propGroup.addFloat3("Sphere Color",          vec3(10.0f),   0.0f,   20.0f,   &m_sphereColor);

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

	m_shEnvMap = Shader::CreateVsFs("shaders/Envmap_vs.glsl", "shaders/Envmap_fs.glsl", "ENVMAP_CUBE\0");
	m_shDownsample = Shader::CreateCs("shaders/Downsample_cs.glsl", 8, 8);
	m_shFeatures = Shader::CreateVsFs("shaders/Basic_vs.glsl", "shaders/Features_fs.glsl");
	m_shBlur = Shader::CreateCs("shaders/GaussBlur_cs.glsl", 16, 16);	
	m_shComposite = Shader::CreateVsFs("shaders/Basic_vs.glsl", "shaders/Composite_fs.glsl");

	bool ret = m_shEnvMap && m_shFeatures;

	m_txGhostColorGradient = Texture::Create("textures/ghost_color_gradient.psd");
	m_txGhostColorGradient->setName("txGhostColorGradient");
	m_txGhostColorGradient->setWrap(GL_CLAMP_TO_EDGE);
	m_txLensDirt = Texture::Create("textures/lens_dirt.png");
	m_txLensDirt->setName("txLensDirt");
	m_txLensDirt->setWrap(GL_CLAMP_TO_EDGE);

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
		reinit |= ImGui::SliderInt("Downsample", &m_downsample, 0, APT_MAX(m_txSceneColor->getMipCount() - 1, 8));

		ImGui::Spacing();
		ImGui::Checkbox("Lens Flare Only", &m_showLensFlareOnly);
		ImGui::SliderInt("Ghost Count", &m_ghostCount, 0, 32);
		ImGui::SliderFloat("Ghost Spacing", &m_ghostSpacing, 0.0f, 2.0f);
		ImGui::SliderFloat("Ghost Threshold", &m_ghostThreshold, 0.0f, 20.0f);
		ImGui::SliderFloat("Halo Radius", &m_haloRadius, 0.0f, 2.0f);
		ImGui::SliderFloat("Halo Threshold", &m_haloThreshold, 0.0f, 20.0f);
		ImGui::SliderFloat("Halo Aspect Ratio", &m_haloAspectRatio, 0.0f, 2.0f);
		ImGui::SliderFloat("Chromatic Aberration", &m_chromaticAberration, 0.0f, 0.2f);		
		
		ImGui::Spacing();
		ImGui::SliderInt("Blur Size", &m_blurSize, 4, 32);
		ImGui::SliderFloat("Blur Step", &m_blurStep, 1.0f, 4.0f);

		ImGui::Spacing();
		ImGui::SliderFloat("Global Brightness", &m_globalBrightness, 0.0f, 1.0f);

		ImGui::Spacing();
		ImGui::ColorEdit3("Sphere Color", &m_sphereColor.x, ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);

		ImGui::Spacing();
		ImGui::Spacing();
		if (ImGui::TreeNode("Color Correction")) {
			m_colorCorrection.edit();
			ImGui::TreePop();
		}
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

		glAssert(glEnable(GL_DEPTH_TEST));
		glAssert(glClear(GL_DEPTH_BUFFER_BIT));
		ctx->setShader(m_shSphere);
		ctx->setUniform("uWorld", mat4(1.0f));
		ctx->setUniform("uColor", m_sphereColor);
		ctx->bindBuffer(cam->m_gpuBuffer);
		ctx->setMesh(m_msSphere);
		ctx->draw();
		glAssert(glDisable(GL_DEPTH_TEST));	
	}
	 
 // downsample
	{	AUTO_MARKER("Downsample");
	
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
	}

 // lens flare
	{	AUTO_MARKER("Lens Flare");
		{	AUTO_MARKER("Features");
			ctx->setFramebufferAndViewport(m_fbFeatures);
			ctx->setShader(m_shFeatures);
			ctx->bindTexture(m_txSceneColor);
			ctx->bindTexture(m_txGhostColorGradient);
			ctx->setUniform("uDownsample",          (float)m_downsample);
			ctx->setUniform("uGhostCount",          m_ghostCount);
			ctx->setUniform("uGhostSpacing",        m_ghostSpacing);
			ctx->setUniform("uGhostThreshold",      m_ghostThreshold);
			ctx->setUniform("uHaloRadius",          m_haloRadius);
			ctx->setUniform("uHaloThreshold",       m_haloThreshold);
			ctx->setUniform("uHaloAspectRatio",     m_haloAspectRatio);
			ctx->setUniform("uChromaticAberration", m_chromaticAberration);
			ctx->drawNdcQuad();
		}
		{	AUTO_MARKER("Blur");
			ctx->setShader(m_shBlur);
			ctx->setUniform("uRadiusPixels", m_blurSize);

		 // horizontal
			ctx->setUniform("uDirection", vec2(m_blurStep, 0.0f));
			ctx->bindTexture("txSrc", m_txFeatures[0]);
			ctx->bindImage("txDst", m_txFeatures[1], GL_WRITE_ONLY);
			ctx->dispatch(m_txFeatures[1]);
			glAssert(glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT));
			
			ctx->clearImageBindings();
			ctx->clearTextureBindings();
			
		 // vertical
			ctx->setUniform("uDirection", vec2(0.0f, m_blurStep));
			ctx->bindTexture("txSrc", m_txFeatures[1]);
			ctx->bindImage("txDst", m_txFeatures[0], GL_WRITE_ONLY);
			ctx->dispatch(m_txFeatures[0]);
			glAssert(glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT));
		}
		{	AUTO_MARKER("Composite");
			ctx->setFramebufferAndViewport(m_fbScene);
			ctx->setShader(m_shComposite);
			ctx->setUniform("uGlobalBrightness", m_globalBrightness);
			ctx->bindTexture(m_txFeatures[0]);
			ctx->bindTexture(m_txLensDirt);
			glAssert(glEnable(GL_BLEND));
			glAssert(glBlendFunc(GL_ONE, GL_ONE));
			ctx->drawNdcQuad();
			glAssert(glDisable(GL_BLEND));
		}
	}
	
	if (m_showLensFlareOnly) {
		ctx->blitFramebuffer(m_fbFeatures, nullptr);
	} else {
		m_colorCorrection.draw(ctx, m_txSceneColor, nullptr);
	}
	
	AppBase::draw();
}

bool LensFlare_ScreenSpace::initScene()
{
	shutdownScene();

	int mipCount = Texture::GetMaxMipCount(m_resolution.x, m_resolution.y);
	m_txSceneColor = Texture::Create2d(m_resolution.x, m_resolution.y, GL_R11F_G11F_B10F, mipCount);
	if (!m_txSceneColor) return false;
	m_txSceneColor->setName("txSceneColor");
	m_txSceneColor->setWrap(GL_CLAMP_TO_EDGE);
	m_txSceneColor->setMinFilter(GL_LINEAR_MIPMAP_NEAREST);

	m_txSceneDepth = Texture::Create2d(m_resolution.x, m_resolution.y, GL_DEPTH32F_STENCIL8);
	if (!m_txSceneDepth) return false;
	m_txSceneDepth->setName("txSceneDepth");
	m_txSceneDepth->setWrap(GL_CLAMP_TO_EDGE);
	
	m_fbScene = Framebuffer::Create(2, m_txSceneColor, m_txSceneDepth);
	if (!m_fbScene) return false;

	m_txEnvmap = Texture::Create("textures/env_factory.dds");
	if (!m_txEnvmap) return false;
	
	MeshDesc msDesc;
	msDesc.addVertexAttr(VertexAttr::Semantic_Positions, DataType_Float32, 3);
	MeshData* msData = MeshData::CreateSphere(msDesc, 1.0f, 24, 24);
	m_msSphere = Mesh::Create(*msData);
	MeshData::Destroy(msData);
	if (!m_msSphere) return false;

	m_shSphere = Shader::CreateVsFs("shaders/Sphere_vs.glsl", "shaders/Sphere_fs.glsl");
	if (!m_shSphere) return false;

	return true;
}

void LensFlare_ScreenSpace::shutdownScene()
{
	Texture::Release(m_txSceneColor);
	Texture::Release(m_txSceneDepth);
	Framebuffer::Destroy(m_fbScene);
	Texture::Release(m_txEnvmap);
	Mesh::Release(m_msSphere);
}


bool LensFlare_ScreenSpace::initLensFlare()
{
	shutdownLensFlare();

	ivec2 sz = ivec2(m_txSceneColor->getWidth(), m_txSceneColor->getHeight());
	sz.x = max(sz.x >> m_downsample, 1);
	sz.y = max(sz.y >> m_downsample, 1);
	m_txFeatures[0] = Texture::Create2d(sz.x, sz.y, m_txSceneColor->getFormat());
	if (!m_txFeatures[0]) return false;
	m_txFeatures[0]->setName("txFeatures");
	m_txFeatures[0]->setWrap(GL_CLAMP_TO_EDGE);

	m_txFeatures[1] = Texture::Create(m_txFeatures[0], false);
	m_txFeatures[1]->setWrap(GL_CLAMP_TO_EDGE);

	m_fbFeatures = Framebuffer::Create(1, m_txFeatures[0]);
	if (!m_fbFeatures) return false;

	return true;
}

void LensFlare_ScreenSpace::shutdownLensFlare()
{
	Texture::Release(m_txFeatures[0]);
	Texture::Release(m_txFeatures[1]);
	Framebuffer::Destroy(m_fbFeatures);
}
