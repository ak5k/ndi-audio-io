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

int clamp(float x)
{
	if (x < 0.0f)
		return 0;

	if (x > 1.0f)
		return 255;

	return (int)(x * 255.0f);
}

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
	const int xres = 3840;
	const int yres = 2160;
	const int framerate_n = 60000;
	const int framerate_d = 1001;
	const int scroll_dist = 4;

	// Allocate the memory
	uint8_t* p_src = (uint8_t*)malloc(xres * yres * scroll_dist * 2);
	for (int y = 0; y < yres * scroll_dist; y++) {
		// Get the line
		uint8_t* p_src_line = p_src + y * xres * 2;
		for (int x = 0; x < xres; x += 2, p_src_line += 4) {
			// Generate some patterns of some kind
			const float fy   = (float)y / (float)yres;
			const float fx_0 = (float)(x + 0) / (float)xres;
			const float fx_1 = (float)(x + 1) / (float)xres;

			// Get the color in RGB
			const int r0 = clamp(cos(fx_0 *  9.0f + fy *  9.5f) * 0.5f + 0.5f);
			const int g0 = clamp(cos(fx_0 * 12.0f + fy * 40.5f) * 0.5f + 0.5f);
			const int b0 = clamp(cos(fx_0 * 23.0f + fy * 15.5f) * 0.5f + 0.5f);

			const int r1 = clamp(cos(fx_1 *  9.0f + fy *  9.5f) * 0.5f + 0.5f);
			const int g1 = clamp(cos(fx_1 * 12.0f + fy * 40.5f) * 0.5f + 0.5f);
			const int b1 = clamp(cos(fx_1 * 23.0f + fy * 15.5f) * 0.5f + 0.5f);

			// Color convert the pixels using integer
			p_src_line[0] = std::max(0, std::min(255, ((112 * b0 -  87 * g0 -  26 * r0) >> 8) + 128));
			p_src_line[1] = std::max(0, std::min(255, (( 16 * b0 + 157 * g0 +  47 * r0) >> 8) +  16));
			p_src_line[2] = std::max(0, std::min(255, ((112 * r1 -  10 * b1 - 102 * g1) >> 8) + 128));
			p_src_line[3] = std::max(0, std::min(255, (( 16 * b1 + 157 * g1 +  47 * r1) >> 8) +  16));
		}
	}

	// Keep track of times
	auto prev_time = std::chrono::high_resolution_clock::now();

	// Display that we're thinking about thins
	printf("Running benchmark ...\n");

	// Cycle over data
	for (int idx = 0; !exit_loop; idx++) {
		// We are going to create a 1920x1080 interlaced frame at 29.97Hz.
		NDIlib_video_frame_v2_t NDI_video_frame;
		NDI_video_frame.xres = xres;
		NDI_video_frame.yres = yres;
		NDI_video_frame.FourCC = NDIlib_FourCC_type_UYVY;
		NDI_video_frame.p_data = p_src + xres * 2 * (idx % (yres * (scroll_dist - 1)));
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
			printf("%dx%d video encoded at %1.1ffps.\n", xres, yres, 1000.0f / std::chrono::duration_cast<std::chrono::duration<float>>(this_time - prev_time).count());

			// Cycle the timers
			prev_time = this_time;
		}
	}

	// Sync
	printf("Benchmark stopped.\n");
	NDIlib_send_send_video_async_v2(pNDI_send, NULL);

	// Free the video frame
	free(p_src);

	// Destroy the NDI sender
	NDIlib_send_destroy(pNDI_send);

	// Not required, but nice
	NDIlib_destroy();

	// Success
	return 0;
}
