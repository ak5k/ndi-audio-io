#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>
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

	// Create an NDI source that is called "My Audio" and is clocked to the audio.
	NDIlib_send_create_t NDI_send_create_desc;
	NDI_send_create_desc.p_ndi_name = "My Audio";
	NDI_send_create_desc.clock_audio = true;

	// We create the NDI finder
	NDIlib_send_instance_t pNDI_send = NDIlib_send_create(&NDI_send_create_desc);
	if (!pNDI_send)
		return 0;

	// We are going to send 1920 audio samples at a time
	NDIlib_audio_frame_v2_t NDI_audio_frame;
	NDI_audio_frame.sample_rate = 48000;
	NDI_audio_frame.no_channels = 4;
	NDI_audio_frame.no_samples = 1920;
	NDI_audio_frame.p_data = (float*)malloc(NDI_audio_frame.no_samples * NDI_audio_frame.no_channels * sizeof(float));
	NDI_audio_frame.channel_stride_in_bytes = NDI_audio_frame.no_samples * sizeof(float);

	// We will send 1000 frames of video. 
	for (int idx = 0; !exit_loop && idx < 1000; idx++) {
		// Fill in the buffer with silence. It is likely that you would do something much smarter than this.
		for (int ch = 0; ch < NDI_audio_frame.no_channels; ch++) {
			// Get the pointer to the start of this channel
			float* p_ch = (float*)((uint8_t*)NDI_audio_frame.p_data + ch * NDI_audio_frame.channel_stride_in_bytes);

			// Fill it with silence
			for (int sample_no = 0; sample_no < NDI_audio_frame.no_samples; sample_no++)
				p_ch[sample_no] = 0.0f;
		}

		// We now submit the frame. Note that this call will be clocked so that we end up submitting 
		// at exactly 48kHz
		NDIlib_send_send_audio_v2(pNDI_send, &NDI_audio_frame);

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

