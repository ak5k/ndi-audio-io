/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 7.0.7

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library.
  Copyright (c) 2020 - Raw Material Software Limited.

  ==============================================================================
*/

//[Headers] You can add your own extra header files here...
//[/Headers]

#include "PluginEditor.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
NdiAudioProcessorEditor::NdiAudioProcessorEditor (NdiAudioProcessor& p)
    : AudioProcessorEditor (&p),
      ap (p)
{
    //[Constructor_pre] You can add your own custom stuff here..
    LookAndFeel::setDefaultLookAndFeel(&lf);
    lf.setColourScheme(LookAndFeel_V4::getMidnightColourScheme());
    //[/Constructor_pre]

    combobox_sources.reset (new juce::ComboBox ("ndi_sources"));
    addAndMakeVisible (combobox_sources.get());
    combobox_sources->setEditableText (true);
    combobox_sources->setJustificationType (juce::Justification::centredLeft);
    combobox_sources->setTextWhenNothingSelected (TRANS("no source"));
    combobox_sources->setTextWhenNoChoicesAvailable (TRANS("no sources"));
    combobox_sources->addListener (this);

    juce__textEditor.reset (new juce::TextEditor ("new text editor"));
    addAndMakeVisible (juce__textEditor.get());
    juce__textEditor->setMultiLine (false);
    juce__textEditor->setReturnKeyStartsNewLine (false);
    juce__textEditor->setReadOnly (false);
    juce__textEditor->setScrollbarsShown (true);
    juce__textEditor->setCaretVisible (true);
    juce__textEditor->setPopupMenuEnabled (true);
    juce__textEditor->setText (juce::String());

    juce__sendButton.reset (new juce::TextButton ("send"));
    addAndMakeVisible (juce__sendButton.get());
    juce__sendButton->addListener (this);
    juce__sendButton->setColour (juce::TextButton::buttonColourId, juce::Colour (0xff0e0e0e));
    juce__sendButton->setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff6257ff));

    juce__recvButton.reset (new juce::TextButton ("recv"));
    addAndMakeVisible (juce__recvButton.get());
    juce__recvButton->addListener (this);
    juce__recvButton->setColour (juce::TextButton::buttonColourId, juce::Colour (0xff0e0e0e));
    juce__recvButton->setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff6257ff));

    drawable1 = juce::Drawable::createFromImageData (ndiLogoMasterLight_svg, ndiLogoMasterLight_svgSize);

    //[UserPreSize]
    juce__textEditor->setText(ap.getNDISendTextInput());
    juce__textEditor->addListener(this);
    juce__textEditor->applyFontToAllText(lf.getPopupMenuFont());
    juce__textEditor->setJustification(Justification::verticallyCentred);

    juce__sendButton->setToggleable(true);
    juce__sendButton->setClickingTogglesState(true);

    juce__recvButton->setToggleable(true);
    juce__recvButton->setClickingTogglesState(true);

    rxAttachment.reset(
        new ButtonAttachment(ap.getAPVTS(), "recv", *juce__recvButton));
    txAttachment.reset(
        new ButtonAttachment(ap.getAPVTS(), "send", *juce__sendButton));
    comboboxAttachment.reset(
        new ComboBoxAttachment(ap.getAPVTS(), "ndi_recv", *combobox_sources));

    //[/UserPreSize]

    setSize (640, 480);


    //[Constructor] You can add your own custom stuff here..
    timerCallback();
    startTimer(1000);

    setResizable(true, false);
    setResizeLimits(240, 240, INT16_MAX, INT16_MAX);

    // if (juce::JUCEApplicationBase::isStandaloneApp())
    // {
    //     auto pluginHolder = juce::StandalonePluginHolder::getInstance();

    //     if (pluginHolder)
    //     {
    // pluginHolder->getMuteInputValue().setValue(false);
    // pluginHolder->processorHasPotentialFeedbackLoop = false;
    // pluginHolder->muteInput.store(false);
    //     }
    // }
    //[/Constructor]
}

NdiAudioProcessorEditor::~NdiAudioProcessorEditor()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    rxAttachment = nullptr;
    txAttachment = nullptr;
    comboboxAttachment = nullptr;
    //[/Destructor_pre]

    combobox_sources = nullptr;
    juce__textEditor = nullptr;
    juce__sendButton = nullptr;
    juce__recvButton = nullptr;
    drawable1 = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    // LookAndFeel::setDefaultLookAndFeel(nullptr);
    //[/Destructor]
}

