#include "audio.hpp"
#include "fourier.hpp"
#include "chirps.hpp"

#include <portaudio.h>
#include <fftw3.h>

#include <cmath>
#include <vector>
#include <stdexcept>

#include <iostream> // debug only

const double SPEED_OF_SOUND = 340.0; // metres/second

class recorder {
	std::vector<double> memory;
	std::size_t position;
	std::size_t total;

public:
	recorder(std::size_t capacity)
		: memory(capacity, 0.0)
		, position(0)
		, total(0)
	{}

	void record(double sample, double time) {
		(void) time;

		memory[position] = sample;
		if(++ position == memory.size()) {
			position = 0;
		}
		++ total;
	}

	std::size_t capacity(void) const {
		return memory.size();
	}

	std::size_t latest(void) const {
		return total;
	}

	double get(std::size_t p) const {
		return memory[p%memory.size()];
	}
};

class searcher {
	const recorder *r;
	fft transformer;
	std::vector<double> needleFreq;
	std::size_t sz;
	std::size_t negativeSpace;
	double shift;

	std::size_t nextRec;
	std::vector<double> results;
	std::size_t calibrationTime;
	std::size_t calibrationP;

public:
	searcher(
		std::size_t size,
		std::size_t resultsSize,
		double sampleRate,
		const recorder *rec,
		const chirp &needle
	)
		: r(rec)
		, transformer(size)
		, needleFreq(size*2)
		, sz(size)
		, negativeSpace(40) // space to show to the left of the calibration mark
		, shift(50) // estimated sample count between speaker and microphone (chosen for clarity; in reality probably closer to 10)
		, nextRec(0)
		, results(resultsSize, 0.0)
		, calibrationTime(std::size_t(sampleRate * 2))
		, calibrationP(0)
	{
		// Calculate frequency spectrum of ideal needle
		auto posn = transformer.p();
		auto freq = transformer.f();

		for(std::size_t i = 0; i < sz; ++ i) {
			posn[i][0] = needle.sample(double(i) / sampleRate);
			posn[i][1] = 0;
		}
		transformer.pToF();
		memcpy(&needleFreq[0], freq, sz * sizeof(fftw_complex));
	}

	searcher(const searcher&) = delete;
	searcher(searcher&&) = default;

	searcher &operator=(const searcher&) = delete;
	searcher &operator=(searcher&&) = default;

	void perform_calibration(void) {
		// Find strongest signal to anchor against
		// (will most likely be the immediate feedback loop timing)
		calibrationP = 0;
		double maxV = -1;
		std::size_t rs = results.size();
		for(std::size_t i = 0; i < rs; ++ i) {
			double v = std::abs(results[i]);
			if(v > maxV) {
				calibrationP = i;
				maxV = v;
			}
		}
		calibrationP = (calibrationP + rs - negativeSpace) % rs;
	}

	void analyse_next_batch(void) {
		if(nextRec + sz > r->latest()) {
			throw std::runtime_error("not enough data");
		}

		double dist0 = (shift - negativeSpace);
		double scaleFactor = std::pow(sz, -0.5);

		auto posn = transformer.p();
		auto freq = transformer.f();

		for(std::size_t i = 0; i < sz; ++ i) {
			posn[i][0] = r->get(nextRec + i);
			posn[i][1] = 0;
		}
		transformer.pToF();
		deconvolve_freq(
			sz,
			freq,
			(fftw_complex*) &needleFreq[0],
			freq,
			100.0
		);
		transformer.fToP();
		// splitting convolution into parts only works for middle half
		std::size_t rs = results.size();
		std::size_t p0 = nextRec + sz/4 - calibrationP;
		for(std::size_t i = sz/4, e = i + sz/2; i < e; ++ i) {
			std::size_t p = (p0 + i) % rs;
			// Increase power with d, since sound pressure tails off as d^-1
			double dist = p + dist0;
			results[p] = posn[i%sz][0] * dist * scaleFactor;
		}
		nextRec += sz/2;
	}

