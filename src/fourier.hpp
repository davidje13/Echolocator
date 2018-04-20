#ifndef INCLUDED_FOURIER_HPP
#define INCLUDED_FOURIER_HPP

#include <fftw3.h>

#include <cmath>

void deconvolve_freq(
	std::size_t size,
	const fftw_complex *num,
	const fftw_complex *den,
	fftw_complex *target,
	double dampingCheat
) {
	for(std::size_t i = 0; i < size; ++ i) {
		double denom = 1.0 / (den[i][0] * den[i][0] + den[i][1] * den[i][1] + dampingCheat);
		double re = num[i][0] * den[i][0] + num[i][1] * den[i][1];
		double im = num[i][1] * den[i][0] - num[i][0] * den[i][1];
		target[i][0] = re * denom;
		target[i][1] = im * denom;
	}
}

class fft {
	fftw_complex *pSpace;
	fftw_complex *fSpace;
	fftw_plan forwardPlan;
	fftw_plan reversePlan;
	std::size_t sz;

public:
	fft(std::size_t size)
		: pSpace((fftw_complex *) fftw_malloc(size * sizeof(fftw_complex)))
		, fSpace((fftw_complex *) fftw_malloc(size * sizeof(fftw_complex)))
		, forwardPlan(fftw_plan_dft_1d(
			int(size),
			pSpace,
			fSpace,
			FFTW_FORWARD,
			FFTW_MEASURE
		))
		, reversePlan(fftw_plan_dft_1d(
			int(size),
			fSpace,
			pSpace,
			FFTW_BACKWARD,
			FFTW_MEASURE
		))
		, sz(size)
	{}

	fft(const fft&) = delete;
	fft(fft &&c)
		: pSpace(c.pSpace)
		, fSpace(c.fSpace)
		, forwardPlan(c.forwardPlan)
		, reversePlan(c.reversePlan)
		, sz(c.sz)
	{
		c.pSpace = nullptr;
		c.fSpace = nullptr;
		c.forwardPlan = nullptr;
		c.reversePlan = nullptr;
	}

	fft &operator=(const fft&) = delete;
	fft &operator=(fft &&c) {
		pSpace = c.pSpace;
		fSpace = c.fSpace;
		forwardPlan = c.forwardPlan;
		reversePlan = c.reversePlan;
		sz = c.sz;

		c.pSpace = nullptr;
		c.fSpace = nullptr;
		c.forwardPlan = nullptr;
		c.reversePlan = nullptr;

		return *this;
	}

	std::size_t size(void) const {
		return sz;
	}

	fftw_complex *p(void) {
		return pSpace;
	}

	const fftw_complex *p(void) const {
		return pSpace;
	}

	fftw_complex *f(void) {
		return fSpace;
	}

	const fftw_complex *f(void) const {
		return fSpace;
	}

	void pToF(void) {
		fftw_execute(forwardPlan);
	}

	void fToP(void) {
		fftw_execute(reversePlan);
	}

	~fft(void) {
		if(forwardPlan != nullptr) {
			fftw_destroy_plan(forwardPlan);
			forwardPlan = nullptr;
		}
		if(reversePlan != nullptr) {
			fftw_destroy_plan(reversePlan);
			reversePlan = nullptr;
		}
		if(pSpace != nullptr) {
			fftw_free(pSpace);
			pSpace = nullptr;
		}
		if(fSpace != nullptr) {
			fftw_free(fSpace);
			fSpace = nullptr;
		}
	}
};

#endif
