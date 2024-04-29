#include "PluginProcessor.h"
#include "PluginEditor.h"

std::mutex NdiAudioProcessor::init_mutex{};

//==============================================================================
NdiAudioProcessor::NdiAudioProcessor()
    // : AudioProcessor()
    : AudioProcessor(
          BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts{*this, nullptr, juce::Identifier("APVTS"), createParameterLayout()}
{
    std::scoped_lock init_lock(init_mutex);

    std::string ndi_runtime_path{};
    auto ndi_runtime_path_included =
#ifdef __linux__
        File::getSpecialLocation(File::userApplicationDataDirectory)
#else
        File::getSpecialLocation(File::commonApplicationDataDirectory)
#endif
            .getChildFile(JucePlugin_Name)
            .getChildFile(NDILIB_LIBRARY_NAME)
            .getFullPathName();

#ifdef __APPLE__
    ndi_runtime_path_included =
        File("/usr/local/lib")
            .getChildFile(NDILIB_LIBRARY_NAME)
            .getFullPathName();
#endif

#ifdef __linux__
    // linux and macos
    goto linux_load;
linux_load:
#define dynamic_load 1
    hNDILib = nullptr;
    const char *p_ndi_runtime_v5 = getenv(NDILIB_REDIST_FOLDER);
    if (p_ndi_runtime_v5)
    {
        ndi_runtime_path = p_ndi_runtime_v5;
        ndi_runtime_path += "/";
        ndi_runtime_path += NDILIB_LIBRARY_NAME;
    }

    auto f = juce::File(ndi_runtime_path);
    if (!f.existsAsFile())
        ndi_runtime_path = ndi_runtime_path_included.getCharPointer();

    f = juce::File(ndi_runtime_path);
    if (!f.existsAsFile())
    {
        printf(
            "Please re-install the NewTek NDI Runtimes from " NDILIB_REDIST_URL
            " to use this application.");
        return;
    }

    hNDILib = dlopen(ndi_runtime_path.c_str(), RTLD_LOCAL | RTLD_LAZY);

    // The main NDI entry point for dynamic loading if we got the librari
    const NDIlib_v5 *(*NDIlib_v5_load)(void) = nullptr;
    if (hNDILib)
    {
        *((void **)&NDIlib_v5_load) = dlsym(hNDILib, "NDIlib_v5_load");
    }
    if (!NDIlib_v5_load)
    {
        // Unload the library if we loaded it
        if (hNDILib)
            dlclose(hNDILib);

        printf(
            "Please re-install the NewTek NDI Runtimes from " NDILIB_REDIST_URL
            " to use this application.");
        return;
    }

#if TARGET_OS_MAC
    goto macos_post;
#endif
#elif _WIN32
    // Windows 32 and 64
#define dynamic_load 1

    char *p_ndi_runtime_v5 = nullptr;
    size_t sz = 0;
    auto err = _dupenv_s(&p_ndi_runtime_v5, &sz, NDILIB_REDIST_FOLDER);
    if (!err && p_ndi_runtime_v5 != nullptr)
    {
        ndi_runtime_path = p_ndi_runtime_v5;
        free(p_ndi_runtime_v5);
        ndi_runtime_path += "\\";
        ndi_runtime_path += NDILIB_LIBRARY_NAME;
    }

    auto f = juce::File(ndi_runtime_path);
    if (!f.existsAsFile())
        ndi_runtime_path = ndi_runtime_path_included.getCharPointer();

    f = juce::File(ndi_runtime_path);
    if (!f.existsAsFile())
    {
        MessageBoxA(NULL,
                    "Please re-install the NewTek NDI Runtimes to use this "
                    "application.",
                    "Runtime Warning.", MB_OK);
        ShellExecuteA(NULL, "open", NDILIB_REDIST_URL, 0, 0, SW_SHOWNORMAL);
        return;
    }
    hNDILib = LoadLibraryA(ndi_runtime_path.c_str());

    const NDIlib_v5 *(*NDIlib_v5_load)(void) = nullptr;
    if (hNDILib)
    {
        *((FARPROC *)&NDIlib_v5_load) =
            GetProcAddress(hNDILib, "NDIlib_v5_load");
    }

    if (!NDIlib_v5_load)
    {
        if (hNDILib)
            FreeLibrary(hNDILib);
        MessageBoxA(NULL,
                    "Please re-install the NewTek NDI Runtimes to use this "
                    "application.",
                    "Runtime Warning.", MB_OK);
        ShellExecuteA(NULL, "open", NDILIB_REDIST_URL, 0, 0, SW_SHOWNORMAL);
        return;
    }

#elif __APPLE__
#include "TargetConditionals.h" // <--- Include this
#ifdef TARGET_OS_IOS            // <--- Change this with a proper definition
// iOS, including iPhone and iPad
#elif TARGET_IPHONE_SIMULATOR
// iOS Simulator
#elif TARGET_OS_MAC
    goto macos_pre;
    // pre-load stuff
macos_pre:
    hNDILib = nullptr;

    goto linux_load;

    goto macos_post;
    // post-load stuff
macos_post:

#else
// Unsupported platform
#endif
#elif __ANDROID__
// Android all versions
#else
//  Unsupported architecture
#endif

    p_NDILib = NDIlib_v5_load();

    // We can now run as usual
    if (!p_NDILib->initialize())
    {
        // Cannot run NDI. Most likely because the CPU is not sufficient (see
        // SDK documentation). you can check this directly with a call to
        // NDIlib_is_supported_CPU()
        printf("Cannot run NDI.");
        return;
    }
    ndi_find_create.show_local_sources = true;
    ndi_find = p_NDILib->find_create_v2(&ndi_find_create);

    apvts.addParameterListener("recv", this);
    apvts.addParameterListener("send", this);
    apvts.addParameterListener("ndi_recv", this);

    if (juce::JUCEApplicationBase::isStandaloneApp())
    {
        auto pluginHolder = juce::StandalonePluginHolder::getInstance();

        is_standalone = true;
        if (pluginHolder)
        {
            pluginHolder->muteInput = false;
        }
    }

    metadata_string = "<ndi_product long_name=\"" JucePlugin_Name
                      "\" "
                      " short_name=\"" JucePlugin_Name
                      "\" "
                      " manufacturer=\"" JucePlugin_Manufacturer
                      "\" "
                      " version=\" " JucePlugin_VersionString "  \" />";

    ndi_metadata.p_data = metadata_string.data();

    return;
}

NdiAudioProcessor::~NdiAudioProcessor()
{
    if (!p_NDILib)
        return;

    if (ndi_find)
        p_NDILib->find_destroy(ndi_find);

    if (ndi_framesync)
        p_NDILib->framesync_destroy(ndi_framesync);

    if (ndi_recv)
        p_NDILib->recv_destroy(ndi_recv);

    if (ndi_send)
        p_NDILib->send_destroy(ndi_send);

    p_NDILib->destroy();

#ifdef dynamic_load
#if _WIN32
    FreeLibrary(hNDILib);
#else
    dlclose(hNDILib);
#endif
#endif
}

//==============================================================================
const juce::String NdiAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool NdiAudioProcessor::acceptsMidi() const
{
    return false;
}

bool NdiAudioProcessor::producesMidi() const
{
    return false;
}

bool NdiAudioProcessor::isMidiEffect() const
{
    return false;
}

double NdiAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int NdiAudioProcessor::getNumPrograms()
{
    return 1; // NB: some hosts don't cope very well if you tell them there are
              // 0 programs, so this should be at least 1, even if you're not
              // really implementing programs.
}

int NdiAudioProcessor::getCurrentProgram()
{
    return 0;
}

void NdiAudioProcessor::setCurrentProgram(int index)
{
    (void)index;
    return;
}

const juce::String NdiAudioProcessor::getProgramName(int index)
{
    (void)index;
    return {};
}

void NdiAudioProcessor::changeProgramName(int index,
                                          const juce::String &newName)
{
    (void)index;
    (void)newName;
}

//==============================================================================
void NdiAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    recv_buf.resize(
        static_cast<std::vector<float, std::allocator<float>>::size_type>(
            samplesPerBlock) *
        (size_t)getTotalNumOutputChannels());

    send_buf.resize(
        static_cast<std::vector<float, std::allocator<float>>::size_type>(
            samplesPerBlock) *
        (size_t)getTotalNumInputChannels());

    recv_audio_frame.p_data = recv_buf.data();
    send_audio_frame.p_data = send_buf.data();

    this->sample_rate = sampleRate;

    if (!p_NDILib)
        return;

    releaseResources();
    parseSendTextInput(getNDISendTextInput());
    ndi_send_create.p_ndi_name = ndi_send_name.getCharPointer();

    if (send_ok)
        ndi_send = p_NDILib->send_create(&ndi_send_create);

    ndi_recv_create.bandwidth = NDIlib_recv_bandwidth_max;
    ndi_recv_create.source_to_connect_to.p_ndi_name =
        ndi_recv_name.getCharPointer();

    ndi_recv_create.p_ndi_recv_name = ndi_send_name.getCharPointer();

    if (recv_ok)
        ndi_recv = p_NDILib->recv_create_v3(&ndi_recv_create);

    // We need a frame-sync.
    ndi_framesync = p_NDILib->framesync_create(ndi_recv);
}

