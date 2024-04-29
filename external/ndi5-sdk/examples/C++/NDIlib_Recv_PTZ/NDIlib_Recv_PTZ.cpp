#include <csignal>
#include <cstddef>
#include <cstdio>
#include <atomic>
#include <chrono>
#include <string>

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

	// We first need to look for a source on the network
	const NDIlib_find_create_t NDI_find_create_desc; /* Use defaults */
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
	NDI_recv_create_desc.p_ndi_recv_name = "Example PTZ Receiver";

	NDIlib_recv_instance_t pNDI_recv = NDIlib_recv_create_v3(&NDI_recv_create_desc);
	if (!pNDI_recv)
		return 0;

	// Destroy the NDI finder. We needed to have access to the pointers to p_sources[0]
	NDIlib_find_destroy(pNDI_find);

	// Run for one minute
	const auto start = std::chrono::high_resolution_clock::now();
	while (!exit_loop && std::chrono::high_resolution_clock::now() - start < std::chrono::seconds(30)) {
		// Receive something
		switch (NDIlib_recv_capture_v2(pNDI_recv, NULL, NULL, NULL, 1000)) {
			// There is a status change on the receiver (e.g. new web interface)
			case NDIlib_frame_type_status_change:
			{
				// Get the Web UR
				if (NDIlib_recv_ptz_is_supported(pNDI_recv)) {
					// Display the details
					printf("This source supports PTZ functionality. Moving to preset #3.\n");

					// Move it to preset number  as quickly as it can go !
					NDIlib_recv_ptz_recall_preset(pNDI_recv, 3, 1.0);
				}

				break;
			}

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

