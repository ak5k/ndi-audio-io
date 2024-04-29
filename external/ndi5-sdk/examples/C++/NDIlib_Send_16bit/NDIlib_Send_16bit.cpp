#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
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

	// We are going to create a 1920x1080 frame in V210 (10 bit packed)
	NDIlib_video_frame_v2_t NDI_video_frame_16bit;
	NDI_video_frame_16bit.xres = 1920;
	NDI_video_frame_16bit.yres = 1080;
	NDI_video_frame_16bit.FourCC = NDIlib_FourCC_video_type_P216;
	NDI_video_frame_16bit.line_stride_in_bytes = NDI_video_frame_16bit.xres * sizeof(uint16_t);
	NDI_video_frame_16bit.p_data = (uint8_t*)malloc(NDI_video_frame_16bit.line_stride_in_bytes * 2 * NDI_video_frame_16bit.yres);

	// Run for one minute
	int frame_no = 0;

	using namespace std::chrono;
	for (const auto start = high_resolution_clock::now(); high_resolution_clock::now() - start < minutes(5); frame_no++) {
		// Get the current time
		const auto start_send = high_resolution_clock::now();

		// Cycle over the lines of the source
		const int white_line_y = frame_no % NDI_video_frame_16bit.yres;
		for (int y = 0; y < NDI_video_frame_16bit.yres; y++) {
			// Get the line pointer
			uint16_t* p_line_y = (uint16_t*)(NDI_video_frame_16bit.p_data + y * NDI_video_frame_16bit.line_stride_in_bytes);
			uint16_t* p_line_cbcr = (uint16_t*)((uint8_t*)p_line_y + NDI_video_frame_16bit.yres * NDI_video_frame_16bit.line_stride_in_bytes);

			// The Luminance line is based on brightness
			std::fill_n((uint16_t*)p_line_y, NDI_video_frame_16bit.xres, white_line_y == y ? 60160 : 4096);

			// The chroma is all grey
			std::fill_n((uint16_t*)p_line_cbcr, NDI_video_frame_16bit.xres, 32768);
		}

		// We now submit the frame. Note that this call will be clocked so that we end up submitting at exactly 29.97fps.
		NDIlib_send_send_video_v2(pNDI_send, &NDI_video_frame_16bit);
	}

	// Free the video frame
	free(NDI_video_frame_16bit.p_data);

	// Destroy the NDI sender
	NDIlib_send_destroy(pNDI_send);

	// Not required, but nice
	NDIlib_destroy();

	// Success
	return 0;
}
