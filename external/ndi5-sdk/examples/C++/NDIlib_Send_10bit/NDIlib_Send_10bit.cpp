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

void fill_line_v210(uint8_t* p_line, const int xres, const uint16_t y, const uint16_t u, const uint16_t v)
{
	for (int N = xres; ; N -= 6, p_line += 16) {
		// The first 32bits
		if (N < 1) return;
		*(uint32_t*)(0 + p_line) = (u >> 6) | ((y >> 6) << 10) | ((v >> 6) << 20);

		// The second 32bits
		if (N < 3) return;
		*(uint32_t*)(4 + p_line) = (y >> 6) | ((u >> 6) << 10) | ((y >> 6) << 20);

		// The third 32bits
		if (N < 4) return;
		*(uint32_t*)(8 + p_line) = (v >> 6) | ((y >> 6) << 10) | ((u >> 6) << 20);

		// The third 32bits
		if (N < 6) return;
		*(uint32_t*)(12 + p_line) = (y >> 6) | ((v >> 6) << 10) | ((y >> 6) << 20);
	}
}

int main(int argc, char* argv[])
{
	// Not required, but "correct" (see the SDK documentation).
	if (!NDIlib_initialize())
		return 0;

	// We create the NDI sender
	NDIlib_send_instance_t pNDI_send = NDIlib_send_create();
	if (!pNDI_send)
		return 0;

	// We are going to create a 1920x1080 frame in V210 (10 bit packed)
	NDIlib_video_frame_v2_t NDI_video_frame_10bit;
	NDI_video_frame_10bit.xres = 1920;
	NDI_video_frame_10bit.yres = 1080;
	NDI_video_frame_10bit.FourCC = (NDIlib_FourCC_video_type_e)NDI_LIB_FOURCC('V', '2', '1', '0');

	// The format of V210 is :
	// [10 bits U0] [10 bits Y0] [10 bits V0] [2 bits unused] [10 bits Y1] [10 bits U2] [10 bits Y2] [2 bits unused] etc...
	NDI_video_frame_10bit.line_stride_in_bytes = (NDI_video_frame_10bit.xres * sizeof(uint32_t) * 4 + /* Round up !*/5) / 6;
	NDI_video_frame_10bit.p_data = (uint8_t*)malloc(NDI_video_frame_10bit.line_stride_in_bytes * NDI_video_frame_10bit.yres);

	// We have a PA12 output
	NDIlib_video_frame_v2_t NDI_video_frame_16bit;
	NDI_video_frame_16bit.xres = NDI_video_frame_10bit.xres;
	NDI_video_frame_16bit.yres = NDI_video_frame_10bit.yres;

	NDI_video_frame_16bit.line_stride_in_bytes = NDI_video_frame_16bit.xres * sizeof(uint16_t);
	NDI_video_frame_16bit.p_data = (uint8_t*)malloc(NDI_video_frame_16bit.line_stride_in_bytes * 2 * NDI_video_frame_16bit.yres);

	// Run for one minute
	int frame_no = 0;

	using namespace std::chrono;
	for (const auto start = high_resolution_clock::now(); high_resolution_clock::now() - start < minutes(5); frame_no++) {
		// Get the current time
		const auto start_send = high_resolution_clock::now();

		// Cycle over the lines of the source
		const int white_line_y = frame_no % NDI_video_frame_10bit.yres;
		for (int y = 0; y < NDI_video_frame_10bit.yres; y++) {
			// Get the line pointer
			uint8_t* p_line = NDI_video_frame_10bit.p_data + y * NDI_video_frame_10bit.line_stride_in_bytes;
			fill_line_v210(p_line, NDI_video_frame_10bit.xres, (y == white_line_y) ? 60160 : 4096, 32768, 32768);
		}

		// Convert into the destination
		NDIlib_util_V210_to_P216(&NDI_video_frame_10bit, &NDI_video_frame_16bit);

		// We now submit the frame. Note that this call will be clocked so that we end up submitting at exactly 29.97fps.
		NDIlib_send_send_video_v2(pNDI_send, &NDI_video_frame_16bit);
	}

	// Free the video frame
	free(NDI_video_frame_10bit.p_data);
	free(NDI_video_frame_16bit.p_data);

	// Destroy the NDI sender
	NDIlib_send_destroy(pNDI_send);

	// Not required, but nice
	NDIlib_destroy();

	// Success
	return 0;
}
