#include <cinttypes>
#include <cmath>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#define strcasecmp  _stricmp

#ifdef _WIN64
#pragma comment(lib, "Processing.NDI.Lib.x64.lib")
#else // _WIN64
#pragma comment(lib, "Processing.NDI.Lib.x86.lib")
#endif // _WIN64
#else
#include <strings.h>
#include <unistd.h>
#endif // _WIN32

#include <Processing.NDI.Lib.h>

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

static std::atomic<bool>       g_exit_process(false);
static std::mutex		       g_exit_lock;
static bool				       g_exit_threads = false;
static std::condition_variable g_exit_cv;

// Helper to convert from dB to a ratio
double dB_to_ratio(double dB)
{
	return pow(10.0, dB / 20.0);
}

bool process_input(const std::string& audio_device_name, const std::string& audio_ndi_name, float gain_in_dB)
{
	// Initialize a miniaudio context.
	ma_context context;
	if (ma_context_init(nullptr, 0, nullptr, &context) != MA_SUCCESS) {
		puts("ERROR : Unable to initialize audio devices.");
		return false;
	}

	// Let's enumerate the capture devices.
	ma_device_info* p_capture_devices;
	ma_uint32 num_capture_devices;
	if (ma_context_get_devices(&context, nullptr, nullptr, &p_capture_devices, &num_capture_devices) != MA_SUCCESS) {
		puts("ERROR : Unable to enumerate audio devices.");
	}

	ma_uint32 device_num = (ma_uint32)-1;

	// Look for the capture device by name.
	for (ma_uint32 i = 0; i != num_capture_devices; i++) {
		if (strcasecmp(p_capture_devices[i].name, audio_device_name.c_str()) == 0) {
			device_num = i;
			break;
		}
	}

	// The device name didn't match anything. Perhaps it's a numeric index instead.
	if (device_num == (ma_uint32)-1) {
		int num = atoi(audio_device_name.c_str());
		if (num != 0)
			device_num = num - 1;
	}

	// There have been no matches so far. We'll fallback to the default device instead.
	if (device_num == (ma_uint32)-1) {
		for (ma_uint32 i = 0; i != num_capture_devices; i++) {
			if (p_capture_devices[i].isDefault) {
				device_num = i;
				break;
			}
		}
	}

	// Audio sending class.
	struct audio_cature_t {
		audio_cature_t(const char* p_audio_name, float gain_in_dB)
			: m_gain_in_dB(gain_in_dB)
		{
			// Create the NDI source.
			NDIlib_send_create_t send_create(p_audio_name);
			m_p_ndi_send = NDIlib_send_create(&send_create);
		}

		~audio_cature_t(void)
		{
			// Stop the NDI sender.
			NDIlib_send_destroy(m_p_ndi_send);
			m_p_ndi_send = nullptr;
		}

		void callback_proc(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
		{
			// Build up a source audio level.
			NDIlib_audio_frame_interleaved_32f_t src_audio_frame;
			src_audio_frame.no_channels = pDevice->capture.channels;
			src_audio_frame.sample_rate = pDevice->capture.internalSampleRate;
			src_audio_frame.no_samples = frameCount;
			src_audio_frame.p_data = (float*)pInput;

			// Convert this to an output buffer.
			NDIlib_audio_frame_v2_t	dst_audio_frame;
			m_workspace.resize(src_audio_frame.no_channels * src_audio_frame.no_samples);
			dst_audio_frame.p_data = (float*)m_workspace.data();
			dst_audio_frame.channel_stride_in_bytes = sizeof(float) * src_audio_frame.no_samples;

			// Convert the audio.
			NDIlib_util_audio_from_interleaved_32f_v2(&src_audio_frame, &dst_audio_frame);

			// Scale the audio correctly.
			for (size_t i = 0; i != m_workspace.size(); i++)
				m_workspace[i] *= m_gain_in_dB;

			// Send the audio please!
			NDIlib_send_send_audio_v2(m_p_ndi_send, &dst_audio_frame);
		}

		// The NDI audio sender.
		NDIlib_send_instance_t m_p_ndi_send = nullptr;

		// The audio gain.
		float m_gain_in_dB;

		// Workspace for planar channels.
		std::vector<float> m_workspace;
	} audio_capture(audio_ndi_name.c_str(), (float)dB_to_ratio(gain_in_dB));

	// I do not know how this happened.
	if (device_num != (ma_uint32)-1) {
		// We are now going to initialize the device
		ma_device_config config = ma_device_config_init(ma_device_type_capture);

		// Setup the device to be used.
		config.capture.pDeviceID = &p_capture_devices[device_num].id;
		config.capture.format = ma_format_f32;
		config.pUserData = &audio_capture;
		config.dataCallback = [](ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
			((audio_cature_t*)pDevice->pUserData)->callback_proc(pDevice, pOutput, pInput, frameCount);
		};

		// Create a device.
		ma_device device;
		if (ma_device_init(&context, &config, &device) == MA_SUCCESS) {
			// The device is sleeping by default so you'll need to start it manually.
			ma_device_start(&device);

			// Wait for exit to be signaled and process audio until then!
			std::unique_lock<std::mutex> exit_lock(g_exit_lock);
			g_exit_cv.wait(exit_lock, [] { return g_exit_threads; });
			exit_lock.unlock();

			// This will stop the device so no need to do that manually.
			ma_device_uninit(&device);
		}
	}

	ma_context_uninit(&context);
	return true;
}

bool process_output(const std::string& audio_device_name, const std::string& audio_ndi_name, float gain_in_dB)
{
	// Initialize a miniaudio context.
	ma_context context;
	if (ma_context_init(nullptr, 0, nullptr, &context) != MA_SUCCESS) {
		puts("ERROR : Unable to initialize audio devices.");
		return false;
	}

	// Let's enumerate the playback devices.
	ma_device_info* p_playback_devices;
	ma_uint32 num_playback_devices;
	if (ma_context_get_devices(&context, &p_playback_devices, &num_playback_devices, nullptr, nullptr) != MA_SUCCESS) {
		puts("ERROR : Unable to enumerate audio devices.");
	}

	ma_uint32 device_num = (ma_uint32)-1;

	// Look for the playback device by name.
	for (ma_uint32 i = 0; i != num_playback_devices; i++) {
		if (strcasecmp(p_playback_devices[i].name, audio_device_name.c_str()) == 0) {
			device_num = i;
			break;
		}
	}

	// The device name didn't match anything. Perhaps it's a numeric index instead.
	if (device_num == (ma_uint32)-1) {
		int num = atoi(audio_device_name.c_str());
		if (num != 0)
			device_num = num - 1;
	}

	// There have been no matches so far. We'll fallback to the default device instead.
	if (device_num == (ma_uint32)-1) {
		for (ma_uint32 i = 0; i != num_playback_devices; i++) {
			if (p_playback_devices[i].isDefault) {
				device_num = i;
				break;
			}
		}
	}

	// Audio receiving class.
	struct audio_playback_t {
		audio_playback_t(const char* p_audio_name, float gain_in_dB)
			: m_gain_in_dB(gain_in_dB)
		{
			// Create the NDI receiver.
			NDIlib_recv_create_v3_t recv_create;
			recv_create.source_to_connect_to.p_ndi_name = p_audio_name;
			recv_create.bandwidth = NDIlib_recv_bandwidth_audio_only;
			m_p_ndi_recv = NDIlib_recv_create_v3(&recv_create);

			// We need a frame-sync.
			m_p_ndi_framesync = NDIlib_framesync_create(m_p_ndi_recv);
		}

		~audio_playback_t(void)
		{
			// Stop the frame-sync.
			NDIlib_framesync_destroy(m_p_ndi_framesync);
			m_p_ndi_framesync = nullptr;

			// Stop the NDI receiver.
			NDIlib_recv_destroy(m_p_ndi_recv);
			m_p_ndi_recv = nullptr;
		}

		void callback_proc(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
		{
			// We get the audio data.
			NDIlib_audio_frame_v2_t src_aud;
			NDIlib_framesync_capture_audio(m_p_ndi_framesync, &src_aud, pDevice->playback.internalSampleRate, pDevice->playback.channels, frameCount);

			// Build up a source audio level.
			NDIlib_audio_frame_interleaved_32f_t dst_aud;
			dst_aud.no_channels = pDevice->playback.channels;
			dst_aud.sample_rate = pDevice->playback.internalSampleRate;
			dst_aud.no_samples = frameCount;
			dst_aud.p_data = (float*)pOutput;

			// Convert the channels from planar to interleaved.
			NDIlib_util_audio_to_interleaved_32f_v2(&src_aud, &dst_aud);

			// Free the original frame.
			NDIlib_framesync_free_audio(m_p_ndi_framesync, &src_aud);

			// Scale the audio correctly.
			for (int i = 0; i < dst_aud.no_samples * dst_aud.no_channels; i++)
				dst_aud.p_data[i] *= m_gain_in_dB;
		}

		// The NDI audio receiver.
		NDIlib_recv_instance_t m_p_ndi_recv = nullptr;

		// The NDI frame-sync.
		NDIlib_framesync_instance_t m_p_ndi_framesync = nullptr;

		// The audio gain.
		float m_gain_in_dB;
	} audio_playback(audio_ndi_name.c_str(), (float)dB_to_ratio(gain_in_dB));

	// I do not know how this happened.
	if (device_num != (ma_uint32)-1) {
		// We are now going to initialize the device.
		ma_device_config config = ma_device_config_init(ma_device_type_playback);

		// Setup the device to be used.
		config.playback.pDeviceID = &p_playback_devices[device_num].id;
		config.playback.format = ma_format_f32;
		config.pUserData = &audio_playback;
		config.dataCallback = [](ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
			((audio_playback_t*)pDevice->pUserData)->callback_proc(pDevice, pOutput, pInput, frameCount);
		};

		// Create a device.
		ma_device device;
		if (ma_device_init(&context, &config, &device) == MA_SUCCESS) {
			// The device is sleeping by default so you'll need to start it manually.
			ma_device_start(&device);

			// Wait for exit to be signaled and process audio until then!
			std::unique_lock<std::mutex> exit_lock(g_exit_lock);
			g_exit_cv.wait(exit_lock, [] { return g_exit_threads; });
			exit_lock.unlock();

			// This will stop the device so no need to do that manually.
			ma_device_uninit(&device);
		}
	}

	ma_context_uninit(&context);
	return true;
}

int main(int argc, char* argv[])
{
	// Catch interrupt so that we can shut down gracefully.
	signal(SIGINT, [](int) { g_exit_process = true; });

	// Display the copyright.
	puts("NDI Free Audio");
	puts("==============\n");

	// The command lines
	std::string input_source, input_name_source = "Free Audio";
	std::string output_source = "default", output_name_source;
	float input_gain_dB = 0.0, output_gain_dB = 0.0;

	// Parse the command line
	for (int i = 1; i < argc; i++) {
		// Get the output source
		if (strcasecmp(argv[i], "-output") == 0) {
			// Get the arguments
			if (++i < argc)
				output_source = argv[i];

			continue;
		}

		// Get the output source
		if (strcasecmp(argv[i], "-output_name") == 0) {
			// Get the arguments
			if (++i < argc)
				output_name_source = argv[i];

			continue;
		}

		// Get the output gain.
		if (strcasecmp(argv[i], "-output_gain") == 0) {
			// Get the argument.
			if (++i < argc)
				output_gain_dB = (float)atof(argv[i]);

			continue;
		}

		// Get the input source.
		if (strcasecmp(argv[i], "-input") == 0) {
			// Get the argument.
			if (++i < argc)
				input_source = argv[i];

			continue;
		}

		// Get the input source.
		if (strcasecmp(argv[i], "-input_name") == 0) {
			// Get the argument.
			if (++i < argc)
				input_name_source = argv[i];

			continue;
		}

		// Get the input gain.
		if (strcasecmp(argv[i], "-input_gain") == 0) {
			// Get the argument.
			if (++i < argc)
				input_gain_dB = (float)atof(argv[i]);

			continue;
		}
	}

	// Start the output thread if needed.
	std::thread output_thread;
	if (!output_source.empty() && !output_name_source.empty()) {
		puts("Starting Audio Output ...");
		output_thread = std::thread(std::bind(process_output, output_source, output_name_source, output_gain_dB));
	}

	// Start the input thread if needed.
	std::thread input_thread;
	if (!input_source.empty() && !input_name_source.empty()) {
		puts("Starting Audio Input ...");
		input_thread = std::thread(std::bind(process_input, input_source, input_name_source, input_gain_dB));
	}

	// Wait for things to finish
	if (output_thread.joinable() || input_thread.joinable()) {
		char progress[] = "\r[.....................................................]";
		const char rotate[] = { "-\\|/" };
		for (size_t i = 0; !g_exit_process; i++) {
			progress[2 + (i % (sizeof(progress) - 4))] = rotate[i % (sizeof(rotate) - 1)];
			fputs(progress, stdout);
			fflush(stdout);
			std::this_thread::sleep_for(std::chrono::milliseconds(250));
			progress[2 + (i % (sizeof(progress) - 4))] = '.';
		}

		fputs("\n\nExiting ...\n", stdout);
		fflush(stdout);

		// Signal the threads to exit.

		std::unique_lock<std::mutex> exit_lock(g_exit_lock);
		g_exit_threads = true;
		exit_lock.unlock();
		g_exit_cv.notify_all();

		// Wait for the threads to exit.

		if (output_thread.joinable())
			output_thread.join();

		if (input_thread.joinable())
			input_thread.join();

		// Finished.
		return 0;
	}

	// List the command line options.
	puts("\nOptions:");
	puts("       -input \"audio device name\"");
	puts("    or -input 1");
	puts("    or -input default");
	puts("    -input_name \"Some Source\"");
	puts("    -input_gain +10dB\n");
	puts("       -output \"audio device name\"");
	puts("    or -output 3");
	puts("    or -output default");
	puts("    -output_name \"Some Network Source\"");
	puts("    -output_gain -15dB\n");

	// Initialize a miniaudio context.
	ma_context context;
	if (ma_context_init(nullptr, 0, nullptr, &context) != MA_SUCCESS) {
		puts("ERROR : Unable to initialize audio devices.");
		return 1;
	}

	// Let's enumerate the devices.
	ma_device_info* p_playback_devices;
	ma_device_info* p_capture_devices;
	ma_uint32 num_playback_devices, num_capture_devices;
	if (ma_context_get_devices(&context, &p_playback_devices, &num_playback_devices, &p_capture_devices, &num_capture_devices) != MA_SUCCESS) {
		puts("ERROR : Unable to enumerate audio devices.");
	}

	// List the input devices.
	puts("\nInput Devices:");
	for (ma_uint32 i = 0; i != num_capture_devices; i++)
		printf("    %" PRIu32 " : %s %s\n", 1 + i, p_capture_devices[i].name, p_capture_devices[i].isDefault ? "[default]" : "");

	// List the output devices.
	puts("\nOutput Devices:");
	for (ma_uint32 i = 0; i != num_playback_devices; i++)
		printf("    %" PRIu32 " : %s %s\n", 1 + i, p_playback_devices[i].name, p_playback_devices[i].isDefault ? "[default]" : "");

	ma_context_uninit(&context);
	return 0;
}
