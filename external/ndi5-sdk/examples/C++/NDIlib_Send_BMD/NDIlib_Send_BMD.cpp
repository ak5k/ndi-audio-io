#include <csignal>
#include <cstdio>
#include <atomic>
#include <chrono>
#include <locale>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <Processing.NDI.Lib.h>

#ifdef _WIN32

#ifdef _WIN64
#pragma comment(lib, "Processing.NDI.Lib.x64.lib")
#else // _WIN64
#pragma comment(lib, "Processing.NDI.Lib.x86.lib")
#endif // _WIN64

#include <codecvt>
#ifndef snprintf
#define snprintf _snprintf
#endif
#include "DeckLinkAPI_h.h"
#else
#include "DeckLinkAPI.h"
#include "DeckLinkAPIDispatch.cpp"
#endif

#ifdef _WIN32
typedef BOOL bool_type;
#else
typedef bool bool_type;
#endif

struct decklink_ndi_bridge : public IDeckLinkInputCallback {
	decklink_ndi_bridge(IDeckLinkInput* p_decklink_input, const std::string& ndi_name)
		: m_p_decklink_input(p_decklink_input)
		, m_p_decklink_video_frame(NULL)
		, m_p_ndi_send(NULL)
		, m_frame_rate_n(30000), m_frame_rate_d(1001)
		, m_frame_format(NDIlib_frame_format_type_interleaved)
	{
		// Going to keep a reference to this decklink device
		m_p_decklink_input->AddRef();

		// Setup the NDI sender parameters
		NDIlib_send_create_t ndi_send_create_desc;
		ndi_send_create_desc.p_ndi_name = ndi_name.c_str();

		// Create the NDI sender instance
		m_p_ndi_send = NDIlib_send_create(&ndi_send_create_desc);
		if (!m_p_ndi_send) {
			m_p_decklink_input->Release();
			m_p_decklink_input = NULL;
			return;
		}
	}

	virtual ~decklink_ndi_bridge(void)
	{
		// Release the decklink device
		if (m_p_decklink_input) {
			m_p_decklink_input->StopStreams();
			m_p_decklink_input->Release();
			m_p_decklink_input = NULL;
		}

		// Release the NDI sender instance
		if (m_p_ndi_send) {
			NDIlib_send_destroy(m_p_ndi_send);
			m_p_ndi_send = NULL;
		}

		// Release the async frame data
		if (m_p_decklink_video_frame) {
			m_p_decklink_video_frame->Release();
			m_p_decklink_video_frame = NULL;
		}
	}

	bool activate(void)
	{
		if (!m_p_decklink_input || !m_p_ndi_send)
			return false;

		// Flags to use, depending on the input's capabilities
		BMDVideoInputFlags video_flags = bmdVideoInputFlagDefault;

		// Figure out some of the capabilities of this input
		IDeckLinkAttributes* p_decklink_attr = NULL;
		if (SUCCEEDED(m_p_decklink_input->QueryInterface(IID_IDeckLinkAttributes, reinterpret_cast<void**>(&p_decklink_attr)))) {
			// Does this device support format detection?
			bool_type supports_fmt_detection = false;
			if (SUCCEEDED(p_decklink_attr->GetFlag(BMDDeckLinkSupportsInputFormatDetection, &supports_fmt_detection)) && supports_fmt_detection)
				video_flags |= bmdVideoInputEnableFormatDetection;
		}

		// Set up the initial audio and video formats
		m_p_decklink_input->EnableVideoInput(bmdModeHD1080i5994, bmdFormat8BitYUV, video_flags);
		m_p_decklink_input->EnableAudioInput(bmdAudioSampleRate48kHz, bmdAudioSampleType16bitInteger, num_audio_channels);
		m_p_decklink_input->SetCallback(this);

		// Begin streaming
		return SUCCEEDED(m_p_decklink_input->StartStreams());
	}

