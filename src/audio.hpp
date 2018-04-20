#ifndef INCLUDED_AUDIO_HPP
#define INCLUDED_AUDIO_HPP

#include <memory>
#include <vector>

class echolocator_internal {
public:
	virtual void run_async(void) = 0;
	virtual void analyse(void) = 0;
	virtual bool is_calibrated(void) const = 0;
	virtual std::size_t searches_count(void) const = 0;
	virtual const std::vector<double> &observations(std::size_t search) const = 0;

	virtual ~echolocator_internal(void) = default;
};

class echolocator {
	std::unique_ptr<echolocator_internal> impl;

public:
	echolocator(int sample_rate);

	inline void run_async(void) {
		impl->run_async();
	}

	inline void analyse(void) {
		impl->analyse();
	}

	inline bool is_calibrated(void) const {
		return impl->is_calibrated();
	}

	inline std::size_t searches_count(void) const {
		return impl->searches_count();
	}

	inline const std::vector<double> &observations(std::size_t search) const {
		return impl->observations(search);
	}
};

#endif
