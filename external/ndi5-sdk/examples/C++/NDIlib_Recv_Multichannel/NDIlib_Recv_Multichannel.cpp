#include <cassert>
#include <csignal>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <atomic>
#include <chrono>
#include <thread>

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

struct receive_example {
	// Constructor
	receive_example(const int channel_no, const NDIlib_source_t& source);

	// Destructor
	~receive_example(void);

private:	// Create the receiver
	NDIlib_recv_instance_t m_pNDI_recv;

	// The thread to run
	std::thread m_receive_thread;

	// Are we ready to exit
	std::atomic<bool> m_exit;

	// The channel number
	const int m_channel_no;

	// This is called to receive frames
	void receive(void);
};

// Constructor
receive_example::receive_example(const int channel_no, const NDIlib_source_t& source)
	: m_pNDI_recv(NULL), m_exit(false), m_channel_no(channel_no)
{
	// We now have at least one source, so we create a receiver to look at it.
	// We tell it that we prefer YCbCr video since it is more efficient for us. If the source has an alpha channel
	// it will still be provided in BGRA
	NDIlib_recv_create_v3_t recv_create_desc;
	recv_create_desc.source_to_connect_to = source;
	recv_create_desc.bandwidth = NDIlib_recv_bandwidth_highest;
	recv_create_desc.p_ndi_recv_name = "Example Multichannel Receiver";

	// Display the frames per second
	printf("Channel %d is connecting to %s.\n", m_channel_no, source.p_ndi_name);

	// Create the receiver
	m_pNDI_recv = NDIlib_recv_create_v3(&recv_create_desc);
	assert(m_pNDI_recv);

	// Start a thread to receive frames
	m_receive_thread = std::thread(&receive_example::receive, this);
}

// Destructor
receive_example::~receive_example(void)
{
	// Wait for the thread to exit
	m_exit = true;
	m_receive_thread.join();

	// Destroy the receiver
	NDIlib_recv_destroy(m_pNDI_recv);
}

// This is called to receive frames
void receive_example::receive(void)
{
	// Keep track of times
	auto prev_time = std::chrono::high_resolution_clock::now();

	// Lets work until things end
	for (int frame_no = 0; !m_exit; ) {
		// The descriptors
		NDIlib_video_frame_v2_t video_frame;
		NDIlib_audio_frame_v2_t audio_frame;
		NDIlib_metadata_frame_t metadata_frame;

		switch (NDIlib_recv_capture_v2(m_pNDI_recv, &video_frame, &audio_frame, &metadata_frame, 1000)) {
			// No data
			case NDIlib_frame_type_none:
				break;

				// Video data
			case NDIlib_frame_type_video:
			{
				// Free the memory
				NDIlib_recv_free_video_v2(m_pNDI_recv, &video_frame);

				// Double check that we are running sufficiently well
				NDIlib_recv_queue_t recv_queue;
				NDIlib_recv_get_queue(m_pNDI_recv, &recv_queue);
				if (recv_queue.video_frames > 2) {
					// Display the frames per second
					printf("Channel %d queue depth is %d.\n", m_channel_no, recv_queue.video_frames);
				}

				// Every 1000 frames we check how long it has taken
				if (frame_no == 200) {
					// Get the time
					const auto this_time = std::chrono::high_resolution_clock::now();

					// Display the frames per second
					printf("Channel %d is receiving video at %1.1ffps.\n", m_channel_no, (float)frame_no / std::chrono::duration_cast<std::chrono::duration<float>>(this_time - prev_time).count());

					// Cycle the timers and reset the count
					prev_time = this_time;
					frame_no = 0;
				} else {
					frame_no++;
				}

				break;
			}

			// Audio data
			case NDIlib_frame_type_audio:
				NDIlib_recv_free_audio_v2(m_pNDI_recv, &audio_frame);
				break;

				// Meta data
			case NDIlib_frame_type_metadata:
				NDIlib_recv_free_metadata(m_pNDI_recv, &metadata_frame);
				break;

				// There is a status change on the receiver (e.g. new web interface)
			case NDIlib_frame_type_status_change:
				printf("Receiver connection status changed.\n");
				break;

				// Everything else
			default:
				break;
		}
	}
}

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
	::signal(SIGINT, sigint_handler);

	// Create a finder
	NDIlib_find_create_t NDI_find_create_desc; /* Defalt settings */
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

	// Start up a bunch of receivers
	// Note : Obviously it is tempting to specify a very high number of receivers here
	//        however it is important to remember that each one is probably going to take some
	//        real amount of network bandwidth (and some decompression time). In general on
	//        a mediocre 1Gbit ethernet you should easily be able to get 4 channels of HD video.
	//        If you have a well configured 1Gbe network then you should easily get to 8 channels.
	//        Beyond this you will need a fast machine and a 10Gbit ethernet in order to get more 
	//		  streams. For those that immediately want a very high input count, bear in mind that 
	//		  even getting 8 channels from an SDI capture card can push a machine, and other 
	//		  leading IP standards typically cannot do a single HD channel on a 1Gbe connection !
	static const int no_receivers = 4;
	receive_example* p_receivers[no_receivers] = { 0 };
	for (int idx = 0; idx < no_receivers; idx++)
		p_receivers[idx] = new receive_example(idx + 1, p_sources[0]);

	// Destroy the NDI finder. We needed to have access to the pointers to p_sources[0]
	NDIlib_find_destroy(pNDI_find);

	// Lets measure the performance for one minute
	std::this_thread::sleep_until(std::chrono::high_resolution_clock::now() + std::chrono::minutes(1));

	// Delete the receivers
	for (int idx = 0; idx < no_receivers; idx++)
		delete p_receivers[idx];

	// Not required, but nice
	NDIlib_destroy();

	// Finished
	return 0;
}