	void update(void) {
		std::size_t latest = r->latest();
		std::size_t cap = r->capacity();

		if(nextRec < latest - cap) {
			// must be falling behind; skip to current
			std::cerr << "!";
			nextRec = latest - sz;
		}

		if(nextRec < calibrationTime) {
			// Calibration stage; store raw sound data
			std::size_t rs = results.size();
			while(nextRec < latest) {
				results[nextRec%rs] = r->get(nextRec);
				++ nextRec;
			}
			if(nextRec >= calibrationTime) {
				perform_calibration();
			}
		} else {
			// Runtime: deconvolve against needle to find echos
			while(nextRec + sz <= latest) {
				analyse_next_batch();
			}
		}
	}

	bool is_calibrated(void) const {
		return (nextRec >= calibrationTime);
	}

	const std::vector<double> &observations(void) const {
		return results;
	}
};

class echolocator_impl : public echolocator_internal {
	std::vector<repeating_chirp> outputs;
	std::vector<recorder> inputs;
	std::vector<searcher> searchers;
	double secondsPerFrame;
	double framesPerSecond;
	PaStream *stream;

	double tmOutput;
	double tmInput;

	int stream_callback(
		const void *input,
		void *output,
		unsigned long frameCount,
		const PaStreamCallbackTimeInfo *timeInfo,
		PaStreamCallbackFlags statusFlags
	) {
		(void) statusFlags;
		(void) timeInfo;

		if(statusFlags != 0) {
			std::cerr << statusFlags;
		}

//		timeInfo->outputBufferDacTime;
//		timeInfo->inputBufferAdcTime;

		// Populate speaker audio
		std::size_t n = outputs.size();
		float *outputBuffer = static_cast<float*>(output);
		for(std::size_t i = 0; i < frameCount; ++ i) {
			for(std::size_t j = 0; j < n; ++ j) {
				outputBuffer[i*n+j] = float(outputs[j].sample(tmOutput));
			}
			tmOutput += secondsPerFrame;
		}

		// Check microphone audio
		n = inputs.size();
		const float *inputBuffer = static_cast<const float*>(input);
		for(std::size_t i = 0; i < frameCount; ++ i) {
			for(std::size_t j = 0; j < n; ++ j) {
				inputs[j].record(double(inputBuffer[i*n+j]), tmInput);
			}
			tmInput += secondsPerFrame;
		}

		return paContinue;
	}

	static int global_stream_callback(
		const void *input,
		void *output,
		unsigned long frameCount,
		const PaStreamCallbackTimeInfo *timeInfo,
		PaStreamCallbackFlags statusFlags,
		void *userData
	) {
		return ((echolocator_impl*) userData)->stream_callback(
			input,
			output,
			frameCount,
			timeInfo,
			statusFlags
		);
	}

public:
	inline echolocator_impl(int sample_rate)
		: outputs()
		, inputs()
		, secondsPerFrame(1.0 / sample_rate)
		, framesPerSecond(sample_rate)
		, stream(nullptr)
	{
		PaError error = Pa_Initialize();
		if(error != paNoError) {
			throw std::runtime_error("Pa_Initialize failed");
		}
	}

