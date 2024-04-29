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

	// Run for one minute
	using namespace std::chrono;
	for (const auto start = high_resolution_clock::now(); high_resolution_clock::now() - start < minutes(5);) {
		// Using a frame-sync we can always get data which is the magic and it will adapt 
		// to the frame-rate that it is being called with.
		NDIlib_video_frame_v2_t video_frame;
		NDIlib_framesync_capture_video(pNDI_framesync, &video_frame);

		// Display video here. The reason that the frame-sync does not return a frame until it has
		// received the frame (e.g. it could return a black 1920x1080 image p) is that you are likely to
		// want to default to some video standard (NTSC or PAL) and there would be no way to know what
		// your default image should be from an API level.
		if (video_frame.p_data) {
			// You display the video frame.
		}

		// Release the video. You could keep the frame if you want and release it later.
		NDIlib_framesync_free_video(pNDI_framesync, &video_frame);

		// Get audio samples
		NDIlib_audio_frame_v2_t audio_frame;
		NDIlib_framesync_capture_audio(pNDI_framesync, &audio_frame, 48000, 4, 1600);

		// Play audio here. This will always return values and even silence if there
		// is not yet any audio.

		// Release the video. You could keep the frame if you want and release it later.
		NDIlib_framesync_free_audio(pNDI_framesync, &audio_frame);

		// This is our clock. We are going to run at 30Hz and the frame-sync is smart enough to
		// best adapt the video and audio to match that.
		std::this_thread::sleep_for(milliseconds(33));
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
