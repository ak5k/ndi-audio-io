#include <cstdio>
#include <string.h>
#include <chrono>
#include <Processing.NDI.Lib.h>
#include "rapidxml/rapidxml.hpp"

#ifdef _WIN32
#define strcasecmp _stricmp
#ifdef _WIN64
#pragma comment(lib, "Processing.NDI.Lib.x64.lib")
#else // _WIN64
#pragma comment(lib, "Processing.NDI.Lib.x86.lib")
#endif // _WIN64
#endif // _WIN32

int main(int argc, char* argv[])
{
	// Not required, but "correct" (see the SDK documentation).
	if (!NDIlib_initialize())
		return 0;

	// Run a command line option to specify the source
	if (argc != 2)
		return 0;

	// We are going to create a receiver that receives very little data from the source.
	NDIlib_recv_create_v3_t recv_desc;
	recv_desc.bandwidth = NDIlib_recv_bandwidth_metadata_only;
	recv_desc.source_to_connect_to.p_ndi_name = argv[1];

	// We now have at least one source, so we create a receiver to look at it.
	NDIlib_recv_instance_t pNDI_recv = NDIlib_recv_create_v3(&recv_desc);
	if (!pNDI_recv)
		return 0;

	// Run for one minute
	using namespace std::chrono;
	for (const auto start = high_resolution_clock::now(); high_resolution_clock::now() - start < minutes(5);) {
		// We are going to get meta-data from the source
		NDIlib_metadata_frame_t metadata;
		if (NDIlib_recv_capture_v2(pNDI_recv, nullptr, nullptr, &metadata, 5000) == NDIlib_frame_type_metadata) {
			// Parse the XML
			try {
				rapidxml::xml_document<char> parser;
				parser.parse<0>((char*)metadata.p_data);

				// Get the tag
				rapidxml::xml_node<char>* p_node = parser.first_node();

				// Check its a node
				if ((!p_node) || (p_node->type() != rapidxml::node_element)) {
					// Not a valid message
				} else if (!::strcasecmp(p_node->name(), "ndi_tally_echo")) {
					// Get the zoom factor
					const rapidxml::xml_attribute<char>* p_on_program = p_node->first_attribute("on_program");
					const rapidxml::xml_attribute<char>* p_on_preview = p_node->first_attribute("on_preview");

					// Display the tally state
					printf(
						"Tally, on_program = %s, on_preview = %s\n",
						p_on_program ? p_on_program->value() : "false",
						p_on_preview ? p_on_preview->value() : "false"
					);
				}
			} catch (...) {
				// Bad XML somehow
			}

			// Free any meta-data 
			NDIlib_recv_free_metadata(pNDI_recv, &metadata);
		}
	}

	// Destroy the receiver
	NDIlib_recv_destroy(pNDI_recv);

	// Not required, but nice
	NDIlib_destroy();

	// Finished
	return 0;
}
