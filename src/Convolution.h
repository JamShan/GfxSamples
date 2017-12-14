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

		Type_Count
	};
	int   m_type;
	int   m_size;
	float m_gaussianSigma;
	bool  m_showKernel;
	
	float* m_weights;
	float* m_offsets;

	void initKernel();
	void shutdownKernel();

	enum Mode
	{
		Mode_2d,
		Mode_Separable,
		Mode_SeparableBilinear,

		Mode_Count
	};
	int m_mode;
};


#endif // Convolution_h
