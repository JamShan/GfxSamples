#include "Convolution.h"

#include <frm/def.h>
#include <frm/gl.h>
#include <frm/icon_fa.h>
#include <frm/Buffer.h>
#include <frm/Framebuffer.h>
#include <frm/GlContext.h>
#include <frm/Mesh.h>
#include <frm/Profiler.h>
#include <frm/Shader.h>
#include <frm/Texture.h>
#include <frm/Window.h>

#include <apt/ArgList.h>

using namespace frm;
using namespace apt;

namespace {

#define USE_INTEGRATION 1

inline float GaussianDistribution(float _d, float _sigma, float _sigma2)
{
	float d = (_d * _d) / (2.0f * _sigma2);
	return 1.0f / (kTwoPi * _sigma2) * exp(-d);
}

// Evaluate GaussianDistribution() over the interval [_mind,_maxd] via trapezoidal integration.
float GaussianIntegration(float _mind, float _maxd, float _sigma, float _sigma2, int _sampleCount = 64)
{
	const float w = 1.0f / (float)(_sampleCount - 1);
	const float stepd = (_maxd - _mind) * w;
	float ret = GaussianDistribution(_mind, _sigma, _sigma2);
	float d = _mind + stepd;
	for (int i = 1; i < _sampleCount - 1; ++i) {
		ret += GaussianDistribution(d, _sigma, _sigma2) * 2.0f;
		d += stepd;
	}
	ret += GaussianDistribution(d, _sigma, _sigma2);
	return ret * stepd / 2.0f;
}


float GaussianKernel1d(int _size, float _sigma, float* weights_, bool _normalize = true)
{
 // force _size to be odd
	if (_size % 2 == 0) {
		++_size;
	}
 // generate
	const float sigma2 = _sigma * _sigma;
	const int n = _size / 2;
	float sum = 0.0f;
	for (int i = 0; i < _size; ++i) {
		float d = (float)(i - n);
		#if USE_INTEGRATION
			weights_[i] = GaussianIntegration(d - 0.5f, d + 0.5f, _sigma, sigma2);
		#else
			weights_[i] = GaussianDistribution(d, _sigma, sigma2);
		#endif
		sum += weights_[i];
	}
 // normalize
	if (_normalize) {
		for (int i = 0; i < _size; ++i) {
			weights_[i] /= sum;
		}
	}
	return sum;
}

float GaussianKernel2d(int _size, float _sigma, float* weights_, bool _normalize = true)
{
 // force _size to be odd
	if (_size % 2 == 0) {
		++_size;
	}
 // generate first row
	float sum = GaussianKernel1d(_size, _sigma, weights_, false);
	sum *= sum;	
 // derive subsequent rows from the first
	for (int i = 1; i < _size; ++i) {
		for (int j = 0; j < _size; ++j) {
			int k = i * _size + j;
			weights_[k] = (weights_[i] * weights_[j]) / (_normalize ? sum : 1.0f);
		}
	}
 // copy the first row from the last
	for (int i = 0; i < _size; ++i) {
		weights_[i] = weights_[(_size - 1) * _size + i];
	}
	return sum;
}

// Find sigma such that no weights are < _epsilon. Epsilon should be the smallest representable for the precision of the signal to be convolved e.g. 1/255 for 8-bit.
float GaussianFindSigma(int _size, float _epsilon)
{
	float* tmp = new float[_size];
	const float d = (float)(-_size / 2);
	float sigma = 1.0f;
	float stp = 1.0f;
	while (stp > 0.01f) {
		GaussianKernel1d(_size, sigma, tmp);
		float w = tmp[0];
		if (w > _epsilon) {
			sigma -= stp;
			stp *= 0.5f;
		}
		sigma += stp;
	}
	delete[] tmp;
	return sigma;
}

float BinomialKernel1d(int _size, float* weights_, bool _normalize = true)
{
 // force _size to be odd
	if (_size % 2 == 0) {
		++_size;
	}
 // generate (Pascal's triangle)
	weights_[0] = 1.0f;
	float sum = 1.0f;
	for (int i = 1; i < _size; ++i) {
		weights_[i] = weights_[i - 1] * (float)(_size - i) / (float)i;
		sum += weights_[i];
	}
 // normalize
	if (_normalize) {
		for (int i = 0; i < _size; ++i) {
			weights_[i] /= sum;
		}
	}
	return sum;
}

float BinomialKernel2d(int _size, float* weights_, bool _normalize = true)
{
 // force _size to be odd
	if (_size % 2 == 0) {
		++_size;
	}
 // generate first row
	float sum = BinomialKernel1d(_size, weights_, false);
	sum *= sum;	
 // derive subsequent rows from the first
	for (int i = 1; i < _size; ++i) {
		for (int j = 0; j < _size; ++j) {
			int k = i * _size + j;
			weights_[k] = (weights_[i] * weights_[j]) / (_normalize ? sum : 1.0f);
		}
	}
 // copy the first row from the last
	for (int i = 0; i < _size; ++i) {
		weights_[i] = weights_[(_size - 1) * _size + i];
	}
	return sum;
}

// Outputs are arrays of _size / 2 + 1.
void KernelOptimizerBilinear1d(int _size, const float* _weightsIn, float* weightsOut_, float* offsetsOut_)
{
	const int n = _size / 2;
	int j = 0;
	for (int i = 0; i != _size - 1; i += 2) {
		float w1 = _weightsIn[i];
		float w2 = _weightsIn[i + 1];
		float w3 = w1 + w2;
		float o1 = (float)(i - n);
		float o2 = (float)(i - n + 1);
		float o3 = (o1 * w1 + o2 * w2) / w3;
		weightsOut_[j] = w3;
		offsetsOut_[j] = o3;
		++j;
	}
	weightsOut_[j] = _weightsIn[_size - 1];
	offsetsOut_[j] = (float)(_size - 1 - n);
}

} // namespace

