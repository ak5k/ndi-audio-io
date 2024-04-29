#include <cstdio>
#include <chrono>
#include <cassert>
#include <Processing.NDI.Lib.h>

#include "LodePNG/lodepng.h"

#ifdef _WIN32
#ifdef _WIN64
#pragma comment(lib, "Processing.NDI.Lib.x64.lib")
#else // _WIN64
#pragma comment(lib, "Processing.NDI.Lib.x86.lib")
#endif // _WIN64
#else
#include "LodePNG/lodepng.cpp"
#endif // _WIN32

int main(int argc, char* argv[])
{
	// Not required, but "correct" (see the SDK documentation).
	if (!NDIlib_initialize())
		return 0;

	// Create a finder
	NDIlib_find_instance_t pNDI_find = NDIlib_find_create_v2();
	if (!pNDI_find)
		return 0;

	// Wait until there is one source
	uint32_t no_sources = 0;
	const NDIlib_source_t* p_sources = NULL;
	while (!no_sources) {
		// Wait until the sources on the network have changed
		printf("Looking for sources ...\n");
		NDIlib_find_wait_for_sources(pNDI_find, 1000/* One second */);
		p_sources = NDIlib_find_get_current_sources(pNDI_find, &no_sources);
	}

	// We now have at least one source, so we create a receiver to look at it.
	NDIlib_recv_create_v3_t recv_desc;
	recv_desc.color_format = NDIlib_recv_color_format_RGBX_RGBA;
	NDIlib_recv_instance_t pNDI_recv = NDIlib_recv_create_v3(&recv_desc);
	if (!pNDI_recv)
		return 0;

	// Connect to our sources
	NDIlib_recv_connect(pNDI_recv, p_sources + 0);

	// Destroy the NDI finder. We needed to have access to the pointers to p_sources[0]
	NDIlib_find_destroy(pNDI_find);

	// We wait for up to a minute to receive a video frame
	NDIlib_video_frame_v2_t video_frame;
	if (NDIlib_recv_capture_v2(pNDI_recv, &video_frame, nullptr, nullptr, 60000) == NDIlib_frame_type_video) {
		// If we have stride that does not match the width then we'd need to make a copy, but this is just test
		// code so this will have to do for now in this example.
		assert(video_frame.line_stride_in_bytes == video_frame.xres * 4);
		lodepng_encode_file("CoolNDIImage.png", video_frame.p_data, video_frame.xres, video_frame.yres, LCT_RGBA, 8);

		// Free the data 
		NDIlib_recv_free_video_v2(pNDI_recv, &video_frame);
	}

	// Destroy the receiver
	NDIlib_recv_destroy(pNDI_recv);

	// Not required, but nice
	NDIlib_destroy();

	// Finished
	return 0;
}