//==============================================================================
void NdiAudioProcessorEditor::paint (juce::Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (juce::Colour (0xff0e0e0e));

    {
        int x = proportionOfWidth (0.2703f), y = proportionOfHeight (0.1979f), width = proportionOfWidth (0.2000f), height = proportionOfHeight (0.1167f);
        //[UserPaintCustomArguments] Customize the painting arguments here..
        //[/UserPaintCustomArguments]
        g.setColour (juce::Colours::black);
        jassert (drawable1 != nullptr);
        if (drawable1 != nullptr)
            drawable1->drawWithin (g, juce::Rectangle<int> (x, y, width, height).toFloat(),
                                   juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize, 1.000f);
    }

    {
        int x = proportionOfWidth (0.6078f), y = proportionOfHeight (0.1646f), width = proportionOfWidth (0.3422f), height = proportionOfHeight (0.1313f);
        juce::String text (TRANS("Audio IO"));
        juce::Colour fillColour = juce::Colour (0xff6257ff);
        //[UserPaintCustomArguments] Customize the painting arguments here..
        g.setFont(
            juce::Font("Arial",
                       static_cast<float>(jmin(proportionOfWidth(0.09375f),
                                               proportionOfHeight(0.125f))),
                       juce::Font::plain)
                .withTypefaceStyle("Black"));
        g.setColour(fillColour);
        g.drawText(text, x, y, width, height, juce::Justification::centred,
                   true);
    #if 0
        //[/UserPaintCustomArguments]
        g.setColour (fillColour);
        g.setFont (juce::Font ("Arial", 57.40f, juce::Font::plain).withTypefaceStyle ("Black"));
        g.drawText (text, x, y, width, height,
                    juce::Justification::centred, true);
    }

    //[UserPaint] Add your own custom painting code here..
#endif
    }
    //[/UserPaint]
}

void NdiAudioProcessorEditor::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    combobox_sources->setBounds (proportionOfWidth (0.1375f), proportionOfHeight (0.7063f), proportionOfWidth (0.4500f), proportionOfHeight (0.0917f));
    juce__textEditor->setBounds (proportionOfWidth (0.1375f), proportionOfHeight (0.4354f), proportionOfWidth (0.4500f), proportionOfHeight (0.0896f));
    juce__sendButton->setBounds (proportionOfWidth (0.6750f), proportionOfHeight (0.4354f), proportionOfWidth (0.2141f), proportionOfHeight (0.0917f));
    juce__recvButton->setBounds (proportionOfWidth (0.6750f), proportionOfHeight (0.7063f), proportionOfWidth (0.2141f), proportionOfHeight (0.0917f));
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void NdiAudioProcessorEditor::comboBoxChanged (juce::ComboBox* comboBoxThatHasChanged)
{
    //[UsercomboBoxChanged_Pre]
    std::scoped_lock lk(editor_mutex);
    //[/UsercomboBoxChanged_Pre]

    if (comboBoxThatHasChanged == combobox_sources.get())
    {
        //[UserComboBoxCode_combobox_sources] -- add your combo box handling code here..
        //code here..
        // code here..
        if (timer_update == true)
        {
            return;
        }

        auto text = combobox_sources->getText();

        if (text.isEmpty() || text == prev_text)
            return;

        prev_text = text;

        ap.parseRecvTextInput(text);
        //[/UserComboBoxCode_combobox_sources]
    }

    //[UsercomboBoxChanged_Post]
    //[/UsercomboBoxChanged_Post]
}

void NdiAudioProcessorEditor::buttonClicked (juce::Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    std::scoped_lock lk(editor_mutex);
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == juce__sendButton.get())
    {
        //[UserButtonCode_juce__sendButton] -- add your button handler code here..
        //here..
        // here..
        //[/UserButtonCode_juce__sendButton]
    }
    else if (buttonThatWasClicked == juce__recvButton.get())
    {
        //[UserButtonCode_juce__recvButton] -- add your button handler code here..
        //here..
        // here..
        //[/UserButtonCode_juce__recvButton]
    }

    //[UserbuttonClicked_Post]
    //[/UserbuttonClicked_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
//other code here...
// other code here...
// any
void NdiAudioProcessorEditor::textEditorReturnKeyPressed(TextEditor& editor)
{
    std::scoped_lock lk(editor_mutex);
    if (&editor != this->juce__textEditor.get())
        return;

    if (editor.getText().isNotEmpty() &&
        editor.getText() == ap.getNDISendTextInput())
        return;

    ap.parseSendTextInput(editor.getText());
    ap.parameterChanged("send",
                        ap.getAPVTS().getRawParameterValue("send")->load());
    editor.setText(ap.getNDISendTextInput());
}