void NdiAudioProcessor::releaseResources()
{
    if (!p_NDILib)
        return;

    if (ndi_framesync)
    {
        p_NDILib->framesync_destroy(ndi_framesync);
        ndi_framesync = nullptr;
    }

    // Stop the NDI receiver.
    if (ndi_recv)
    {
        p_NDILib->recv_destroy(ndi_recv);
        ndi_recv = nullptr;
    }

    if (ndi_send)
    {
        p_NDILib->send_destroy(ndi_send);
        ndi_send = nullptr;
    }
    return;
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool NdiAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
    (void)layouts;
    // This checks if the input layout matches the output layout

    return true;
}
#endif

template <typename T>
void NdiAudioProcessor::processBlock2(juce::AudioBuffer<T> &buffer,
                                      juce::MidiBuffer &midiMessages)
{
    (void)midiMessages;

    juce::ScopedNoDenormals noDenormals;
    const auto totalNumInputChannels = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    const auto numSamples = buffer.getNumSamples();
    const auto sampleRate = static_cast<int>(getSampleRate());

    if (!p_NDILib || !audio_lock.tryEnter())
    {
        if (is_standalone == true)
            for (auto i = 0; i < totalNumOutputChannels; i++)
                buffer.clear(i, 0, buffer.getNumSamples());
        return;
    }

    if (send_ok)
    {
        send_audio_frame.sample_rate = sampleRate;
        send_audio_frame.no_channels = totalNumInputChannels;
        send_audio_frame.no_samples = numSamples;
        send_audio_frame.channel_stride_in_bytes =
            numSamples * static_cast<int>(sizeof(T));

        for (auto i = 0; i < totalNumInputChannels; i++)
        {
            auto write_p = send_audio_frame.p_data;
            write_p += static_cast<unsigned long long>(
                i * send_audio_frame.channel_stride_in_bytes /
                static_cast<int>(sizeof(T)));
            auto read_p = buffer.getReadPointer(i);
            for (auto j = 0; j < numSamples; j++)
            {
                *write_p++ = static_cast<float>(*read_p++);
            }
        }
        p_NDILib->send_send_audio_v2(ndi_send, &send_audio_frame);
    }

    // clear output buffer
    if (is_standalone)
        for (auto i = 0; i < totalNumOutputChannels; i++)
            buffer.clear(i, 0, buffer.getNumSamples());

    if (recv_ok)
    {
        // clear output buffer
        if (!is_standalone)
            for (auto i = 0; i < totalNumOutputChannels; i++)
                buffer.clear(i, 0, buffer.getNumSamples());

        // get source channel count
        p_NDILib->framesync_capture_audio(ndi_framesync, &recv_audio_frame, 0,
                                          0, 0);

        auto num_source_channels = recv_audio_frame.no_channels;

        p_NDILib->framesync_capture_audio(ndi_framesync, &recv_audio_frame,
                                          sampleRate, num_source_channels,
                                          numSamples);

        // select channels logic
        auto select_channels_ok = false;

        if (recv_channels.size() > 0 &&
            recv_channels.size() <= totalNumOutputChannels)
        {
            select_channels_ok = true;
            for (auto &&i : recv_channels)
            {
                if (i > num_source_channels)
                    select_channels_ok = false;
            }
        }

        auto num_channels =
            select_channels_ok
                ? jmin((int)recv_channels.size(), totalNumOutputChannels)
                : jmin(totalNumOutputChannels, num_source_channels);

        for (auto i = 0; i < num_channels; i++)
        {
            auto n = i;
            if (select_channels_ok)
                n = recv_channels[i];

            // skip if -1
            if (n < 0)
                continue;

            auto read_p = recv_audio_frame.p_data;
            read_p += static_cast<unsigned long long>(
                n * recv_audio_frame.channel_stride_in_bytes /
                static_cast<int>(sizeof(T)));
            auto write_p = buffer.getWritePointer(i);
            for (auto j = 0; j < numSamples; j++)
            {
                *write_p++ = read_p ? *read_p++ : 0;
            }
        }

        // Free the original frame.
        p_NDILib->framesync_free_audio(ndi_framesync, &recv_audio_frame);
    }
    audio_lock.exit();
}

