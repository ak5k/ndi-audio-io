#include <csignal>
#include <cstddef>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <atomic>
#include <Processing.NDI.Lib.h>

#ifdef _WIN32
#ifdef _WIN64
#pragma comment(lib, "Processing.NDI.Lib.x64.lib")
#else // _WIN64
#pragma comment(lib, "Processing.NDI.Lib.x86.lib")
#endif // _WIN64
#endif


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

	// Create an NDI source that is called "My Video" and is clocked to the video.
	NDIlib_send_create_t NDI_send_create_desc;
	NDI_send_create_desc.p_ndi_name = "My Video";

	// We create the NDI sender
	NDIlib_send_instance_t pNDI_send = NDIlib_send_create(&NDI_send_create_desc);
	if (!pNDI_send)
		return 0;

	// We are going to create a 1920x1080 interlaced frame at 29.97Hz.
	NDIlib_video_frame_v2_t NDI_video_frame;
	NDI_video_frame.xres = 1920;
	NDI_video_frame.yres = 1080;
	NDI_video_frame.FourCC = NDIlib_FourCC_type_BGRA;
	NDI_video_frame.line_stride_in_bytes = 1920 * 4;

	// We are going to need two frame-buffers because one will typically be in flight (being used by NDI send)
	// while we are filling in the other at the same time.
	void* p_frame_buffers[2] = {
		malloc(1920 * 1080 * 4),
		malloc(1920 * 1080 * 4)
	};

	// We will send 1000 frames of video. 
	for (int idx = 0; idx < 1000; idx++) {
		// Fill in the buffer. Note that we alternate between buffers because we are going to have one buffer processing
		// being filled in while the second is "in flight" and being processed by the API.
		memset(p_frame_buffers[idx & 1], (idx & 1) ? 255 : 0, 1920 * 1080 * 4);

		// We now submit the frame asynchronously. This means that this call will return immediately and the 
		// API will "own" the memory location until there is a synchronizing event. A synchronizing event is 
		// one of : NDIlib_send_send_video_async, NDIlib_send_send_video, NDIlib_send_destroy
		NDI_video_frame.p_data = (uint8_t*)p_frame_buffers[idx & 1];
		NDIlib_send_send_video_async_v2(pNDI_send, &NDI_video_frame);
	}

	// Because one buffer is in flight we need to make sure that there is no chance that we might free it before
	// NDI is done with it. You can ensure this either by sending another frame, or just by sending a frame with
	// a NULL pointer.
	NDIlib_send_send_video_async_v2(pNDI_send, NULL);

	// Free the video frame
	free(p_frame_buffers[0]);
	free(p_frame_buffers[1]);

	// Destroy the NDI sender
	NDIlib_send_destroy(pNDI_send);

	// Not required, but nice
	NDIlib_destroy();

	// Success
	return 0;
}

