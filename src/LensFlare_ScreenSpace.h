/*	Screen space 'pseudo' lens flare.

	(1) Downsample
		- Usually already exists as an input for other post processing effects; downsample into the mip chain of the lighting buffer.
		- Can use previous frame's result and therefore move steps 2 and 3 to async compute.

	(2) Feature generation
		- Ghost gradient color works better if you sample inside the sample loop.

	(3) Blur

	(4) Upscale/composite

	References:
	(1) http://ivizlab.sfu.ca/papers/cgf2012.pdf (separable shaped blur)
	(2) http://sebastien.hillaire.free.fr/index.php?option=com_content&view=article&id=71&Itemid=105 (Hexagonal Blur, Hilaire)
*/

#pragma once
#ifndef LensFlare_ScreenSpace_h
#define LensFlare_ScreenSpace_h

#include <frm/AppSample3d.h>
#include <frm/RenderNodes.h>

class LensFlare_ScreenSpace: public frm::AppSample3d
{
	typedef AppSample3d AppBase;
public:
	LensFlare_ScreenSpace();
	virtual ~LensFlare_ScreenSpace();

	virtual bool init(const apt::ArgList& _args) override;
	virtual void shutdown() override;
	virtual bool update() override;
	virtual void draw() override;

 // render nodes
	frm::ColorCorrection m_colorCorrection;

 // scene
	frm::Texture*     m_txSceneColor;
	frm::Texture*     m_txSceneDepth;
	frm::Framebuffer* m_fbScene;
	frm::Shader*      m_shEnvMap;
	frm::Texture*     m_txEnvmap;
	frm::Shader*      m_shColorCorrection;
	
	bool initScene();
	void shutdownScene();


 // lens flare
	bool                m_showLensFlareOnly;
	bool                m_showFeaturesOnly;
	int                 m_downsample;
	int                 m_ghostCount;
	float               m_ghostSpacing;
	float               m_ghostThreshold;
	frm::Texture*       m_txGhostColorGradient;
	float               m_haloRadius;
	float               m_haloThreshold;
	float               m_haloAspectRatio;
	float               m_chromaticAberration;
	int                 m_blurSize;
	float               m_blurStep;
	float               m_globalBrightness;
	frm::Texture*       m_txLensDirt;
	frm::Texture*       m_txStarburst;
	frm::Shader*        m_shDownsample;
	frm::Shader*        m_shFeatures;
	frm::Shader*        m_shBlur;
	frm::Shader*        m_shComposite;
	frm::Texture*       m_txFeatures[2]; // 2 targets for separable blur
	frm::Framebuffer*   m_fbFeatures;

	bool initLensFlare();
	void shutdownLensFlare();
};


#endif // AppSkeleton_h