static Convolution s_inst;

Convolution::Convolution()
	: AppBase("Convolution")
{
	PropertyGroup& propGroup = m_props.addGroup("Convolution");
	//                  name                default            min     max          storage
	propGroup.addInt   ("Type",             Type_Gaussian,     0,      Type_Count,  &m_type);
	propGroup.addInt   ("Size",             5,                 1,      21,          &m_size);
	propGroup.addFloat ("Gaussian Sigma",   1.0f,              0.0f,   4.0f,        &m_gaussianSigma);
	propGroup.addBool  ("Show Kernel",      false,                                  &m_showKernel);
	propGroup.addBool  ("Show Graph",       false,                                  &m_showKernelGraph);
	propGroup.addInt   ("Mode",             Mode_2d,           0,      Mode_Count,  &m_mode);
}

Convolution::~Convolution()
{
}

bool Convolution::init(const apt::ArgList& _args)
{
	if (!AppBase::init(_args)) {
		return false;
	}

	m_txSrc = Texture::Create("textures/blurtest1.png");
	
	for (uint i = 0; i < APT_ARRAY_COUNT(m_txDst); ++i) {
		m_txDst[i] = Texture::Create2d(m_txSrc->getWidth(), m_txSrc->getHeight(), GL_RGBA8);
		m_txDst[i]->setNamef("txDst[%d]", i);
	}

	initKernel();

	return true;
}

void Convolution::shutdown()
{
	shutdownKernel();

	Texture::Release(m_txDst[0]);
	Texture::Release(m_txDst[1]);
	Texture::Release(m_txSrc);

	AppBase::shutdown();
}