void NdiAudioProcessorEditor::timerCallback()
{
    if (!ap.getNDILib())
        return;

    // actual NDI send name
    auto this_tx_name = ap.getNDISendName2();

    uint32_t no_sources;
    const NDIlib_source_t* p_sources =
        ap.getNDILib()->find_get_current_sources(ap.getNDIFind(), &no_sources);

    combobox_sources->clear();
    for (uint32_t i = 0; i < no_sources; i++)
    {
        String ndi_name = p_sources[i].p_ndi_name;
        if (ndi_name != this_tx_name)
            combobox_sources->addItem(p_sources[i].p_ndi_name, (int)i + 1);
    }

    for (auto i = 0; i < combobox_sources->getNumItems(); i++)
    {
        auto combobox_item_text = combobox_sources->getItemText(i);
        auto ndi_recv_name = ap.getNDIRecvName();
        if (combobox_item_text == ndi_recv_name)
        {
            timer_update = true;
            combobox_sources->setText(
                ap.getNDIRecvTextInput(),
                juce::NotificationType::sendNotificationSync);
            timer_update = false;
            break;
        }
    }
    combobox_sources->setTextWhenNothingSelected(
        ap.getNDIRecvName().isNotEmpty() ? ap.getNDIRecvTextInput()
                                         : "no source");
}
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="NdiAudioProcessorEditor"
                 componentName="" parentClasses="public juce::AudioProcessorEditor, private juce::Timer, public juce::TextEditor::Listener"
                 constructorParams="NdiAudioProcessor&amp; p" variableInitialisers="AudioProcessorEditor (&amp;p)&#10;ap (p)&#10;"
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="1" initialWidth="640" initialHeight="480">
  <BACKGROUND backgroundColour="ff0e0e0e">
    <IMAGE pos="27.031% 19.792% 20% 11.667%" resource="ndiLogoMasterLight_svg"
           opacity="1.0" mode="2"/>
    <TEXT pos="60.781% 16.458% 34.219% 13.125%" fill="solid: ff6257ff"
          hasStroke="0" text="Audio IO" fontname="Arial" fontsize="57.4"
          kerning="0.0" bold="0" italic="0" justification="36" typefaceStyle="Black"/>
  </BACKGROUND>
  <COMBOBOX name="ndi_sources" id="8a9f4410678088ea" memberName="combobox_sources"
            virtualName="" explicitFocusOrder="0" pos="13.75% 70.625% 45% 9.167%"
            editable="1" layout="33" items="" textWhenNonSelected="no source"
            textWhenNoItems="no sources"/>
  <TEXTEDITOR name="new text editor" id="c6b28ec6d037d807" memberName="juce__textEditor"
              virtualName="" explicitFocusOrder="0" pos="13.75% 43.542% 45% 8.958%"
              initialText="" multiline="0" retKeyStartsLine="0" readonly="0"
              scrollbars="1" caret="1" popupmenu="1"/>
  <TEXTBUTTON name="send" id="8ea332196abf1fb" memberName="juce__sendButton"
              virtualName="" explicitFocusOrder="0" pos="67.5% 43.542% 21.406% 9.167%"
              bgColOff="ff0e0e0e" bgColOn="ff6257ff" buttonText="send" connectedEdges="0"
              needsCallback="1" radioGroupId="0"/>
  <TEXTBUTTON name="recv" id="1fc1e6a5a8d1dd8f" memberName="juce__recvButton"
              virtualName="" explicitFocusOrder="0" pos="67.5% 70.625% 21.406% 9.167%"
              bgColOff="ff0e0e0e" bgColOn="ff6257ff" buttonText="recv" connectedEdges="0"
              needsCallback="1" radioGroupId="0"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif

//==============================================================================
// Binary resources - be careful not to edit any of these sections!

