#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <chrono>
#include <algorithm>
#include <Processing.NDI.Lib.h>

#include "rapidxml/rapidxml.hpp"

#ifdef _WIN32
#define strcasecmp _stricmp
#ifdef _WIN64
#pragma comment(lib, "Processing.NDI.Lib.x64.lib")
#else // _WIN64
#pragma comment(lib, "Processing.NDI.Lib.x86.lib")
#endif // _WIN64
#else
#include <strings.h>
#endif // _WIN32

int main(int argc, char* argv[])
{
	// Not required, but "correct" (see the SDK documentation).
	if (!NDIlib_initialize())
		return 0;

	// We create the NDI sender
	NDIlib_send_instance_t pNDI_send = NDIlib_send_create();
	if (!pNDI_send)
		return 0;

	// We are going to create a 1920x1080 interlaced frame at 29.97Hz.
	NDIlib_video_frame_v2_t NDI_video_frame;
	NDI_video_frame.xres = 1920;
	NDI_video_frame.yres = 1080;
	NDI_video_frame.FourCC = NDIlib_FourCC_type_BGRX;
	NDI_video_frame.p_data = (uint8_t*)malloc(NDI_video_frame.xres * NDI_video_frame.yres * 4);

	// We are going to mark this as if it was a PTZ camera.
	NDIlib_metadata_frame_t NDI_capabilities;
	NDI_capabilities.p_data = "<ndi_capabilities ntk_ptz=\"true\" web_control=\"http://ndi.newtek.com/\" ntk_exposure_v2=\"true\"/>"; // Your camera web page would go here instead of ndi.newtek.com
	NDIlib_send_add_connection_metadata(pNDI_send, &NDI_capabilities);

	// Run for one minute
	using namespace std::chrono;
	int frame_no = 0;
	for (const auto start = high_resolution_clock::now(); high_resolution_clock::now() - start < minutes(5);) {
		// Get the current time
		const auto start_send = high_resolution_clock::now();

		// Fill in the buffer. It is likely that you would do something much smarter than this. This should really render
		// something based on the current PTZ settings. This example is not that fancy but you could do something really 
		// cool here if you want.
		memset((void*)NDI_video_frame.p_data, ((frame_no++) & 1) ? 255 : 0, NDI_video_frame.xres * NDI_video_frame.yres * 4);

		// We now submit the frame. Note that this call will be clocked so that we end up submitting at exactly 29.97fps.
		NDIlib_send_send_video_v2(pNDI_send, &NDI_video_frame);

		// Process any commands received from the other end of the connection.
		NDIlib_metadata_frame_t metadata_cmd;
		while (NDIlib_send_capture(pNDI_send, &metadata_cmd, 0) == NDIlib_frame_type_metadata) {
			// Parse the XML
			try {
				// Get the parser
				std::string xml(metadata_cmd.p_data);
				rapidxml::xml_document<char> parser;
				parser.parse<0>((char*)xml.data());

				// Get the tag
				rapidxml::xml_node<char>* p_node = parser.first_node();

				// Check its a node
				if ((!p_node) || (p_node->type() != rapidxml::node_element)) {
					// Not a valid message
				}

				// Start recording
				if (!::strcasecmp(p_node->name(), "ntk_ptz_zoom")) {
					// Get the zoom factor
					const rapidxml::xml_attribute<char>* p_zoom = p_node->first_attribute("zoom");
					const float zoom = p_zoom ? (float)::atof(p_zoom->value()) : 0.0f;

					// Display what just happened
					printf("Zoom = %1.2f\n", std::max(0.0f, std::min(1.0f, zoom)));
				} else if (!::strcasecmp(p_node->name(), "ntk_ptz_zoom_speed")) {
					// Get the zoom factor
					const rapidxml::xml_attribute<char>* p_zoom_speed = p_node->first_attribute("zoom_speed");
					const float zoom_speed = p_zoom_speed ? (float)::atof(p_zoom_speed->value()) : 0.0f;

					// Display what just happened
					printf("Change Zoom at speed = %1.2f\n", std::max(-1.0f, std::min(1.0f, zoom_speed)));
				} else if (!::strcasecmp(p_node->name(), "ntk_ptz_pan_tilt_speed")) {
					// Get the values
					const rapidxml::xml_attribute<char>* p_pan_speed = p_node->first_attribute("pan_speed");
					const rapidxml::xml_attribute<char>* p_tilt_speed = p_node->first_attribute("tilt_speed");
					const float pan_speed = p_pan_speed ? (float)::atof(p_pan_speed->value()) : 0.0f;
					const float tilt_speed = p_tilt_speed ? (float)::atof(p_tilt_speed->value()) : 0.0f;

					// Display what just happened
					printf("Move Pan, Tilt at speed = [%1.2f, %1.2f]\n", std::max(-1.0f, std::min(1.0f, pan_speed)), std::max(-1.0f, std::min(1.0f, tilt_speed)));
				} else if (!::strcasecmp(p_node->name(), "ntk_ptz_pan_tilt")) {
					// Get the values
					const rapidxml::xml_attribute<char>* p_pan = p_node->first_attribute("pan");
					const rapidxml::xml_attribute<char>* p_tilt = p_node->first_attribute("tilt");
					const float pan = p_pan ? (float)::atof(p_pan->value()) : 0.0f;
					const float tilt = p_tilt ? (float)::atof(p_tilt->value()) : 0.0f;

					// Display what just happened
					printf("Move Pan, Tilt to speed = [%1.2f, %1.2f]\n", std::max(-1.0f, std::min(1.0f, pan)), std::max(-1.0f, std::min(1.0f, tilt)));
				} else if (!::strcasecmp(p_node->name(), "ntk_ptz_store_preset")) {
					// Get the values
					const rapidxml::xml_attribute<char>* p_index = p_node->first_attribute("index");
					const int index = p_index ? ::atoi(p_index->value()) : 0;

					// Display what just happened
					if ((index >= 0) && (index < 100))
						printf("Store preset = %d\n", std::max(0, std::min(99, index)));
				} else if (!::strcasecmp(p_node->name(), "ntk_ptz_recall_preset")) {
					// Get the values
					const rapidxml::xml_attribute<char>* p_index = p_node->first_attribute("index");
					const rapidxml::xml_attribute<char>* p_speed = p_node->first_attribute("speed");
					const int   index = p_index ? ::atoi(p_index->value()) : 0;
					const float speed = p_speed ? (float)::atof(p_speed->value()) : 1.0f;

					// Display it
					if ((index >= 0) && (index < 100))
						printf("Recall preset = %d at speed = %1.2f\n", index, std::max(0.0f, std::min(1.0f, speed)));
				} else if (!::strcasecmp(p_node->name(), "ntk_ptz_flip")) {
					// Get the values
					const rapidxml::xml_attribute<char>* p_enabled = p_node->first_attribute("enabled");
					const bool enabled = p_enabled ? (::strcasecmp(p_enabled->value(), "true") == 0) : false;

					// Display it
					printf("Flip camera = %s\n", enabled ? "true" : "false");
				} else if (!::strcasecmp(p_node->name(), "ntk_ptz_focus")) {
					// Get the values
					const rapidxml::xml_attribute<char>* p_mode = p_node->first_attribute("mode");
					const rapidxml::xml_attribute<char>* p_distance = p_node->first_attribute("distance");

					// The possible modes
					enum e_focus_mode { e_focus_mode_auto, e_focus_mode_manual };

					// Get the auto-focus mode
					e_focus_mode mode = e_focus_mode_auto;
					if (!p_mode) {
					} else if (!::strcasecmp(p_mode->value(), "auto")) {
						mode = e_focus_mode_auto;
					} else if (!::strcasecmp(p_mode->value(), "manual")) {
						mode = e_focus_mode_manual;
					} else { }

					// Get the focus distance
					const float distance = p_distance ? (float)::atof(p_distance->value()) : 0.5f;

					// Make the callback
					if (mode == e_focus_mode_auto)
						printf("Auto focus on.\n");
					else if (mode == e_focus_mode_manual)
						printf("Manual focus to distance = %1.2f\n", std::max(0.0f, std::min(1.0f, distance)));
				} else if (!::strcasecmp(p_node->name(), "ntk_ptz_focus_speed")) {
					// Get the zoom factor
					const rapidxml::xml_attribute<char>* p_focus_speed = p_node->first_attribute("distance");
					const float focus_speed = p_focus_speed ? (float)::atof(p_focus_speed->value()) : 0.0f;

					// Make the callback
					printf("Move manual focus at speed = %1.2f\n", std::max(-1.0f, std::min(1.0f, focus_speed)));

					// SUccess
					return true;
				} else if (!::strcasecmp(p_node->name(), "ntk_ptz_white_balance")) {
					// Get the values
					const rapidxml::xml_attribute<char>* p_mode = p_node->first_attribute("mode");
					const rapidxml::xml_attribute<char>* p_red = p_node->first_attribute("red");
					const rapidxml::xml_attribute<char>* p_blue = p_node->first_attribute("blue");

					enum e_white_balance_mode {
						e_white_balance_mode_auto, e_white_balance_mode_indoor, e_white_balance_mode_outdoor,
						e_white_balance_mode_one_push, e_white_balance_mode_manual
					};

					// Get the auto-focus mode
					e_white_balance_mode mode = e_white_balance_mode_auto;
					if (!p_mode) {
					} else if (!::strcasecmp(p_mode->value(), "auto")) {
						mode = e_white_balance_mode_auto;
					} else if (!::strcasecmp(p_mode->value(), "indoor")) {
						mode = e_white_balance_mode_indoor;
					} else if (!::strcasecmp(p_mode->value(), "outdoor")) {
						mode = e_white_balance_mode_outdoor;
					} else if (!::strcasecmp(p_mode->value(), "one_push")) {
						mode = e_white_balance_mode_one_push;
					} else if (!::strcasecmp(p_mode->value(), "manual")) {
						mode = e_white_balance_mode_manual;
					} else {
					}

					// Get the focus distance
					const float red = p_red ? (float)::atof(p_red->value()) : 0.5f;
					const float blue = p_blue ? (float)::atof(p_blue->value()) : 0.5f;

					if (mode == e_white_balance_mode_auto) {
						printf("Set white-balance into auto mode.\n");
					} else if (mode == e_white_balance_mode_indoor) {
						printf("Set white-balance into indoor mode.\n");
					} else if (mode == e_white_balance_mode_outdoor) {
						printf("Set white-balance into outdoor mode.\n");
					} else if (mode == e_white_balance_mode_one_push) {
						printf("Set white-balance into one-push mode (i.e. snap shot the current auto white-balance).\n");
					} else if (mode == e_white_balance_mode_manual) {
						printf(
							"Set white-balance into manual mode with red, blue = [ %1.2f, %1.2f ]\n",
							std::max(0.0f, std::min(1.0f, red)), std::max(0.0f, std::min(1.0f, blue))
						);
					}
				} else if (!::strcasecmp(p_node->name(), "ntk_ptz_exposure")) {
					// Get the values
					const rapidxml::xml_attribute<char>* p_mode = p_node->first_attribute("mode");
					const rapidxml::xml_attribute<char>* p_iris = p_node->first_attribute("value");
					const rapidxml::xml_attribute<char>* p_gain = p_node->first_attribute("gain");
					const rapidxml::xml_attribute<char>* p_shutter = p_node->first_attribute("shutter");

					enum e_exposure_mode { e_exposure_mode_auto, e_exposure_mode_manual };

					// Get the auto-exposure mode
					e_exposure_mode mode = e_exposure_mode_auto;
					if (!p_mode) {
					} else if (!::strcasecmp(p_mode->value(), "auto")) {
						mode = e_exposure_mode_auto;
					} else if (!::strcasecmp(p_mode->value(), "manual")) {
						mode = e_exposure_mode_manual;
					} else {
					}

					// Get the iris value
					const float iris = p_iris ? (float)::atof(p_iris->value()) : 0.5f;
					// Get the gain(iso) value
					const float gain = p_gain ? (float)::atof(p_gain->value()) : 0.0f;
					// Get the shutter speed value
					const float shutter = p_shutter ? (float)::atof(p_shutter->value()) : 0.0f;


					// Make the callback
					if (mode == e_exposure_mode_auto) {
						printf("Set exposure into auto mode.\n");
					} else if (mode == e_exposure_mode_manual) {
						printf(
							"Set exposure into manual mode with iris = %1.2f gain = %1.2f shutter speed = %1.2f\n",
							std::max(0.0f, std::min(1.0f, iris)), std::max(0.0f, std::min(1.0f, gain)), std::max(0.0f, std::min(1.0f, shutter))
						);
					}

					// Success
					return true;
				}
			} catch (...) {
			}

			// Free the metadata memory
			NDIlib_send_free_metadata(pNDI_send, &metadata_cmd);
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
