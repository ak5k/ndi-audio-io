#include <csignal>
#include <string>
#include <cstddef>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <atomic>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#else
#include <stdlib.h>
#include <dlfcn.h>
#endif

#include "Processing.NDI.Lib.h"

static std::atomic<bool> exit_loop(false);
static void sigint_handler(int) { exit_loop = true; }

int main(int argc, char* argv[])
{
#ifdef _WIN32
	// We check whether the NDI run-time is installed
	const char* p_ndi_runtime_v4 = getenv(NDILIB_REDIST_FOLDER);
	if (!p_ndi_runtime_v4) {
		// The NDI run-time is not yet installed. Let the user know and take them to the download URL.
		MessageBoxA(NULL, "Please install the NewTek NDI Runtimes to use this application.", "Runtime Warning.", MB_OK);
		ShellExecuteA(NULL, "open", NDILIB_REDIST_URL, 0, 0, SW_SHOWNORMAL);
		return 0;
	}

	// We now load the DLL as it is installed
	std::string ndi_path = p_ndi_runtime_v4;
	ndi_path += "\\" NDILIB_LIBRARY_NAME;

	// Try to load the library
	HMODULE hNDILib = LoadLibraryA(ndi_path.c_str());

	// The main NDI entry point for dynamic loading if we got the librari
	const NDIlib_v4* (*NDIlib_v4_load)(void) = NULL;
	if (hNDILib)
		*((FARPROC*)&NDIlib_v4_load) = GetProcAddress(hNDILib, "NDIlib_v4_load");

	// If we failed to load the library then we tell people to re-install it
	if (!NDIlib_v4_load) {
		// Unload the DLL if we loaded it
		if (hNDILib)
			FreeLibrary(hNDILib);

		// The NDI run-time is not installed correctly. Let the user know and take them to the download URL.
		MessageBoxA(NULL, "Please re-install the NewTek NDI Runtimes to use this application.", "Runtime Warning.", MB_OK);
		ShellExecuteA(NULL, "open", NDILIB_REDIST_URL, 0, 0, SW_SHOWNORMAL);
		return 0;
	}
#else
	std::string ndi_path;

	const char* p_NDI_runtime_folder = getenv(NDILIB_REDIST_FOLDER);
	if (p_NDI_runtime_folder) {
		ndi_path = p_NDI_runtime_folder;
		ndi_path += NDILIB_LIBRARY_NAME;
	} else {
		ndi_path = NDILIB_LIBRARY_NAME;
	}

	// Try to load the library
	void* hNDILib = dlopen(ndi_path.c_str(), RTLD_LOCAL | RTLD_LAZY);

	// The main NDI entry point for dynamic loading if we got the library
	const NDIlib_v4* (*NDIlib_v4_load)(void) = NULL;
	if (hNDILib)
		*((void**)&NDIlib_v4_load) = dlsym(hNDILib, "NDIlib_v4_load");

	// If we failed to load the library then we tell people to re-install it
	if (!NDIlib_v4_load) {
		// Unload the library if we loaded it
		if (hNDILib)
			dlclose(hNDILib);

		printf("Please re-install the NewTek NDI Runtimes from " NDILIB_REDIST_URL " to use this application.");
		return 0;
	}
#endif

	// Lets get all of the DLL entry points
	const NDIlib_v4* p_NDILib = NDIlib_v4_load();

	// We can now run as usual
	if (!p_NDILib->initialize()) {
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
	NDIlib_send_instance_t pNDI_send = p_NDILib->send_create(&NDI_send_create_desc);
	if (!pNDI_send) return 0;

	// We are going to create a 1920x1080 interlaced frame at 29.97Hz.
	NDIlib_video_frame_v2_t NDI_video_frame;
	NDI_video_frame.FourCC = NDIlib_FourCC_type_BGRA;
	NDI_video_frame.xres = 1920;
	NDI_video_frame.yres = 1080;
	NDI_video_frame.line_stride_in_bytes = 1920 * 4;
	NDI_video_frame.p_data = (uint8_t*)malloc(1920 * 1080 * 4);

	// We will send 1000 frames of video. 
	while (!exit_loop) {
		// Get the current time
		const auto start_time = std::chrono::high_resolution_clock::now();

		// Send 200 frames
		for (int idx = 0; !exit_loop && idx < 200; idx++) {
			// Fill in the buffer. It is likely that you would do something much smarter than this.
			memset((void*)NDI_video_frame.p_data, (idx & 1) ? 255 : 0, 1920 * 1080 * 4);

			// We now submit the frame. Note that this call will be clocked so that we end up submitting at exactly 29.97fps.
			p_NDILib->send_send_video_v2(pNDI_send, &NDI_video_frame);
		}

		// Get the end time
		const auto end_time = std::chrono::high_resolution_clock::now();

		// Just display something helpful
		printf("200 frames sent, average fps=%1.2f\n", 200.0f / std::chrono::duration_cast<std::chrono::duration<float>>(end_time - start_time).count());
	}

	// Free the video frame
	free(NDI_video_frame.p_data);

	// Destroy the NDI sender
	p_NDILib->send_destroy(pNDI_send);

	// Not required, but nice
	p_NDILib->destroy();

	// Free the NDI Library
#if _WIN32
	FreeLibrary(hNDILib);
#else
	dlclose(hNDILib);
#endif

	// Success. We are done
	return 0;
}
