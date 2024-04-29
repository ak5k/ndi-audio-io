#include <csignal>
#include <cstddef>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
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

	// Create an NDI source that is called "My Video and Audio" and is clocked to the video.
	NDIlib_send_create_t NDI_send_create_desc;
	NDI_send_create_desc.p_ndi_name = "My Video and Audio";

	// We create the NDI sender
	NDIlib_send_instance_t pNDI_send = NDIlib_send_create(&NDI_send_create_desc);
	if (!pNDI_send)
		return 0;

	// We are going to create a 1920x1080 interlaced frame at 29.97Hz.
	NDIlib_video_frame_v2_t NDI_video_frame;
	NDI_video_frame.xres = 1920;
	NDI_video_frame.yres = 1080;
	NDI_video_frame.FourCC = NDIlib_FourCC_type_UYVY;
	NDI_video_frame.p_data = (uint8_t*)malloc(1920 * 1080 * 2);
	NDI_video_frame.line_stride_in_bytes = 1920 * 2;

	// Because 48kHz audio actually involves 1601.6 samples per frame, we make a basic sequence that we follow.
	static const int audio_no_samples[] = { 1602, 1601, 1602, 1601, 1602 };

	// Create an audio buffer
	NDIlib_audio_frame_v2_t NDI_audio_frame;
	NDI_audio_frame.sample_rate = 48000;
	NDI_audio_frame.no_channels = 2;
	NDI_audio_frame.no_samples = 1602; // Will be changed on the fly
	NDI_audio_frame.p_data = (float*)malloc(sizeof(float) * 1602 * 2);
	NDI_audio_frame.channel_stride_in_bytes = sizeof(float) * 1602;

	// We will send 1000 frames of video. 
	for (int idx = 0; !exit_loop && idx < 1000; idx++) {
		// Display black ?
		const bool black = (idx % 50) > 10;

		// Because we are clocking to the video it is better to always submit the audio
		// before, although there is very little in it. I'll leave it as an exercises for the
		// reader to work out why.
		NDI_audio_frame.no_samples = audio_no_samples[idx % 5];

		// When not black, insert noise into the buffer. This is a horrible noise, but its just
		// for illustration.
		// Fill in the buffer with silence. It is likely that you would do something much smarter than this.
		for (int ch = 0; ch < 2; ch++) {
			// Get the pointer to the start of this channel
			float* p_ch = (float*)((uint8_t*)NDI_audio_frame.p_data + ch * NDI_audio_frame.channel_stride_in_bytes);

			// Fill it with silence
			for (int sample_no = 0; sample_no < 1602; sample_no++)
				p_ch[sample_no] = ((float)rand() / (float)RAND_MAX - 0.5f) * (black ? 0.0f : 2.0f);
		}

		// Submit the audio buffer
		NDIlib_send_send_audio_v2(pNDI_send, &NDI_audio_frame);

		// Every 50 frames display a few frames of while
		std::fill_n((uint16_t*)NDI_video_frame.p_data, 1920 * 1080, black ? (128 | (16 << 8)) : (128 | (235 << 8)));

		// We now submit the frame. Note that this call will be clocked so that we end up submitting 
		// at exactly 29.97fps.
		NDIlib_send_send_video_v2(pNDI_send, &NDI_video_frame);

		// Just display something helpful
		printf("Frame number %d send.\n", idx);
	}

	// Free the video frame
	free((void*)NDI_video_frame.p_data);
	free((void*)NDI_audio_frame.p_data);

	// Destroy the NDI sender
	NDIlib_send_destroy(pNDI_send);

	// Not required, but nice
	NDIlib_destroy();

	// Finished
	return 0;
}