// JUCER_RESOURCE: ndiLogoMasterLight_svg, 1760, "../Source"
static const unsigned char resource_NdiAudioProcessorEditor_ndiLogoMasterLight_svg[] = { 60,115,118,103,32,119,105,100,116,104,61,34,49,48,52,55,34,32,104,101,105,103,104,116,61,34,53,53,48,34,32,118,
105,101,119,66,111,120,61,34,48,32,48,32,49,48,52,55,32,53,53,48,34,32,102,105,108,108,61,34,110,111,110,101,34,32,120,109,108,110,115,61,34,104,116,116,112,58,47,47,119,119,119,46,119,51,46,111,114,103,
47,50,48,48,48,47,115,118,103,34,62,10,60,112,97,116,104,32,100,61,34,77,50,48,52,46,52,49,52,32,49,51,54,46,49,52,49,86,52,49,51,46,51,50,72,50,55,50,46,52,56,49,86,50,57,54,46,50,48,55,67,50,55,50,46,
52,56,49,32,50,56,56,46,56,54,53,32,50,55,54,46,56,57,54,32,50,56,51,46,55,50,53,32,50,56,51,46,56,56,55,32,50,56,51,46,55,50,53,67,50,56,57,46,55,55,52,32,50,56,51,46,55,50,53,32,50,57,51,46,56,50,49,
32,50,56,55,46,48,50,57,32,51,48,48,46,52,52,52,32,50,57,55,46,51,48,57,76,51,55,51,46,54,54,51,32,52,49,51,46,51,50,72,52,52,55,46,57,56,53,86,49,51,54,46,49,52,49,72,51,55,57,46,57,49,56,86,50,53,52,
46,55,50,50,67,51,55,57,46,57,49,56,32,50,54,50,46,48,54,53,32,51,55,53,46,53,48,50,32,50,54,55,46,50,48,52,32,51,54,56,46,53,49,50,32,50,54,55,46,50,48,52,67,51,54,50,46,54,50,53,32,50,54,55,46,50,48,
52,32,51,53,56,46,53,55,55,32,50,54,51,46,57,32,51,53,49,46,57,53,53,32,50,53,51,46,54,50,49,76,50,55,56,46,55,51,54,32,49,51,54,46,49,52,49,72,50,48,52,46,52,49,52,90,34,32,102,105,108,108,61,34,35,70,
49,70,49,70,49,34,47,62,10,60,112,97,116,104,32,100,61,34,77,55,49,55,46,52,48,51,32,50,55,52,46,53,52,55,67,55,49,55,46,52,48,51,32,49,57,56,46,49,56,53,32,54,53,51,46,48,49,53,32,49,51,54,46,49,52,49,
32,53,55,52,46,50,55,55,32,49,51,54,46,49,52,49,72,52,54,52,46,54,51,51,86,52,49,51,46,51,50,72,53,55,52,46,50,55,55,67,54,53,51,46,51,56,51,32,52,49,51,46,51,50,32,55,49,55,46,52,48,51,32,51,53,48,46,
57,48,57,32,55,49,55,46,52,48,51,32,50,55,52,46,53,52,55,90,77,54,52,55,46,52,57,54,32,50,55,52,46,49,56,67,54,52,55,46,52,57,54,32,51,49,56,46,54,48,50,32,54,49,52,46,55,53,32,51,53,48,46,57,48,57,32,
53,54,57,46,56,54,50,32,51,53,48,46,57,48,57,72,53,52,49,46,53,57,54,67,53,51,54,46,54,56,51,32,51,53,48,46,57,48,57,32,53,51,50,46,55,48,49,32,51,52,54,46,57,51,53,32,53,51,50,46,55,48,49,32,51,52,50,
46,48,51,52,86,50,48,55,46,52,50,55,67,53,51,50,46,55,48,49,32,50,48,50,46,53,50,54,32,53,51,54,46,54,56,51,32,49,57,56,46,53,53,50,32,53,52,49,46,53,57,54,32,49,57,56,46,53,53,50,72,53,54,57,46,56,54,
50,67,54,49,52,46,55,53,32,49,57,56,46,53,53,50,32,54,52,55,46,52,57,54,32,50,51,48,46,56,53,57,32,54,52,55,46,52,57,54,32,50,55,52,46,49,56,90,34,32,102,105,108,108,61,34,35,70,49,70,49,70,49,34,47,62,
10,60,112,97,116,104,32,100,61,34,77,55,51,50,46,49,55,51,32,49,51,54,46,49,52,49,86,52,49,51,46,51,50,72,56,48,48,46,50,52,49,86,49,51,54,46,49,52,49,72,55,51,50,46,49,55,51,90,34,32,102,105,108,108,
61,34,35,70,49,70,49,70,49,34,47,62,10,60,112,97,116,104,32,100,61,34,77,56,50,56,46,50,52,49,32,51,56,54,46,49,57,53,67,56,50,48,46,51,54,54,32,51,56,54,46,49,57,53,32,56,49,52,46,50,52,49,32,51,57,50,
46,49,52,53,32,56,49,52,46,50,52,49,32,51,57,57,46,55,50,57,67,56,49,52,46,50,52,49,32,52,48,55,46,51,55,32,56,50,48,46,51,54,54,32,52,49,51,46,51,50,32,56,50,56,46,50,52,49,32,52,49,51,46,51,50,67,56,
51,54,46,49,49,54,32,52,49,51,46,51,50,32,56,52,50,46,50,52,49,32,52,48,55,46,51,55,32,56,52,50,46,50,52,49,32,51,57,57,46,55,50,57,67,56,52,50,46,50,52,49,32,51,57,50,46,49,52,53,32,56,51,54,46,49,49,
54,32,51,56,54,46,49,57,53,32,56,50,56,46,50,52,49,32,51,56,54,46,49,57,53,90,77,56,50,56,46,50,52,49,32,51,56,56,46,52,55,67,56,51,52,46,54,53,55,32,51,56,56,46,52,55,32,56,51,57,46,54,55,52,32,51,57,
51,46,51,55,32,56,51,57,46,54,55,52,32,51,57,57,46,55,50,57,67,56,51,57,46,54,55,52,32,52,48,54,46,48,56,55,32,56,51,52,46,54,53,55,32,52,49,49,46,48,52,53,32,56,50,56,46,50,52,49,32,52,49,49,46,48,52,
53,67,56,50,49,46,56,50,52,32,52,49,49,46,48,52,53,32,56,49,54,46,56,48,55,32,52,48,54,46,48,56,55,32,56,49,54,46,56,48,55,32,51,57,57,46,55,50,57,67,56,49,54,46,56,48,55,32,51,57,51,46,51,55,32,56,50,
49,46,56,50,52,32,51,56,56,46,52,55,32,56,50,56,46,50,52,49,32,51,56,56,46,52,55,90,77,56,51,51,46,56,57,57,32,51,57,54,46,56,49,50,67,56,51,51,46,56,57,57,32,51,57,52,46,51,48,52,32,56,51,49,46,55,57,
57,32,51,57,50,46,52,57,53,32,56,50,56,46,56,56,50,32,51,57,50,46,52,57,53,72,56,50,51,46,54,51,50,86,52,48,55,46,48,50,72,56,50,54,46,50,53,55,86,52,48,49,46,49,50,57,76,56,51,48,46,57,50,52,32,52,48,
55,46,48,50,72,56,51,51,46,57,53,55,76,56,51,48,46,50,56,50,32,52,48,50,46,52,55,67,56,51,48,46,48,52,57,32,52,48,50,46,49,55,57,32,56,50,57,46,57,51,50,32,52,48,49,46,56,50,57,32,56,50,57,46,57,51,50,
32,52,48,49,46,53,51,55,67,56,50,57,46,57,51,50,32,52,48,49,46,48,55,32,56,51,48,46,49,54,54,32,52,48,48,46,55,50,32,56,51,48,46,54,57,49,32,52,48,48,46,54,48,52,72,56,51,48,46,55,52,57,67,56,51,50,46,
54,55,52,32,52,48,48,46,49,51,55,32,56,51,51,46,56,57,57,32,51,57,56,46,54,55,57,32,56,51,51,46,56,57,57,32,51,57,54,46,56,49,50,90,77,56,51,49,46,49,53,55,32,51,57,54,46,57,50,57,67,56,51,49,46,49,53,
55,32,51,57,56,46,50,49,50,32,56,51,48,46,50,50,52,32,51,57,56,46,57,55,32,56,50,56,46,55,54,54,32,51,57,56,46,57,55,72,56,50,54,46,50,53,55,86,51,57,52,46,56,50,57,72,56,50,56,46,55,54,54,67,56,51,48,
46,50,50,52,32,51,57,52,46,56,50,57,32,56,51,49,46,49,53,55,32,51,57,53,46,54,52,53,32,56,51,49,46,49,53,55,32,51,57,54,46,57,50,57,90,34,32,102,105,108,108,61,34,35,70,49,70,49,70,49,34,47,62,10,60,47,
115,118,103,62,10,0,0};

const char* NdiAudioProcessorEditor::ndiLogoMasterLight_svg = (const char*) resource_NdiAudioProcessorEditor_ndiLogoMasterLight_svg;
const int NdiAudioProcessorEditor::ndiLogoMasterLight_svgSize = 1760;


//[EndFile] You can add extra defines here...
//[/EndFile]

