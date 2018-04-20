#ifndef INCLUDED_CHIRPS_HPP
#define INCLUDED_CHIRPS_HPP

#include <cmath>

class tone {
public:
	double amplitude;
	double frequency;

	inline tone(double amplitudeP, double frequencyP)
		: amplitude(amplitudeP)
		, frequency(frequencyP)
	{}

	inline double sample(double time) const {
		return amplitude * std::sin(time * frequency * M_PI * 2);
	}
};

class chirp {
	double a0;
	double aD;
	double f0;
	double fD;
	double d;

public:
	inline double sample(double time) const {
		if(time < 0 || time >= d) {
			return 0;
		}

		return (a0 + time * aD) * std::sin(time * (f0 + time * fD));
	}

	inline chirp(tone start, tone end, double duration)
		: a0(start.amplitude)
		, aD((end.amplitude - start.amplitude) / duration)
		, f0(start.frequency * M_PI * 2)
		, fD((end.frequency - start.frequency) * M_PI / duration)
		, d(duration)
	{}
};

class repeating_chirp {
	chirp c;
	double o;
	double d;

public:
	inline repeating_chirp(chirp ch, double offset, double duration)
		: c(ch)
		, o(offset)
		, d(duration)
	{}

	inline double time_since_start(double time) const {
		return std::fmod(time - o, d);
	}

	inline double sample(double time) const {
		return c.sample(time_since_start(time));
	}
};

#endif
