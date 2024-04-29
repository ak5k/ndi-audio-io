#include <cmath>
#include <csignal>
#include <cstddef>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <atomic>
#include <chrono>

#ifdef _WIN32
#include <windows.h>

#ifdef _WIN64
#pragma comment(lib, "Processing.NDI.Lib.x64.lib")
#else // _WIN64
#pragma comment(lib, "Processing.NDI.Lib.x86.lib")
#endif // _WIN64

#ifndef snprintf
#define snprintf _snprintf
#endif

#endif

#include <Processing.NDI.Lib.h>

static std::atomic<bool> exit_loop(false);
static void sigint_handler(int) { exit_loop = true; }

int main(int argc, char* argv[])
{
	// Not required, but "correct" (see the SDK documentation).
	if (!NDIlib_initialize()) {
		// Cannot run NDI. Most likely because the CPU is not sufficient (see SDK documentation).
		// you can check this directly with a call to NDIlib_is_supported_CPU()
		printf("Cannot run NDI.");
		return 0;
	}

	// Catch interrupt so that we can shut down gracefully
	signal(SIGINT, sigint_handler);

	// Create an NDI source that is called "Benchmark". This is not clocked since 
	// we are going to write a benchmark.
	NDIlib_send_create_t NDI_send_create_desc;
	NDI_send_create_desc.p_ndi_name = nullptr;
	NDI_send_create_desc.clock_video = false;
	NDI_send_create_desc.clock_audio = false;

#ifdef _DEBUG
	printf("WARNING. This application should be run in RELEASE mode for accurate results ...\n");
#ifndef _WIN64
	printf("WARNING. This application should be run in x64 mode for the best results ...\n");
#endif // _WIN64
#endif // _DEBUG

	// Display that we're thinking about thins
	printf("Generating content for benchmark ...\n");

	// We create the NDI sender
	NDIlib_send_instance_t pNDI_send = NDIlib_send_create(&NDI_send_create_desc);
	if (!pNDI_send)
		return 0;

	// We build a video frame that is twice to long, which allows us to "scroll" down the image
	// so that the content is changing but it takes no CPU time to generate this.
	const int xres = 7680;
	const int yres = 4320;
	const int framerate_n = 60000;
	const int framerate_d = 1001;

	// Allocate the memory
	uint8_t* p_src[2] = {
		(uint8_t*)malloc(xres * yres * 2),
		(uint8_t*)malloc(xres * yres * 2)
	};

	std::fill_n((uint16_t*)p_src[0], xres * yres, (uint16_t)(128 | ( 16 << 8)));
	std::fill_n((uint16_t*)p_src[1], xres * yres, (uint16_t)(128 | (235 << 8)));

	// Keep track of times
	auto prev_time = std::chrono::high_resolution_clock::now();

	// Display that we're thinking about thins
	printf("Running benchmark ...\n");

	// Cycle over data
	for (int idx = 0; !exit_loop; idx++) {
		// Lets send out this video frame
		NDIlib_video_frame_v2_t NDI_video_frame;
		NDI_video_frame.xres = xres;
		NDI_video_frame.yres = yres;
		NDI_video_frame.FourCC = NDIlib_FourCC_type_UYVY;
		NDI_video_frame.p_data = p_src[idx & 1];
		NDI_video_frame.line_stride_in_bytes = xres * 2;
		NDI_video_frame.frame_rate_N = framerate_n;
		NDI_video_frame.frame_rate_D = framerate_d;

		// We now submit the frame. 
		NDIlib_send_send_video_async_v2(pNDI_send, &NDI_video_frame);

		// Every 1000 frames we check how long it has taken
		if (idx && ((idx % 1000) == 0)) {
			// Get the time
			const auto this_time = std::chrono::high_resolution_clock::now();

			// Display the frames per second
			printf("%dx%d video encoded at %1.1ffps.\n", xres, yres,
				1000.0f / std::chrono::duration_cast<std::chrono::duration<float>>(this_time - prev_time).count());

			// Cycle the timers
			prev_time = this_time;
		}
	}

	// Sync
	printf("Benchmark stopped.\n");
	NDIlib_send_send_video_async_v2(pNDI_send, NULL);

	// Free the video frame
	free(p_src[0]);
	free(p_src[1]);

	// Destroy the NDI sender
	NDIlib_send_destroy(pNDI_send);

	// Not required, but nice
	NDIlib_destroy();

	// Success
	return 0;
}