//==============================================================================
bool NdiAudioProcessor::hasEditor() const
{
    //
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor *NdiAudioProcessor::createEditor()
{
    return new NdiAudioProcessorEditor(*this);
}

//================================================================= =
void NdiAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    // block. You should use this method to store your parameters in
    // the memory block. You could do that either as raw data, or use the X
    // ML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.

    auto state = apvts.copyState();
    auto node = state.getOrCreateChildWithName("text_input", nullptr);
    node.setProperty("recv_text_input", recv_text_input, nullptr);
    node.setProperty("send_text_input", send_text_input, nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void NdiAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    // memory You should use this method to restore your paramete
    // rs from this memory block, whose contents will have been created by the
    // getStateInformation()
    // call.
    std::unique_ptr<juce::XmlElement> xmlState(
        getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));

    auto node = apvts.state.getOrCreateChildWithName("text_input", nullptr);
    parseRecvTextInput(node.getProperty("recv_text_input"));
    parseSendTextInput(node.getProperty("send_text_input"));

    parameterChanged("recv", apvts.getRawParameterValue("recv")->load());
    parameterChanged("send", apvts.getRawParameterValue("send")->load());

    return;
}

void NdiAudioProcessor::parameterChanged(const String &parameterID,
                                         float newValue)
{
    if (!p_NDILib)
        return;

    parameter_lock.enter();
    audio_lock.enter();

    if (parameterID == "send")
    {
        send_ok = false;

        if (newValue >= 0.5f && ndi_send_name.isNotEmpty())
        {
            if (ndi_send)
            {
                auto p = ndi_send;
                ndi_send = nullptr;
                p_NDILib->send_destroy(p);
            }

            auto name = ndi_send_name.getCharPointer();

            ndi_send_create.p_ndi_name = name;
            ndi_send_create.p_groups =
                groups.joinIntoString(",").getCharPointer();
            ndi_send_create.clock_audio = true;
            ndi_send = p_NDILib->send_create(&ndi_send_create);

            p_NDILib->send_add_connection_metadata(ndi_send, &ndi_metadata);

            send_ok = true;
        }
    }
    if (parameterID == "recv" || parameterID == "ndi_recv")
    {
        recv_ok = false;

        if (newValue >= 0.5f || parameterID == "ndi_recv")
        {
            if (ndi_framesync)
            {
                p_NDILib->framesync_destroy(ndi_framesync);
                ndi_framesync = nullptr;
            }

            if (ndi_recv)
            {
                auto p = ndi_recv;
                ndi_recv = nullptr;
                p_NDILib->recv_destroy(p);
            }

            auto name = ndi_recv_name.getCharPointer();
            ndi_recv_create.source_to_connect_to.p_ndi_name = name;

            ndi_recv_create.p_ndi_recv_name = ndi_send_name.getCharPointer();

            ndi_recv = p_NDILib->recv_create_v3(&ndi_recv_create);
            ndi_framesync = p_NDILib->framesync_create(ndi_recv);

            p_NDILib->recv_add_connection_metadata(ndi_recv, &ndi_metadata);

            recv_ok = true;
        }
    }

    audio_lock.exit();
    parameter_lock.exit();
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new NdiAudioProcessor();
}
