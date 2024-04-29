#include <cstdio>
#include <chrono>
#include <thread>
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
	NDIlib_recv_instance_t pNDI_recv = NDIlib_recv_create_v3();
	if (!pNDI_recv)
		return 0;

	// Connect to our sources
	NDIlib_recv_connect(pNDI_recv, p_sources + 0);

	// We are now going to use a frame-synchronizer to ensure that the audio is dynamically
	// resampled and time-based con
	NDIlib_framesync_instance_t pNDI_framesync = NDIlib_framesync_create(pNDI_recv);

	// Destroy the NDI finder. We needed to have access to the pointers to p_sources[0]
	NDIlib_find_destroy(pNDI_find);

	// We create the NDI sender
	NDIlib_send_create_t create_params;
	create_params.clock_video = true;
	create_params.clock_audio = false;
	NDIlib_send_instance_t pNDI_send = NDIlib_send_create(&create_params);

	// The current time in 480kHz intervals which allows us to accurately know exactly how many audio samples are needed.
	// This divides into all common video frame-rates.
	uint64_t current_time = 0;

	// Run for one minute
	using namespace std::chrono;
	for (const auto start = high_resolution_clock::now(); high_resolution_clock::now() - start < minutes(5);) {
		// Using a frame-sync we can always get data which is the magic and it will adapt 
		// to the frame-rate that it is being called with.
		NDIlib_video_frame_v2_t video_frame;
		NDIlib_framesync_capture_video(pNDI_framesync, &video_frame);

		// If we got data
		if (video_frame.p_data) {
			// Display the video frame, async
			NDIlib_send_send_video_v2(pNDI_send, &video_frame);

			// This frame started at this time
			const uint64_t frame_start = current_time;
			current_time += (video_frame.frame_rate_D * 480000LL) / video_frame.frame_rate_N;

			// Free the frame
			NDIlib_framesync_free_video(pNDI_framesync, &video_frame);

			// The number of audio samples needed
			// Note that this is basically 48kHz / 480kHz
			const uint64_t no_audio_samples = (current_time + 5) / 10 - (frame_start + 5) / 10;

			// Get audio samples
			NDIlib_audio_frame_v2_t audio_frame;
			NDIlib_framesync_capture_audio(pNDI_framesync, &audio_frame, 48000, 4, (int)no_audio_samples);

			// Send the audio
			NDIlib_send_send_audio_v2(pNDI_send, &audio_frame);

			// Release the video. You could keep the frame if you want and release it later.
			NDIlib_framesync_free_audio(pNDI_framesync, &audio_frame);
		} else {
			// This is our clock. We are going to run at 30Hz and the frame-sync is smart enough to
			// best adapt the video and audio to match that.
			std::this_thread::sleep_for(milliseconds(33));
		}
	}

	// Free the frame-sync
	NDIlib_framesync_destroy(pNDI_framesync);

	// Destroy the receiver
	NDIlib_recv_destroy(pNDI_recv);

	// Not required, but nice
	NDIlib_destroy();

	// Finished
	return 0;
}
