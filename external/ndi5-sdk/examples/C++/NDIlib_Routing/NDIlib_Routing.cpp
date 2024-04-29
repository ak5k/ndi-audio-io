#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <atomic>
#include <chrono>
#include <thread>
#include <random>

#ifdef _WIN32
#include <windows.h>

#ifdef _WIN64
#pragma comment(lib, "Processing.NDI.Lib.x64.lib")
#else // _WIN64
#pragma comment(lib, "Processing.NDI.Lib.x86.lib")
#endif // _WIN64

#endif

#include "Processing.NDI.Lib.h"

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

	// We are going to create a routed NDI source
	NDIlib_routing_create_t NDI_send_create_desc;
	NDI_send_create_desc.p_ndi_name = "Routing";

	// We create the NDI finder
	NDIlib_routing_instance_t pNDI_routing = NDIlib_routing_create(&NDI_send_create_desc);
	if (!pNDI_routing)
		return 0;

	// We are going to create an NDI finder that locates sources on the network.
	// excluding ones that are available on this machine itself. It will use the default
	// groups assigned for the current machine. 
	const NDIlib_find_create_t NDI_find_create_desc; /* Use defaults */
	NDIlib_find_instance_t pNDI_find = NDIlib_find_create_v2(&NDI_find_create_desc);
	if (!pNDI_find)
		return 0;

	// We will route 1000 times.  Approximately once every 15 seconds
	for (int idx = 0; !exit_loop && idx < 1000; idx++) {
		// Get the network sources
		uint32_t no_sources = 0;
		const NDIlib_source_t* p_sources = NDIlib_find_get_current_sources(pNDI_find, &no_sources);

		// If there are sources available.
		if (no_sources) {
			// Connect to a random source
			const NDIlib_source_t* p_new_source = p_sources + (rand() % no_sources);
			printf("routing to %s \n", p_new_source->p_ndi_name);
			NDIlib_routing_change(pNDI_routing, p_new_source);
		} else {
			// Route to nowhere (black)
			NDIlib_routing_clear(pNDI_routing);
		}

		// Chill for a bit
		std::this_thread::sleep_for(std::chrono::seconds(15));
	}

	// Destroy the NDI finder
	NDIlib_find_destroy(pNDI_find);
	NDIlib_routing_destroy(pNDI_routing);

	// Not required, but nice
	NDIlib_destroy();

	// Success
	return 0;
}
