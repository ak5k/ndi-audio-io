#include <csignal>
#include <cstddef>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <atomic>

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

	// Create an NDI source that is called "My 16bpp Audio" and is clocked to the audio.
	NDIlib_send_create_t NDI_send_create_desc;
	NDI_send_create_desc.p_ndi_name = "My 16bpp Audio";
	NDI_send_create_desc.clock_audio = true;

	// We create the NDI finder
	NDIlib_send_instance_t pNDI_send = NDIlib_send_create(&NDI_send_create_desc);
	if (!pNDI_send)
		return 0;

	// We are going to send 1920 audio samples at a time
	NDIlib_audio_frame_interleaved_16s_t NDI_audio_frame;
	NDI_audio_frame.sample_rate = 48000;
	NDI_audio_frame.no_channels = 2;
	NDI_audio_frame.no_samples = 1920;
	NDI_audio_frame.p_data = (short*)malloc(1920 * 2 * sizeof(short));

	// We will send 1000 frames of video. 
	for (int idx = 0; !exit_loop && idx < 1000; idx++) {
		// Fill in the buffer with silence. It is likely that you would do something much smarter than this.
		memset(NDI_audio_frame.p_data, 0, NDI_audio_frame.no_samples * NDI_audio_frame.no_channels * sizeof(short));

		// We now submit the frame. Note that this call will be clocked so that we end up submitting 
		// at exactly 48kHz
		NDIlib_util_send_send_audio_interleaved_16s(pNDI_send, &NDI_audio_frame);

		// Just display something helpful
		printf("Frame number %d sent.\n", idx);
	}

	// Free the video frame
	free(NDI_audio_frame.p_data);

	// Destroy the NDI finder
	NDIlib_send_destroy(pNDI_send);

	// Not required, but nice
	NDIlib_destroy();

	// Success
	return 0;
}
