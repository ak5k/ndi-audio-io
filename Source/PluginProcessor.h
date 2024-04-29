/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>

// #include <shared_mutex>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#include <stdlib.h>
#endif
#include <Processing.NDI.Lib.h>
//==============================================================================
/**
 */

constexpr auto LINEAR_REGRESSION_POINTS = 512;
constexpr auto LISTEN_PORT = 55960;

class NdiAudioProcessor : public juce::AudioProcessor,
                          public juce::AudioProcessorValueTreeState::Listener
{
public:
    //==============================================================================
    NdiAudioProcessor();
    ~NdiAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout &layouts) const override;

    // handle different double and float samples with templating
    void processBlock(juce::AudioBuffer<float> &buffer,
                      juce::MidiBuffer &midi) override
    {
        processBlock2(buffer, midi);
    }

    void processBlock(juce::AudioBuffer<double> &buffer,
                      juce::MidiBuffer &midi) override
    {
        processBlock2(buffer, midi);
    }

    template <typename T>
    void processBlock2(juce::AudioBuffer<T> &, juce::MidiBuffer &);

    //==============================================================================
    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String &newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock &destData) override;
    void setStateInformation(const void *data, int sizeInBytes) override;

    void parameterChanged(const String &parameterID, float newValue) override;

    AudioProcessorValueTreeState &getAPVTS()
    {
        return apvts;
    }

    NDIlib_recv_instance_t getNDIRecv()
    {
        return ndi_recv;
    }

    NDIlib_find_instance_t getNDIFind()
    {
        return ndi_find;
    }

    String getNDIRecvName()
    {
        std::scoped_lock lock{text_mutex};
        return ndi_recv_name;
    }

    void parseRecvTextInput(String s)
    {
        std::scoped_lock lock{text_mutex};

        s = s.trim();
        recv_text_input = s;
        auto v = StringArray::fromTokens(s, ";", "\"");

        groups.clear();
        recv_channels.clear();
        for (auto &&i : v)
        {
            // name part
            if (v.indexOf(i) == 0)
            {
                i = i.trim();
                ndi_recv_name = i;
            }

            // channels
            if (v.indexOf(i) == 1)
            {
                auto t = StringArray::fromTokens(i, ",", "");
                t.trim();
                // t.removeDuplicates(false);
                t.removeEmptyStrings();
                for (auto &&k : t)
                {
                    auto j = k.retainCharacters("1234567890-");
                    if (j.matchesWildcard("*-*", true))
                    {
                        auto n = j.indexOf("-");

                        if (n <= 0)
                            continue;

                        auto first = j.substring(0, n).getIntValue();
                        auto second = j.substring(n + 1).getIntValue();

                        // convert to zero index
                        // e.g. 0,1,2,3, or -1,-1,-1,-1 for channel zero
                        for (; (first > 0 ? first - 1 : first) < second;
                             (first > 0 ? first++ : second--))
                        {
                            recv_channels.add(first - 1);
                        }
                    }
                    else if (j.getIntValue() >= 0)
                    {
                        recv_channels.add(j.getIntValue() - 1);
                    }
                }
            }
        }
    }

    String getNDISendName()
    {
        std::scoped_lock lock{text_mutex};
        return ndi_send_name;
    }

    String getNDISendTextInput()
    {
        std::scoped_lock lock{text_mutex};
        return send_text_input;
    }

    String getNDIRecvTextInput()
    {
        std::scoped_lock lock{text_mutex};
        return recv_text_input;
    }

    // actual NDI send name
    String getNDISendName2()
    {
        std::scoped_lock lock{text_mutex};
        return ndi_send ? p_NDILib->send_get_source_name(ndi_send)->p_ndi_name
                        : "";
    }

    void parseSendTextInput(String s)
    {
        text_mutex.lock();

        s = s.trim();
        send_text_input = s;
        auto v = StringArray::fromTokens(s, ";", "\"");

        groups.clear();
        for (auto &&i : v)
        {
            // name part
            if (v.indexOf(i) == 0)
            {
                i = i.trim();
                ndi_send_name = i;
            }

            // groups
            if (v.indexOf(i) == 1)
            {
                i = i.trim();
                auto t = StringArray::fromTokens(i, ",", "");
                t.trim();
                t.removeDuplicates(false);
                t.removeEmptyStrings();

                groups = t;
            }
        }

        text_mutex.unlock();

        if (s.isEmpty() || getNDISendName().isEmpty())
        {
            parseSendTextInput(Uuid().toString().substring(0, 8));
        }
    }

    NDIlib_send_instance_t getNDISend()
    {
        return ndi_send;
    }

    const NDIlib_v5 *getNDILib()
    {
        return p_NDILib;
    }

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NdiAudioProcessor)
    static std::mutex init_mutex;

    double sample_rate{};

    AudioProcessorValueTreeState apvts;

    const NDIlib_v5 *p_NDILib = nullptr;

    NDIlib_find_instance_t ndi_find = nullptr;
    NDIlib_framesync_instance_t ndi_framesync = nullptr;
    NDIlib_recv_instance_t ndi_recv = nullptr;
    NDIlib_send_instance_t ndi_send = nullptr;

    NDIlib_audio_frame_v2_t recv_audio_frame{};
    NDIlib_audio_frame_v2_t send_audio_frame{};
    NDIlib_find_create_t ndi_find_create{};
    NDIlib_recv_create_v3_t ndi_recv_create{};
    NDIlib_send_create_t ndi_send_create{};

    std::vector<float> recv_buf{};
    std::vector<float> send_buf{};

    String ndi_recv_name{};
    String ndi_send_name{};

    String recv_text_input{};
    String send_text_input{};

    StringArray groups{};
    Array<int> recv_channels{};
    // std::vector<int> recv_channels {};

    bool is_standalone{false};

    bool send_ok{false};
    bool recv_ok{false};

    SpinLock parameter_lock{};
    SpinLock audio_lock{};
    std::mutex text_mutex;

#if _WIN32
    HMODULE hNDILib;
#else
    void *hNDILib;
#endif
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        juce::AudioProcessorValueTreeState::ParameterLayout params;

        params.add(std::make_unique<juce::AudioParameterBool>(
            ParameterID{"recv", 1}, // parameterID, AU versionhint
            "recv",                 // parameter name
            false));                // default value
        params.add(std::make_unique<juce::AudioParameterBool>(
            ParameterID{"send", 1}, // parameterID, AU versionhint
            "send",                 // parameter name
            false));                // default value
        params.add(std::make_unique<juce::AudioParameterFloat>(
            ParameterID{"ndi_recv", 1}, // parameterID, AU versionhint
            "ndi_recv",                 // parameter name
            0.0f, 1.0f, 0.5f));         // default value

        return params;
    }

    NDIlib_metadata_frame_t ndi_metadata;
    std::string metadata_string{};
};