bool Convolution::update()
{
	if (!AppBase::update()) {
		return false;
	}

	bool reinitKernel = false;

	reinitKernel |= ImGui::Combo("Type", &m_type,
		"Box\0"
		"Gaussian\0"
		"Binomial\0"
		);
	reinitKernel |= ImGui::Combo("Mode", &m_mode,
		"2d\0"
		"2d Bilinear\0"
		"Seperable\0"
		"Seperable Bilinear\0"
		);
	reinitKernel |= ImGui::SliderInt("Size", &m_size, 1, 21);
	if (m_type == Type_Gaussian) {
		reinitKernel |= ImGui::SliderFloat("Sigma", &m_gaussianSigma, 0.0f, 4.0f);
		ImGui::Text("Optimal Sigma: %f", m_gaussianSigmaOptimal);
	}
	
	if (reinitKernel) {
		initKernel();
	}

	ImGui::Checkbox("Show Kernel", &m_showKernel);
	if (m_showKernel) {
		bool is2d = m_mode == Mode_2d || m_mode == Mode_2dBilinear;
		bool isBilinear = m_mode == Mode_2dBilinear || m_mode == Mode_SeparableBilinear;

		ImGui::Begin("Kernel", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
			String<128> clipboardStr;
			ImGui::Text("Weights:");
			int cols = isBilinear ? m_size / 2 + 1 : m_size;
			int rows = is2d ? cols : 1;
			for (int i = 0; i < rows; ++i) {
				String<128> rowStr;
				for (int j = 0; j < cols; ++j) {
					int k = i * cols + j;
					rowStr.appendf("%1.4f   ", m_weights[k]);
					clipboardStr.appendf("%f, ", m_weights[k]);
				}
				ImGui::Text((const char*)rowStr);
				clipboardStr.appendf("\n");
			}
			if (ImGui::Button(ICON_FA_CLIPBOARD "##Weights")) {
				ImGui::SetClipboardText((const char*)clipboardStr);
			}

			clipboardStr.clear();
			ImGui::Spacing();
			ImGui::Text("Offsets:");
			for (int i = 0; i < rows; ++i) {
				String<64> rowStr;
				for (int j = 0; j < cols; ++j) {
					int k = i * cols + j;
					if (is2d) {
						rowStr.appendf("(%+1.4f, %+1.4f)   ", m_offsets[k * 2], m_offsets[k * 2 + 1]);
						clipboardStr.appendf("(%f, %f), ", m_offsets[k * 2], m_offsets[k * 2 + 1]);
					} else {
						rowStr.appendf("%+1.4f   ", m_offsets[k]);
						clipboardStr.appendf("%f, ", m_offsets[k]);
					}
				}
				ImGui::Text((const char*)rowStr);
				clipboardStr.appendf("\n");
			}
			if (ImGui::Button(ICON_FA_CLIPBOARD "##Offsets")) {
				ImGui::SetClipboardText((const char*)clipboardStr);
			}
		ImGui::End();
	}
	ImGui::Checkbox("Show Kernel Graph", &m_showKernelGraph);
	if (m_showKernelGraph) {
		bool is2d = m_mode == Mode_2d || m_mode == Mode_2dBilinear;
		bool isBilinear = m_mode == Mode_2dBilinear || m_mode == Mode_SeparableBilinear;

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		vec2 graphBeg  = vec2(ImGui::GetCursorPos()) + vec2(ImGui::GetWindowPos());
		vec2 graphSize = vec2(ImGui::GetContentRegionAvailWidth(), 200.0f);
		vec2 graphEnd  = graphBeg + graphSize;
		float graphScale = 0.0f;
		int n = isBilinear ? m_size / 2 + 1 : m_size;
		for (int i = 0; i < n; ++i) {
			graphScale = APT_MAX(graphScale, m_weights[i]);
		}
		graphScale *= 1.1f;

		drawList->AddRectFilled(graphBeg, graphEnd, IM_COL32_BLACK);
		ImGui::PushClipRect(graphBeg - vec2(1.0f), graphEnd + vec2(1.0f), true);
		
	 // sample the function directly
		if (m_type == Type_Gaussian) {
			const int directSampleCount = (int)graphSize.x / 4;
			vec2 q = graphBeg + vec2(0.0f, graphSize.y);
			for (int i = 0; i < directSampleCount; ++i) {
				vec2 p;
				p.x = (float)i / (float)directSampleCount;
				float d = p.x * (float)m_size - (float)m_size * 0.5f;
				p.y = GaussianDistribution(d, m_gaussianSigma, m_gaussianSigma * m_gaussianSigma) / m_kernelSum / graphScale;
				p.x = graphBeg.x + p.x * graphSize.x;
				p.y = graphEnd.y - p.y * graphSize.y;
				drawList->AddLine(q, p, IM_COL32_MAGENTA);
				q = p;
			}
		}

	 // draw computed weights at offsets
		const float texelCoverage = isBilinear ? 2.0f : 1.0f;
		const int offsetStride = is2d ? 2 : 1;
		const float rectSize = graphSize.x / (float)m_size * texelCoverage;
		float* weights = is2d ? m_weights + (m_kernelSize / 2) : m_weights;
		float* offsets = is2d ? m_offsets + (m_kernelSize / 2 * offsetStride) : m_offsets;
		for (int i = 0; i < n; ++i) {
			vec2 p;
			p.x = *offsets / m_size + 0.5f;
			p.y = *weights / graphScale;
			p.x = floor(graphBeg.x + p.x * graphSize.x);
			p.y = floor(graphEnd.y - p.y * graphSize.y);
			drawList->AddLine(vec2(p.x, graphEnd.y), p, IM_COL32_WHITE);
			drawList->AddCircleFilled(p, 3.0f, IM_COL32_WHITE);

		 // texel coverage per sample
			p.x = (float)i / (float)m_size * texelCoverage;
			p.x = floor(graphBeg.x + p.x * graphSize.x);
			drawList->AddRectFilled(vec2(p.x, graphEnd.y), vec2(floor(p.x + rectSize), p.y), IM_COLOR_ALPHA(IM_COL32_WHITE, 0.2f));

			weights += 1;
			offsets += offsetStride;
		}

	 // texel boundaries
		for (int i = 0; i <= m_size; ++i) {
			float x = floor(graphBeg.x + (float)i / (float)m_size * graphSize.x);
			drawList->AddLine(vec2(x, graphBeg.y), vec2(x, graphEnd.y), IM_COL32_RED);
		}

		ImGui::PopClipRect();
	}


	return true;
}

void Convolution::draw()
{
	GlContext* ctx = GlContext::GetCurrent();

	bool is2d = m_mode == Mode_2d || m_mode == Mode_2dBilinear;
	
	{	AUTO_MARKER("Convolution");
		ctx->setShader(m_shConvolution);
		if (is2d) {
			ctx->setUniformArray("uWeights", m_weights, m_kernelSize);
			ctx->setUniformArray("uOffsets", (vec2*)m_offsets, m_kernelSize);
		} else {
			ctx->setUniformArray("uWeights", m_weights, m_kernelSize);
			ctx->setUniformArray("uOffsets", m_offsets, m_kernelSize);
		}

		if (is2d) {
			ctx->bindTexture("txSrc", m_txSrc);
			ctx->bindImage  ("txDst", m_txDst[0], GL_WRITE_ONLY);
			ctx->dispatch   (m_txDst[0]);

		} else {
			ctx->bindTexture("txSrc", m_txSrc);
			ctx->bindImage  ("txDst", m_txDst[1], GL_WRITE_ONLY);
			ctx->setUniform	("uDirection", vec2(1.0f, 0.0f));
			ctx->dispatch   (m_txDst[1]);
			glAssert(glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT));
			ctx->clearTextureBindings();
			ctx->clearTextureBindings();
			ctx->bindTexture("txSrc", m_txDst[1]);
			ctx->bindImage  ("txDst", m_txDst[0], GL_WRITE_ONLY);
			ctx->setUniform	("uDirection", vec2(0.0f, 1.0f));
			ctx->dispatch   (m_txDst[0]);

		}
	}

	AppBase::draw();
}

