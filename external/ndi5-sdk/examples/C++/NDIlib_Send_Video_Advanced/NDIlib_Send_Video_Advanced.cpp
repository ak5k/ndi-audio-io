#include <csignal>
#include <cstddef>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <atomic>

#ifdef _WIN32
#include <windows.h>

#define strncasecmp _strnicmp

#ifdef _WIN64
#pragma comment(lib, "Processing.NDI.Lib.x64.lib")
#else // _WIN64
#pragma comment(lib, "Processing.NDI.Lib.x86.lib")
#endif // _WIN64
#else
#include <strings.h>
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

	// Create an NDI source that is called "My Video" and is clocked to the video.
	NDIlib_send_create_t NDI_send_create_desc;
	NDI_send_create_desc.p_ndi_name = "My Video";

	// We create the NDI sender
	NDIlib_send_instance_t pNDI_send = NDIlib_send_create(&NDI_send_create_desc);
	if (!pNDI_send)
		return 0;

	// Provide a meta-data registration that allows people to know what we are. Note that this is optional.
	// Note that it is possible for senders to also register their preferred video formats.
	NDIlib_metadata_frame_t NDI_connection_type;
	NDI_connection_type.p_data = "<ndi_product long_name=\"NDILib Send Example.\" "
		"             short_name=\"NDILib Send\" "
		"             manufacturer=\"CoolCo, inc.\" "
		"             version=\"1.000.000\" "
		"             session=\"default\" "
		"             model_name=\"S1\" "
		"             serial=\"ABCDEFG\"/>";
	NDIlib_send_add_connection_metadata(pNDI_send, &NDI_connection_type);

	// We are going to create a 1920x1080 interlaced frame at 59.94Hz.
	NDIlib_video_frame_v2_t NDI_video_frame;
	NDI_video_frame.xres = 1920;
	NDI_video_frame.yres = 1080;
	NDI_video_frame.FourCC = NDIlib_FourCC_type_BGRA;
	NDI_video_frame.p_data = (uint8_t*)malloc(NDI_video_frame.xres * NDI_video_frame.yres * 4);
	NDI_video_frame.line_stride_in_bytes = 1920 * 4;

	// We will send 1000 frames of video. 
	for (int idx = 0; !exit_loop; idx++) {
		// We do not use any resources until we are actually connected.
		if (!NDIlib_send_get_no_connections(pNDI_send, 10000)) {
			// Display status
			printf("No current connections, so no rendering needed (%d).\n", idx);
		} else {
			// Have we received any meta-data
			NDIlib_metadata_frame_t metadata_desc;
			if (NDIlib_send_capture(pNDI_send, &metadata_desc, 0)) {
				// For example, this might be a connection meta-data string that might include information
				// about preferred video formats. A full XML parser should be used here, this code is for
				// illustration purposes only
				if (strncasecmp(metadata_desc.p_data, "<ndi_format", 11)) {
					// Setup the preferred video format.
				}

				// Display that we got meta-data
				printf("Received meta-data : %s\n", metadata_desc.p_data);

				// We must free the data here
				NDIlib_send_free_metadata(pNDI_send, &metadata_desc);
			}

			// Get the tally state of this source (we poll it),
			NDIlib_tally_t NDI_tally;
			NDIlib_send_get_tally(pNDI_send, &NDI_tally, 0);

			// Fill in the buffer. It is likely that you would do something much smarter than this.
			for (int y = 0; y < NDI_video_frame.yres; y++) {
				// The frame data
				uint8_t* p_image = (uint8_t*)NDI_video_frame.p_data + NDI_video_frame.line_stride_in_bytes * y;

				// The index start for this line
				int line_idx = y + idx;

				// Cycle over the line
				for (int x = 0; x < NDI_video_frame.xres; x++, p_image += 4, line_idx++) {
					// Slight transparent blue
					p_image[0] = 255;
					p_image[1] = 128;
					p_image[2] = 128;
					p_image[3] = (line_idx & 16) ? 255 : 128;
				}
			}

			// We now submit the frame. Note that this call will be clocked so that we end up submitting at exactly 59.94fps
			NDIlib_send_send_video_v2(pNDI_send, &NDI_video_frame);

			// Just display something helpful
			if ((idx % 100) == 0)
				printf("Frame number %d sent. %s%s\n", 1 + idx, NDI_tally.on_program ? "PGM " : "", NDI_tally.on_preview ? "PVW " : "");
		}
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
