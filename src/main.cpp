#include "render.hpp"
#include "audio.hpp"
#include "chirps.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>

unsigned char to_saturated_char(double v, double low, double high) {
	double scaled = (v - low) * 256.0 / (high - low);
	return (unsigned char) std::max(0.0, std::min(255.5, scaled));
}

void render_locator_to_output(const echolocator &locator, bitmap_window &output) {
	std::size_t n = locator.searches_count();
	auto &dat = output.image_data();
	std::size_t w = std::size_t(output.width());
	std::size_t bandh = std::size_t(output.height()) / n;
	double scale = 0.5;

	// For each observer
	for(std::size_t i = 0; i < n; ++ i) {
		const auto &obs = locator.observations(i);

		// Normalise range of outputs (control for volume & damping)
		double sum = 0;
		double sum2 = 0;
		for(std::size_t x = 0; x < w; ++ x) {
			double v = std::abs(obs[std::size_t(x*scale)]);
			sum += v;
			sum2 += v * v;
		}
		double avg = sum / w;
		double variance = sum2 / w - avg * avg;
		double sd = std::sqrt(variance);

		// Render
		for(std::size_t x = 0; x < w; ++ x) {
			unsigned char v = to_saturated_char(
				std::abs(obs[std::size_t(x*scale)]),
				avg,
				avg + sd * 3
			);
			for(std::size_t y = 0; y < bandh; ++ y) {
				std::size_t p = (i * bandh + y) * w + x;
				dat[p] = 255 - v;
			}
		}
	}
}

int main(int argc, char **argv) {
	try {
		std::cerr << "Initialising display..." << std::endl;
		bitmap_window::initialise(&argc, argv);
		std::cerr << "Creating window..." << std::endl;
		bitmap_window output(1024, 512, "Echolocator");

		std::cerr << "Drawing test chirp..." << std::endl;
		repeating_chirp test(chirp(
			tone(100, 1.0 / 200),
			tone(100, 1.0 / 10),
			200
		), 25, 250);

		for(double x = 0; x < 1024; x += 0.01) {
			double y = test.sample(x) + output.height() / 2;
			output.image_data()[std::size_t(y) * 1024 + std::size_t(x)] = 255;
		}

		std::cerr << "Creating echolocator..." << std::endl;
		std::unique_ptr<echolocator> locator(new echolocator(96000));

		std::cerr << "Starting echolocator..." << std::endl;
		locator->run_async();

		output.set_display_func([&output, &locator] {
			locator->analyse();
			if(locator->is_calibrated()) {
				render_locator_to_output(*locator, output);
			}
		});

		output.set_exit_func([&locator] {
			std::cerr << std::endl;
			std::cerr << "Shutting down..." << std::endl;
			locator = nullptr; // important to terminate audio on shutdown
			std::cerr << "Done." << std::endl;
		});

		std::cerr << "Showing display..." << std::endl;
		output.run();

		return EXIT_SUCCESS;
	} catch(const std::exception &ex) {
		std::cerr << "Exception: " << ex.what() << std::endl;
	}
}
