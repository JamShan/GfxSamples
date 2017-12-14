/*
	Notes:
	- Image precision has an effect on the useful kernel size (see 1). E.g. for an 8-bit image, weights <1/255 have no effect.
	- Binomial distribution may be preferable to a truncated Gaussian (see 1).

	References:
	(1) http://www.stat.wisc.edu/~mchung/teaching/MIA/reading/diffusion.gaussian.kernel.pdf.pdf
	(2) https://d3cw3dd2w32x2b.cloudfront.net/wp-content/uploads/2012/06/faster_filters.pdf
	(3) http://rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/
*/

#include <cassert>
#include <cstdio>
#define _USE_MATH_DEFINES
#include <cmath>

#define USE_INTEGRATION 1

#define GaussianIntegration(_mind, _maxd, _sigma, _sigma2) GaussianIntegration_Trapezoid(_mind, _maxd, _sigma, _sigma2)

inline float GaussianDistribution(float _d, float _sigma, float _sigma2)
{
	float d = (_d * _d) / (2.0f * _sigma2);
	return 1.0f / (M_PI_2 * _sigma2) * exp(-d);
}

float GaussianIntegration_Riemann(float _mind, float _maxd, float _sigma, float _sigma2)
{
	const int sampleCount = 100;
	const float w = 1.0f / (float)(sampleCount - 1);
	const float stepd = (_maxd - _mind) * w;
	float d = _mind;
	float ret = 0.0f;
	for (int i = 0; i < sampleCount; ++i) {
		ret += GaussianDistribution(d, _sigma, _sigma2) * w;
		d += stepd;
	}
	return ret;
}

float GaussianIntegration_Trapezoid(float _mind, float _maxd, float _sigma, float _sigma2)
{
	const int sampleCount = 64;
	const float w = 1.0f / (float)(sampleCount - 1);
	const float stepd = (_maxd - _mind) * w;
	float ret = GaussianDistribution(_mind, _sigma, _sigma2);
	float d = _mind + stepd;
	for (int i = 1; i < sampleCount - 1; ++i) {
		ret += GaussianDistribution(d, _sigma, _sigma2) * 2.0f;
		d += stepd;
	}
	ret += GaussianDistribution(d, _sigma, _sigma2);
	return ret * stepd / 2.0f;
}


void GaussianKernel1d(int _size, float _sigma, float* weights_)
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
	for (int i = 0; i < _size; ++i) {
		weights_[i] /= sum;
	}
}

void GaussianKernel2d(int _size, float _sigma, float* weights_)
{
 // force _size to be odd
	if (_size % 2 == 0) {
		++_size;
	}

 // generate first row
	GaussianKernel1d(_size, _sigma, weights_);
	
 // derive subsequent rows from the first
	for (int i = 1; i < _size; ++i) {
		for (int j = 0; j < _size; ++j) {
			int k = i * _size + j;
			weights_[k] = weights_[i] * weights_[j];
		}
	}
 // copy the first row from the last
	for (int i = 0; i < _size; ++i) {
		weights_[i] = weights_[(_size - 1) * _size + i];
	}
}

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

int main(int _argv, char** _argc)
{
	const int size = 11;
	const float sigma = 1.9791f;

 	float kernel1d[size];
	GaussianKernel1d(size, sigma, kernel1d);
	printf("kernel1d:\n");
	float sum1d = 0.0f;
	for (int i = 0; i < size; ++i) {
		printf("%1.6f  ", kernel1d[i]);
		sum1d += kernel1d[i];
	}
	printf("\nsum1d = %1.6f\n\n", sum1d);

	/*float kernel2d[size * size];
	GaussianKernel2d(size, sigma, kernel2d);
	printf("kernel2d:\n");
	float sum2d = 0.0f;
	for (int i = 0; i < size; ++i) {
		for (int j = 0; j < size; ++j) {
			int k = i * size + j;
			printf("%1.6f  ", kernel2d[k]);
			sum2d += kernel2d[k];
		}
		printf("\n");
	}
	printf("sum2d = %1.6f\n\n", sum2d);*/

 // optim
	const int sizeOpt = size / 2 + 1;
	float kernel1dOpt[sizeOpt];
	float offsets1dOpt[sizeOpt];
	KernelOptimizerBilinear1d(size, kernel1d, kernel1dOpt, offsets1dOpt);
	for (int i = 0; i < sizeOpt; ++i) {
		printf("%1.6f  ", kernel1dOpt[i]);
	}
	printf("\n");
	for (int i = 0; i < sizeOpt; ++i) {
		printf("%1.6f  ", offsets1dOpt[i]);
	}
	printf("\n\n");

	return 0;
}
