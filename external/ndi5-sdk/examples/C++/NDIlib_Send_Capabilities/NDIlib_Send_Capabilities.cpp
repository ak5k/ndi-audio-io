#include <csignal>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <atomic>
#include <chrono>
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

	// This will provide a web control on the current IP address under the name http://URL/MyControl/
	// You should have a web server running locally that can serve these files and allow your application
	// to be configured correctly. 
	// 
	// To test : Run NDI Video Monitor (from NDI Tools 2.5 or later). When viewing this source it will
	//           have a gadget in the lower right of the display. If you click on this it will take you
	//           to the URL defined below.
	//
	NDIlib_metadata_frame_t NDI_capabilities;
	NDI_capabilities.p_data = "<ndi_capabilities web_control=\"http://%IP%//MyControl\"/>";
	NDIlib_send_add_connection_metadata(pNDI_send, &NDI_capabilities);

	// We are going to create a 1920x1080 progressive frame at 29.97 Hz.
	NDIlib_video_frame_v2_t NDI_video_frame;
	NDI_video_frame.xres = 1920;
	NDI_video_frame.yres = 1080;
	NDI_video_frame.FourCC = NDIlib_FourCC_type_BGRA;
	NDI_video_frame.p_data = (uint8_t*)malloc(NDI_video_frame.xres * NDI_video_frame.yres * 4);

	// We will send 1000 frames of video. 
	while (!exit_loop) {
		// Get the current time
		const auto start_time = std::chrono::high_resolution_clock::now();

		// Send 200 frames
		for (int idx = 0; !exit_loop && idx < 200; idx++) {
			// Fill in the buffer. It is likely that you would do something much smarter than this.
			::memset((void*)NDI_video_frame.p_data, (idx & 1) ? 255 : 0, NDI_video_frame.xres * NDI_video_frame.yres * 4);

			// We now submit the frame. Note that this call will be clocked so that we end up submitting at exactly 29.97fps.
			NDIlib_send_send_video_v2(pNDI_send, &NDI_video_frame);
		}

		// Get the end time
		const auto end_time = std::chrono::high_resolution_clock::now();

		// Just display something helpful
		printf("200 frames sent, average fps=%1.2f\n", 200.0f / std::chrono::duration_cast<std::chrono::duration<float>>(end_time - start_time).count());
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

