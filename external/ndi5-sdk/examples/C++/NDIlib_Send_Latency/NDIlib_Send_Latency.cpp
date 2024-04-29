#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <thread>

#ifdef _WIN32
#include <windows.h>

#define strcasecmp _stricmp

#ifdef _WIN64
#pragma comment(lib, "Processing.NDI.Lib.x64.lib")
#else // _WIN64
#pragma comment(lib, "Processing.NDI.Lib.x86.lib")
#endif // _WIN64
#else
#include <strings.h>
#endif

#include <Processing.NDI.Lib.h>

void receive_source(const NDIlib_source_t* pSourceName, const std::chrono::high_resolution_clock::time_point reference_time)
{
	// Open a receiver and connect to the receiver
	NDIlib_recv_instance_t pNDI_recv = ::NDIlib_recv_create_v3();
	::NDIlib_recv_connect(pNDI_recv, pSourceName);

	for (bool exit = false; !exit;) {
		// Receive a frame
		NDIlib_video_frame_v2_t video_frame;
		if (::NDIlib_recv_capture_v2(pNDI_recv, &video_frame, nullptr, nullptr, 250) == NDIlib_frame_type_video) {
			// Get the current time in ns
			const uint64_t time_since_reference = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - reference_time).count();

			// Show we exit ?
			if ((video_frame.p_metadata) && (0 == strcasecmp(video_frame.p_metadata, "<exit/>")))
				exit = true;

			// Free the data
			::NDIlib_recv_free_video_v2(pNDI_recv, &video_frame);

			// Display the time in MS
			const uint64_t latency_in_ms = (time_since_reference - video_frame.timecode + 500) / 1000;

			// Dislplay it
			printf("NDI video latency (with compression, transmission and decompression) = %1.2fms\n", (float)(time_since_reference - video_frame.timecode) / 1000.0f);
		}
	}

	// Destroy 
	::NDIlib_recv_destroy(pNDI_recv);
}

int main(int argc, char* argv[])
{
	// Not required, but "correct" (see the SDK documentation.
	if (!NDIlib_initialize()) {
		// Cannot run NDI. Most likely because the CPU is not sufficient (see SDK documentation).
		// you can check this directly with a call to NDIlib_is_supported_CPU()
		printf("Cannot run NDI.");
		return 0;
	}

	// This is our reference time which we use 
	const std::chrono::high_resolution_clock::time_point reference_time = std::chrono::high_resolution_clock::now();

	// Create an NDI source that is called "My PNG" and is clocked to the video.
	NDIlib_send_create_t NDI_send_create_desc;
	NDI_send_create_desc.clock_video = false;
	NDI_send_create_desc.p_ndi_name = "Latency Check";

	// We create the NDI sender
	NDIlib_send_instance_t pNDI_send = ::NDIlib_send_create(&NDI_send_create_desc);
	if (!pNDI_send)
		return 0;

	// We create a thread that now listens for sources
	std::thread receive_thread(receive_source, ::NDIlib_send_get_source_name(pNDI_send), reference_time);

	// We are going to create a frame
	NDIlib_video_frame_v2_t NDI_video_frame;
	NDI_video_frame.xres = 640;
	NDI_video_frame.yres = 480;
	NDI_video_frame.FourCC = NDIlib_FourCC_type_UYVY;
	NDI_video_frame.line_stride_in_bytes = NDI_video_frame.xres * 2;
	NDI_video_frame.p_data = new uint8_t[NDI_video_frame.line_stride_in_bytes * NDI_video_frame.yres];
	::memset(NDI_video_frame.p_data, 128, NDI_video_frame.line_stride_in_bytes * NDI_video_frame.yres);

	for (int idx = 100; idx; idx--) {
		// Measure the number of nano-seconds since the reference time
		NDI_video_frame.timecode = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - reference_time).count();
		if (idx == 1)
			NDI_video_frame.p_metadata = "<exit/>";

		// We now submit the frame. Note that this call will be clocked so that we end up submitting at exactly 29.97fps.
		::NDIlib_send_send_video_async_v2(pNDI_send, &NDI_video_frame);

		// We do not want to become time limited, in effect this means that we check the latency every 250ms
		std::this_thread::sleep_for(std::chrono::milliseconds(250));
	}

	// Synchronize, just in case
	::NDIlib_send_send_video_async_v2(pNDI_send, nullptr);

	// Destroy the NDI sender
	::NDIlib_send_destroy(pNDI_send);

	// Free the memory
	delete[] NDI_video_frame.p_data;

	// We wait for the receiver to exit
	receive_thread.join();

	// Not required, but nice
	::NDIlib_destroy();

	// Success
	return 0;
}