void Convolution::initKernel()
{
	shutdownKernel();

 // force size to be odd
	if (m_size % 2 == 0) {
		++m_size;
	}

	bool is2d = m_mode == Mode_2d ;//|| m_mode == Mode_2dBilinear;
	
	const int kernelDims = is2d ? 2 : 1;
	const int hsize = m_size / 2;
	m_kernelSize = m_size * (kernelDims == 2 ? m_size : 1);

 // offsets
	m_offsets = new float[m_kernelSize * kernelDims]; // offsets are vec2 for a 2d kernel
	if (is2d) {
		for (int i = 0; i < m_size; ++i) {
			float y = (float)(i - hsize);
			for (int j = 0; j < m_size; ++j) {
				float x = (float)(j - hsize);
				int k = (i * m_size + j);
				m_offsets[k * 2] = x;
				m_offsets[k * 2 + 1] = y;
			}
		}
	} else {
		for (int i = 0; i < m_size; ++i) {
			m_offsets[i] = (float)(i - hsize);
		}
	}

 // weights
	m_weights = new float[m_kernelSize];
	switch (m_type) {
	case Type_Box:
		for (int i = 0; i < m_kernelSize; ++i) {
			m_weights[i] = 1.0f / (float)m_kernelSize;
		}
		m_kernelSum = 1.0f;
		break;
	case Type_Gaussian:
		if (is2d) {
			m_kernelSum = GaussianKernel2d(m_size, m_gaussianSigma, m_weights);
		} else {
			m_kernelSum = GaussianKernel1d(m_size, m_gaussianSigma, m_weights);
		}
		m_gaussianSigmaOptimal = GaussianFindSigma(m_size, 1.0f / 255.0f);
		break;
	case Type_Binomial:
		if (is2d) {
			m_kernelSum = BinomialKernel2d(m_size, m_weights);
		} else {
			m_kernelSum = BinomialKernel1d(m_size, m_weights);
		}
		break;
	default:
		APT_ASSERT(false);
		break;
	};

	switch (m_mode) {
	case Mode_2dBilinear: {
		int size = m_size / 2 + 1;
		float* weightsOpt = new float[size];
		float* offsetsOpt = new float[size];
		KernelOptimizerBilinear1d(m_size, m_weights, weightsOpt, offsetsOpt);
		delete[] m_weights;
		delete[] m_offsets;
		m_weights = weightsOpt;
		m_offsets = offsetsOpt;

	m_kernelSize = size * size;
	weightsOpt = new float[m_kernelSize];
	offsetsOpt = new float[m_kernelSize * 2];
	m_kernelSum *= m_kernelSum;
 // derive subsequent rows from the first
	for (int i = 1; i < size; ++i) {
		for (int j = 0; j < size; ++j) {
			int k = i * size + j;
			weightsOpt[k] = (m_weights[i] * m_weights[j]) / m_kernelSum;
		}
	}
 // copy the first row from the last
	for (int i = 0; i < size; ++i) {
		weightsOpt[i] = weightsOpt[(size - 1) * size + i];
	}
 // offsets are symmetrical in both dimensions
	for (int i = 0; i < size; ++i) {
		for (int j = 0; j < size; ++j) {
			int k = (i * size + j) * 2;
			offsetsOpt[k] = m_offsets[j];
			offsetsOpt[k + 1] = m_offsets[i];
		}
	}
	delete[] m_weights;
	delete[] m_offsets;
	m_weights = weightsOpt;
	m_offsets = offsetsOpt;

	break;
		}
	case Mode_SeparableBilinear: {
		m_kernelSize = m_size / 2 + 1;
		float* weightsOpt = new float[m_kernelSize];
		float* offsetsOpt = new float[m_kernelSize];
		KernelOptimizerBilinear1d(m_size, m_weights, weightsOpt, offsetsOpt);
		delete[] m_weights;
		delete[] m_offsets;
		m_weights = weightsOpt;
		m_offsets = offsetsOpt;
		break;
		}
	default:
		break;
	}
	
 // shader
	ShaderDesc shDesc;
	shDesc.setPath(GL_COMPUTE_SHADER, "shaders/Convolution_cs.glsl");
	shDesc.setLocalSize(8, 8);
	shDesc.addDefine(GL_COMPUTE_SHADER, "TYPE", m_type);
	shDesc.addDefine(GL_COMPUTE_SHADER, "MODE", m_mode);
	shDesc.addDefine(GL_COMPUTE_SHADER, "KERNEL_SIZE", m_kernelSize);
	m_shConvolution = Shader::Create(shDesc);
}

void Convolution::shutdownKernel()
{
	delete[] m_weights;
	delete[] m_offsets;

	Shader::Release(m_shConvolution);
}