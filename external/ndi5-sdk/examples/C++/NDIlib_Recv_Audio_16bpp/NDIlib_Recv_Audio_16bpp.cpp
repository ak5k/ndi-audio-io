#include <csignal>
#include <cstddef>
#include <cstdio>
#include <atomic>
#include <chrono>

#ifdef _WIN32
#include <windows.h>

#ifdef _WIN64
#pragma comment(lib, "Processing.NDI.Lib.x64.lib")
#else // _WIN64
#pragma comment(lib, "Processing.NDI.Lib.x86.lib")
#endif // _WIN64

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

	// Create a finder
	const NDIlib_find_create_t NDI_find_create_desc; /* Default settings */
	NDIlib_find_instance_t pNDI_find = NDIlib_find_create_v2(&NDI_find_create_desc);
	if (!pNDI_find)
		return 0;

	// We wait until there is at least one source on the network
	uint32_t no_sources = 0;
	const NDIlib_source_t* p_sources = NULL;
	while (!exit_loop && !no_sources) {
		// Wait until the sources on the network have changed
		NDIlib_find_wait_for_sources(pNDI_find, 1000);
		p_sources = NDIlib_find_get_current_sources(pNDI_find, &no_sources);
	}

	// We need at least one source
	if (!p_sources)
		return 0;

	// We now have at least one source, so we create a receiver to look at it.
	// We tell it that we prefer YCbCr video since it is more efficient for us. If the source has an alpha channel
	// it will still be provided in BGRA
	NDIlib_recv_create_v3_t NDI_recv_create_desc;
	NDI_recv_create_desc.source_to_connect_to = p_sources[0];
	NDI_recv_create_desc.p_ndi_recv_name = "Example Audio Converter Receiver";

	// Create the receiver
	NDIlib_recv_instance_t pNDI_recv = NDIlib_recv_create_v3(&NDI_recv_create_desc);
	if (!pNDI_recv) {
		NDIlib_find_destroy(pNDI_find);
		return 0;
	}

	// Destroy the NDI finder. We needed to have access to the pointers to p_sources[0]
	NDIlib_find_destroy(pNDI_find);

	// Run for one minute
	const auto start = std::chrono::high_resolution_clock::now();
	while (!exit_loop && std::chrono::high_resolution_clock::now() - start < std::chrono::minutes(1)) {
		// The descriptors
		NDIlib_video_frame_v2_t video_frame;
		NDIlib_audio_frame_v2_t audio_frame;
		NDIlib_metadata_frame_t metadata_frame;

		switch (NDIlib_recv_capture_v2(pNDI_recv, &video_frame, &audio_frame, &metadata_frame, 1000)) {
			// No data
			case NDIlib_frame_type_none:
				printf("No data received.\n");
				break;

				// Video data
			case NDIlib_frame_type_video:
				printf("Video data received (%dx%d).\n", video_frame.xres, video_frame.yres);
				NDIlib_recv_free_video_v2(pNDI_recv, &video_frame);
				break;

				// Audio data
			case NDIlib_frame_type_audio:
			{
				printf("Audio data received (%d samples).\n", audio_frame.no_samples);

				// Allocate enough space for 16bpp interleaved buffer
				NDIlib_audio_frame_interleaved_16s_t audio_frame_16bpp_interleaved;
				audio_frame_16bpp_interleaved.reference_level = 20;	// We are going to have 20dB of headroom
				audio_frame_16bpp_interleaved.p_data = new short[audio_frame.no_samples * audio_frame.no_channels];

				// Convert it
				NDIlib_util_audio_to_interleaved_16s_v2(&audio_frame, &audio_frame_16bpp_interleaved);

				// Free the original buffer
				NDIlib_recv_free_audio_v2(pNDI_recv, &audio_frame);

				// Feel free to do something with the interleaved audio data here

				// Free the interleaved audio data
				delete[] audio_frame_16bpp_interleaved.p_data;

				break;
			}

			// Meta data
			case NDIlib_frame_type_metadata:
				printf("Meta data received.\n");
				NDIlib_recv_free_metadata(pNDI_recv, &metadata_frame);
				break;

				// There is a status change on the receiver (e.g. new web interface)
			case NDIlib_frame_type_status_change:
				printf("Receiver connection status changed.\n");
				break;

				// Everything else
			default:
				break;
		}
	}

	// Destroy the receiver
	NDIlib_recv_destroy(pNDI_recv);

	// Not required, but nice
	NDIlib_destroy();

	// Finished
	return 0;
}

