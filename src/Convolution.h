#pragma once
#ifndef Convolution_h
#define Convolution_h

#include <frm/AppSample.h>

class Convolution: public frm::AppSample
{
	typedef AppSample AppBase;
public:
	Convolution();
	virtual ~Convolution();

	virtual bool init(const apt::ArgList& _args) override;
	virtual void shutdown() override;
	virtual bool update() override;
	virtual void draw() override;

protected:
	enum Type
	{
		Type_Box,
		Type_Gaussian,
		Type_Binomial,

		Type_Count
	};
	int           m_type;
	int           m_size;
	float         m_gaussianSigma;
	float         m_gaussianSigmaOptimal;
	bool          m_showKernel;
	bool          m_showKernelGraph;

	int           m_kernelSize;
	float         m_kernelSum;
	float*        m_weights;
	float*        m_offsets;

	void initKernel();
	void shutdownKernel();

	enum Mode
	{
		Mode_2d,
		Mode_2dBilinear,
		Mode_Separable,
		Mode_SeparableBilinear,

		Mode_Count
	};
	int m_mode;

	frm::Texture* m_txSrc;
	frm::Texture* m_txDst[2];
	frm::Shader*  m_shConvolution;
};


#endif // Convolution_h
