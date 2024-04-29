#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <Processing.NDI.Lib.h>

#ifdef _WIN32
#ifdef _WIN64
#pragma comment(lib, "Processing.NDI.Lib.x64.lib")
#else // _WIN64
#pragma comment(lib, "Processing.NDI.Lib.x86.lib")
#endif // _WIN64
#endif // _WIN32

int main(int argc, char* argv[])
{
	// Not required, but "correct" (see the SDK documentation).
	if (!NDIlib_initialize())
		return 0;

	// We create the NDI sender
	NDIlib_send_instance_t pNDI_send = NDIlib_send_create();
	if (!pNDI_send)
		return 0;

	// We are going to create a 1920x1080 interlaced frame at 29.97Hz.
	NDIlib_video_frame_v2_t NDI_video_frame;
	NDI_video_frame.xres = 1920;
	NDI_video_frame.yres = 1080;
	NDI_video_frame.FourCC = NDIlib_FourCC_type_BGRX;
	NDI_video_frame.p_data = (uint8_t*)malloc(NDI_video_frame.xres * NDI_video_frame.yres * 4);

	// Run for one minute
	using namespace std::chrono;
	for (const auto start = high_resolution_clock::now(); high_resolution_clock::now() - start < minutes(5);) {
		// Get the current time
		const auto start_send = high_resolution_clock::now();

		// Send 200 frames
		for (int idx = 200; idx; idx--) {
			// Fill in the buffer. It is likely that you would do something much smarter than this.
			memset((void*)NDI_video_frame.p_data, (idx & 1) ? 255 : 0, NDI_video_frame.xres * NDI_video_frame.yres * 4);

			// We now submit the frame. Note that this call will be clocked so that we end up submitting at exactly 29.97fps.
			NDIlib_send_send_video_v2(pNDI_send, &NDI_video_frame);
		}

		// Just display something helpful
		printf("200 frames sent, at %1.2ffps\n", 200.0f / duration_cast<duration<float>>(high_resolution_clock::now() - start_send).count());
	}

	// Free the video frame
	free(NDI_video_frame.p_data);

	// Destroy the NDI sender
	NDIlib_send_destroy(pNDI_send);

	// Not required, but nice
	NDIlib_destroy();

	// Success
	return 0;
}

