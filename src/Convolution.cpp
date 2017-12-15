#include "Convolution.h"

#include <frm/def.h>
#include <frm/gl.h>
#include <frm/Buffer.h>
#include <frm/Framebuffer.h>
#include <frm/GlContext.h>
#include <frm/Mesh.h>
#include <frm/Shader.h>
#include <frm/Texture.h>
#include <frm/Window.h>

#include <apt/ArgList.h>

using namespace frm;
using namespace apt;

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

	m_txSrc = Texture::Create("textures/lena.png");
	
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
		);
	reinitKernel |= ImGui::Combo("Mode", &m_mode,
		"2d\0"
		"Seperable\0"
		"Seperable Bilinear\0"
		);
	reinitKernel |= ImGui::SliderInt("Size", &m_size, 1, 21);
	if (m_type == Type_Gaussian) {
		reinitKernel |= ImGui::SliderFloat("Sigma", &m_gaussianSigma, 0.0f, 4.0f);
	}

	ImGui::Checkbox("Show Kernel", &m_showKernel);
	if (m_showKernel) {
		ImGui::Begin("Kernel", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
			ImGui::Text("Weights:");
			int rows = m_mode == Mode_2d ? m_size : 1;
			for (int i = 0; i < rows; ++i) {
				String<64> rowStr;
				for (int j = 0; j < m_size; ++j) {
					int k = i * m_size + j;
					rowStr.appendf("%1.4f   ", m_weights[k]);
				}
				ImGui::Text((const char*)rowStr);
			}

			ImGui::Spacing();
			ImGui::Text("Offsets:");
			for (int i = 0; i < rows; ++i) {
				String<64> rowStr;
				for (int j = 0; j < m_size; ++j) {
					int k = i * m_size + j;
					if (m_mode == Mode_2d) {
						rowStr.appendf("(%+1.4f, %+1.4f)   ", m_offsets[k * 2], m_offsets[k * 2 + 1]);
					} else {
						rowStr.appendf("%+1.4f   ", m_offsets[k]);
					}
				}
				ImGui::Text((const char*)rowStr);
			}
		ImGui::End();
	}

	if (reinitKernel) {
		initKernel();
	}

	return true;
}

void Convolution::draw()
{
	// sample code here

	AppBase::draw();
}

void Convolution::initKernel()
{
	shutdownKernel();

 // force size to be odd
	if (m_size % 2 == 0) {
		++m_size;
	}

	const int kernelDims = m_mode == Mode_2d ? 2 : 1;
	const int size = m_size * (kernelDims == 2 ? m_size : 1);
	const int hsize = m_size / 2;

 // offsets
	m_offsets = new float[size * kernelDims]; // offsets are vec2 for a 2d kernel
	if (m_mode == Mode_2d) {
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
	m_weights = new float[size];
	switch (m_type) {
	case Type_Box:
		for (int i = 0; i < size; ++i) {
			m_weights[i] = 1.0f / (float)size;
		}
		break;
	case Type_Gaussian:
		break;
	default:
		APT_ASSERT(false);
		break;
	};

 // gpu buffer
	int bfSize = (size + size * kernelDims) * sizeof(float);
	float* bfData = new float[bfSize];
	memcpy(bfData, m_weights, size * sizeof(float));
	memcpy(bfData + size * sizeof(float), m_weights, size * kernelDims * sizeof(float));
	m_bfKernel = Buffer::Create(GL_UNIFORM_BUFFER, bfSize, 0, bfData);
	m_bfKernel->setName("_bfKernel");
	delete[] bfData;

 // shader
	ShaderDesc shDesc;
	shDesc.setPath(GL_COMPUTE_SHADER, "shaders/Convolution_cs.glsl");
	shDesc.setLocalSize(8, 8);
	shDesc.addDefine(GL_COMPUTE_SHADER, "TYPE", m_type);
	shDesc.addDefine(GL_COMPUTE_SHADER, "MODE", m_mode);
	shDesc.addDefine(GL_COMPUTE_SHADER, "KERNEL_SIZE", m_size);
	m_shConvolution = Shader::Create(shDesc);
}

void Convolution::shutdownKernel()
{
	delete[] m_weights;
	delete[] m_offsets;

	Shader::Release(m_shConvolution);
	Buffer::Destroy(m_bfKernel);
}