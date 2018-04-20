#ifndef INCLUDED_RENDER_HPP
#define INCLUDED_RENDER_HPP

#include <vector>
#include <memory>

class bitmap_window {
	class fn_wrapper {
	public:
		virtual void perform(void) const = 0;
		virtual ~fn_wrapper(void) = default;
	};

	template <typename Fn>
	class fn_wrapper_impl : public fn_wrapper {
		Fn fn;

	public:
		fn_wrapper_impl(Fn fnP) : fn(fnP) {}
		void perform(void) const {
			fn();
		}
	};

	int w;
	int h;
	std::vector<unsigned char> img;
	void *data;
	std::unique_ptr<fn_wrapper> displayFunc;
	std::unique_ptr<fn_wrapper> exitFunc;

	static void global_idle(void);
	static void global_display(void);
	static void global_exit(void);

	void idle(void);
	void display(void);
	void exit(void);

public:
	static void initialise(int *argc, char **argv);

	bitmap_window(int width, int height, const char *title);
	bitmap_window(const bitmap_window&) = delete;
	bitmap_window(bitmap_window&&) = delete;

	bitmap_window& operator=(const bitmap_window&) = delete;
	bitmap_window& operator=(bitmap_window&&) = delete;

	int width(void) const;
	int height(void) const;
	std::vector<unsigned char> &image_data(void);
	const std::vector<unsigned char> &image_data(void) const;
	void run(void);

	template <typename Fn>
	void set_display_func(Fn fn) {
		displayFunc = std::unique_ptr<fn_wrapper>(new fn_wrapper_impl<Fn>(fn));
	}

	template <typename Fn>
	void set_exit_func(Fn fn) {
		exitFunc = std::unique_ptr<fn_wrapper>(new fn_wrapper_impl<Fn>(fn));
	}

	~bitmap_window(void);
};

#endif