	void run_async(void) {
		// Configuration

		double step = 0.025;
		double chirpDuration = 0.004;
//		step = 2.5;
//		chirpDuration = 0.4;

		chirp baseChirp(
			tone(1.0, 1000.0),
			tone(1.0, 20000.0),
			chirpDuration
		);

		std::cerr
			<< "Step: "
			<< step << " seconds ("
			<< (1.0 / step)
			<< " pulses per second)"
			<< std::endl
			<< "Max detectable distance: ~"
			<< (step * SPEED_OF_SOUND * 0.5)
			<< " metres (assuming 1 speaker)"
			<< std::endl;

		// Calculated properties

		// Kernel must be at least twice the size of the chirp,
		// and should be PoT for best performance
		std::size_t fftKernel = 16;
		while(double(fftKernel) < 2 * chirpDuration * framesPerSecond) {
			fftKernel <<= 1;
		}
		std::cerr
			<< "Chirp samples: "
			<< int(chirpDuration * framesPerSecond)
			<< std::endl
			<< "FFT kernel size: "
			<< fftKernel
			<< std::endl;

		double framesPerStepRaw = framesPerSecond * step;
		std::size_t framesPerStep = std::size_t(framesPerStepRaw + 0.5);
		std::cerr << "Frames per step: " << framesPerStepRaw << std::endl;
		if(std::abs(std::fmod(framesPerStepRaw + 0.5, 1.0) - 0.5) > 0.001) {
			std::cerr << "!!! WARNING: framesPerStep is not an integer; output will drift" << std::endl;
		}

		// Reset state
		inputs.clear();
		outputs.clear();
		searchers.clear();

		// Load input/output device info
		PaDeviceIndex inDevice = Pa_GetDefaultInputDevice();
		PaDeviceIndex outDevice = Pa_GetDefaultOutputDevice();

		const PaDeviceInfo *inDeviceInfo = Pa_GetDeviceInfo(inDevice);
		const PaDeviceInfo *outDeviceInfo = Pa_GetDeviceInfo(outDevice);

		// OSX reports 2 microphones, but they are identical
		int inputCount = 1;//inDeviceInfo->maxInputChannels;
		int outputCount = outDeviceInfo->maxOutputChannels;

		std::cerr
			<< "Input: " << inDeviceInfo->name
			<< " (x" << inputCount << ")"
			<< std::endl;
		std::cerr
			<< "Output: " << outDeviceInfo->name
			<< " (x" << outputCount << ")"
			<< std::endl;

		PaStreamParameters inParams;
		inParams.device = inDevice;
		inParams.channelCount = inputCount;
		inParams.sampleFormat = paFloat32;
		inParams.suggestedLatency = inDeviceInfo->defaultLowInputLatency;
		inParams.hostApiSpecificStreamInfo = nullptr;

		PaStreamParameters outParams;
		outParams.device = outDevice;
		outParams.channelCount = outputCount;
		outParams.sampleFormat = paFloat32;
		outParams.suggestedLatency = outDeviceInfo->defaultLowOutputLatency;
		outParams.hostApiSpecificStreamInfo = nullptr;

		// Check if requested Hz is supported
		PaError error = Pa_IsFormatSupported(
			(inputCount > 0) ? &inParams : nullptr,
			(outputCount > 0) ? &outParams : nullptr,
			double(framesPerSecond)
		);
		if(error != paNoError) {
			throw std::runtime_error("Pa_IsFormatSupported returned false");
		}

		// Create microphone recorders
		for(int i = 0; i < inputCount; ++ i) {
			inputs.emplace_back(std::size_t(framesPerSecond));
		}

		// Create chirp generators
		chirp silence(tone(0, 0), tone(0, 0), 0);
		bool silenceNonZero = true;

		for(int i = 0; i < outputCount; ++ i) {
			outputs.emplace_back(
				(silenceNonZero && i != 0) ? silence : baseChirp,
				i * step / outputCount,
				step
			);
		}

		for(std::size_t o = 0; o < outputs.size(); ++ o) {
			if(silenceNonZero && o != 0) {
				continue;
			}
			for(std::size_t i = 0; i < inputs.size(); ++ i) {
				searchers.emplace_back(
					fftKernel,
					framesPerStep,
					framesPerSecond,
					&inputs[i], baseChirp
				);
			}
		}

		error = Pa_OpenStream(
			&stream,
			(inputCount > 0) ? &inParams : nullptr,
			(outputCount > 0) ? &outParams : nullptr,
			framesPerSecond,
			paFramesPerBufferUnspecified,
			paClipOff | paDitherOff,
			&echolocator_impl::global_stream_callback,
			this
		);
		if(error != paNoError) {
			throw std::runtime_error("Pa_OpenDefaultStream failed");
		}

		error = Pa_StartStream(stream);
		if(error != paNoError) {
			throw std::runtime_error("Pa_StartStream failed");
		}
	}

	void analyse(void) {
		for(std::size_t i = 0; i < searchers.size(); ++ i) {
			searchers[i].update();
		}
	}

	bool is_calibrated(void) const {
		for(std::size_t i = 0; i < searchers.size(); ++ i) {
			if(!searchers[i].is_calibrated()) {
				return false;
			}
		}
		return true;
	}

	std::size_t searches_count(void) const {
		return searchers.size();
	}

	const std::vector<double> &observations(std::size_t search) const {
		return searchers[search].observations();
	}

	~echolocator_impl(void) {
		if(stream != nullptr) {
			Pa_AbortStream(stream);
			Pa_StopStream(stream);
			Pa_CloseStream(stream);
		}
		Pa_Terminate();
		std::cerr << "Audio shutdown complete." << std::endl;
	}
};

echolocator::echolocator(int sample_rate)
	: impl(new echolocator_impl(sample_rate))
{}