	virtual HRESULT STDMETHODCALLTYPE VideoInputFormatChanged(BMDVideoInputFormatChangedEvents notificationEvents, IDeckLinkDisplayMode* newDisplayMode, BMDDetectedVideoInputFormatFlags detectedSignalFlags) override
	{
		// Get the new frame rate
		BMDTimeValue frame_rate_n, frame_rate_d;
		newDisplayMode->GetFrameRate(&frame_rate_d, &frame_rate_n);

		// Keep track of what this new frame rate is
		m_frame_rate_n = static_cast<int>(frame_rate_n);
		m_frame_rate_d = static_cast<int>(frame_rate_d);

		// Output useful information for testing
		printf("Format change detected\n");
		printf("Resolution: %ldx%ld\n", newDisplayMode->GetWidth(), newDisplayMode->GetHeight());
		printf("Frame rate: %d/%d\n", m_frame_rate_n, m_frame_rate_d);

		// Map the fielding type
		switch (newDisplayMode->GetFieldDominance()) {
			case bmdLowerFieldFirst:
				m_frame_format = NDIlib_frame_format_type_interleaved;
				printf("Frame type: bmdLowerFieldFirst\n");
				break;

			case bmdUpperFieldFirst:
				m_frame_format = NDIlib_frame_format_type_interleaved;
				printf("Frame type: bmdUpperFieldFirst\n");
				break;

			case bmdProgressiveFrame:
				m_frame_format = NDIlib_frame_format_type_progressive;
				printf("frame type: bmdProgressiveFrame\n");
				break;

			case bmdProgressiveSegmentedFrame:
				m_frame_format = NDIlib_frame_format_type_progressive;
				printf("Frame type: bmdProgressiveSegmentedFrame\n");
				break;

			default:
				m_frame_format = NDIlib_frame_format_type_interleaved;
				printf("Frame type: bmdUnknownFieldDominance\n");
				break;
		}

		printf("\n");

		// Pause the stream and set it up for the new video format
		m_p_decklink_input->PauseStreams();
		m_p_decklink_input->EnableVideoInput(newDisplayMode->GetDisplayMode(), bmdFormat8BitYUV, bmdVideoInputEnableFormatDetection);
		m_p_decklink_input->FlushStreams();
		m_p_decklink_input->StartStreams();
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE VideoInputFrameArrived(IDeckLinkVideoInputFrame* videoFrame, IDeckLinkAudioInputPacket* audioPacket) override
	{
		if (videoFrame && (videoFrame->GetFlags() & bmdFrameHasNoInputSource))
			return S_OK;

		// Send the video frame if one is available
		if (videoFrame)
			send(videoFrame);

		// Send the audio frame if one is available
		if (audioPacket)
			send(audioPacket);

		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID* ppv) override { return E_NOINTERFACE; };
	virtual ULONG   STDMETHODCALLTYPE AddRef(void) override { return 1; };
	virtual ULONG   STDMETHODCALLTYPE Release(void) override { return 0; };

private:
	void send(IDeckLinkVideoInputFrame* deckLinkVideoFrame)
	{
		// Save the reference to the previous frame
		IDeckLinkVideoInputFrame* p_prev_video_frame = m_p_decklink_video_frame;

		// Fill in the NDI video frame information
		NDIlib_video_frame_v2_t ndi_video_frame = { };
		ndi_video_frame.timecode = NDIlib_send_timecode_synthesize;
		ndi_video_frame.FourCC = NDIlib_FourCC_type_UYVY;
		ndi_video_frame.xres = deckLinkVideoFrame->GetWidth();
		ndi_video_frame.yres = deckLinkVideoFrame->GetHeight();
		ndi_video_frame.line_stride_in_bytes = deckLinkVideoFrame->GetRowBytes();
		ndi_video_frame.frame_rate_N = m_frame_rate_n;
		ndi_video_frame.frame_rate_D = m_frame_rate_d;
		ndi_video_frame.frame_format_type = m_frame_format;
		ndi_video_frame.picture_aspect_ratio = 16.f / 9.f;

		// Retrieve the pointer to the video data
		deckLinkVideoFrame->GetBytes((void**)&ndi_video_frame.p_data);

		// Keep a reference to this video frame for async purposes
		m_p_decklink_video_frame = deckLinkVideoFrame;
		m_p_decklink_video_frame->AddRef();

		// Send the video frame
		NDIlib_send_send_video_async_v2(m_p_ndi_send, &ndi_video_frame);

		// It's now safe to release the reference to the previous video frame
		if (p_prev_video_frame)
			p_prev_video_frame->Release();
	}

	void send(IDeckLinkAudioInputPacket* deckLinkAudioPacket)
	{
		NDIlib_audio_frame_interleaved_16s_t ndi_audio_frame = { };

		// Fill in the NDI audio frame information
		ndi_audio_frame.timecode = NDIlib_send_timecode_synthesize;
		ndi_audio_frame.no_samples = deckLinkAudioPacket->GetSampleFrameCount();
		ndi_audio_frame.no_channels = num_audio_channels;
		ndi_audio_frame.sample_rate = 48000;
		ndi_audio_frame.reference_level = 0;

		// Retrieve the pointer to the audio data
		deckLinkAudioPacket->GetBytes((void**)&ndi_audio_frame.p_data);

		// Send the audio frame
		NDIlib_util_send_send_audio_interleaved_16s(m_p_ndi_send, &ndi_audio_frame);
	}

private:
	const int num_audio_channels = 8;

	IDeckLinkInput* m_p_decklink_input;
	IDeckLinkVideoInputFrame* m_p_decklink_video_frame;
	NDIlib_send_instance_t    m_p_ndi_send;
	int m_frame_rate_n;
	int m_frame_rate_d;
	NDIlib_frame_format_type_e m_frame_format;
};

#ifdef _WIN32
static IDeckLinkIterator* CreateDeckLinkIteratorInstance(void)
{
	// Ensure COM is initialized
	if (FAILED(CoInitialize(NULL)))
		return NULL;

	// Create the decklink iterator
	IDeckLinkIterator* p_decklink_iterator;
	if (FAILED(CoCreateInstance(CLSID_CDeckLinkIterator, NULL, CLSCTX_ALL, IID_IDeckLinkIterator, (void**)&p_decklink_iterator)))
		return NULL;

	return p_decklink_iterator;
}
#endif

static std::string get_decklink_device_name(IDeckLink* p_decklink_device)
{
	std::string display_name;

#ifdef _WIN32

	BSTR p_display_name = NULL;

	// Retrieve the device's name
	if (SUCCEEDED(p_decklink_device->GetDisplayName(&p_display_name))) {
		// Convert to std::string then free
		display_name = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(p_display_name);
		SysFreeString(p_display_name);
	}

#else

	const char* p_display_name = NULL;

	// Retrieve the device's name
	if (SUCCEEDED(p_decklink_device->GetDisplayName(&p_display_name))) {
		// Convert to std::string then free
		display_name = std::string(p_display_name);
		free((void*)p_display_name);
	}

#endif

	return display_name;
}

std::vector<std::pair<std::string, IDeckLinkInput*>> get_decklink_input_devices(void)
{
	std::vector<std::pair<std::string, IDeckLinkInput*>> input_devices;

	IDeckLinkIterator* p_decklink_iterator = CreateDeckLinkIteratorInstance();
	if (p_decklink_iterator) {
		IDeckLink* p_decklink_device = NULL;
		while (SUCCEEDED(p_decklink_iterator->Next(&p_decklink_device)) && p_decklink_device) {
			// Get the decklink input interface -- ensures this device supports input
			IDeckLinkInput* p_decklink_input = NULL;
			if (SUCCEEDED(p_decklink_device->QueryInterface(IID_IDeckLinkInput, reinterpret_cast<void**>(&p_decklink_input)))) {
				// Figure out some of the capabilities of this input
				IDeckLinkAttributes* p_decklink_attr = NULL;
				if (SUCCEEDED(p_decklink_device->QueryInterface(IID_IDeckLinkAttributes, reinterpret_cast<void**>(&p_decklink_attr)))) {
					// Can this device change its duplex mode?
					bool_type can_change_duplex_mode = false;
					if (SUCCEEDED(p_decklink_attr->GetFlag(BMDDeckLinkSupportsDuplexModeConfiguration, &can_change_duplex_mode)) && can_change_duplex_mode) {
						// If we can configure the duplex mode, switch to half-duplex
						IDeckLinkConfiguration* p_decklink_config = NULL;
						if (SUCCEEDED(p_decklink_device->QueryInterface(IID_IDeckLinkConfiguration, reinterpret_cast<void**>(&p_decklink_config)))) {
							p_decklink_config->SetInt(bmdDeckLinkConfigDuplexMode, bmdDuplexModeHalf);
							p_decklink_config->Release();
						}
					}

					p_decklink_attr->Release();
				}

				// Saving the reference to the input device for later
				input_devices.push_back(std::make_pair(get_decklink_device_name(p_decklink_device), p_decklink_input));
			}

			p_decklink_device->Release();
		}

		p_decklink_iterator->Release();
	}

	return input_devices;
}

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

	// Get the list of decklink input devices and their names
	std::vector<std::pair<std::string, IDeckLinkInput*>> decklink_devices = get_decklink_input_devices();

	// This will be the list of decklink-to-NDI bridges
	std::vector<decklink_ndi_bridge*> bridges;

	for (auto& device : decklink_devices) {
		// Create the decklink-to-NDI bridge
		decklink_ndi_bridge* p_bridge = new decklink_ndi_bridge(device.second, device.first);

		// If it's been successfully activated, keep track of it
		if (p_bridge->activate())
			bridges.push_back(p_bridge);
		else
			delete p_bridge;

		device.second->Release();
	}

	if (!bridges.empty()) {
		// Wait until exit
		while (!exit_loop)
			std::this_thread::sleep_for(std::chrono::seconds(1));

		// Shut everything down
		for (decklink_ndi_bridge* p_bridge : bridges)
			delete p_bridge;
	}

	// Not required, but nice
	NDIlib_destroy();

	// Success
	return 0;
}
