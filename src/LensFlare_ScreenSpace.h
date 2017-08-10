/*	Screen space 'pseudo' lens flare.

	(1) Downsample

	(2) Feature generation

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
	int                 m_downsample;
	int                 m_ghostCount;
	float               m_ghostSpacing;
	frm::Shader*        m_shDownsample;
	frm::Shader*        m_shFeatures;
	frm::Texture*       m_txFeatures;
	frm::Framebuffer*   m_fbFeatures;

	bool initLensFlare();
	void shutdownLensFlare();
};


#endif // AppSkeleton_h
