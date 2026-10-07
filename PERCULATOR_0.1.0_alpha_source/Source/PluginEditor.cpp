#include "PluginEditor.h"

PERCULATORAudioProcessorEditor::PERCULATORAudioProcessorEditor (PERCULATORAudioProcessor& p)
 : AudioProcessorEditor (&p), processor (p)
{
    setSize (1120, 620); setResizable (true, true); setResizeLimits (900, 500, 1600, 900);
    struct Knob { juce::Slider* s; const char* id; const char* suffix; };
    Knob knobs[] {{&harmonics,"harmonics",""},{&balance,"balance",""},{&input,"input"," dB"},{&bias,"bias"," %"},
                  {&mix,"mix"," %"},{&output,"output"," dB"},{&irMix,"irMix"," %"},{&irLevel,"irLevel"," dB"}};
    for (auto& k : knobs) { configureKnob (*k.s, k.suffix); sliderAttachments.push_back (std::make_unique<SA> (p.apvts, k.id, *k.s)); }
    struct Btn { juce::ToggleButton* b; const char* id; };
    Btn buttons[] {{&studio,"studio"},{&irOn,"irOn"},{&phase,"phase"},{&bypass,"bypass"}};
    for (auto& x : buttons) { addAndMakeVisible (*x.b); buttonAttachments.push_back (std::make_unique<BA> (p.apvts, x.id, *x.b)); }
    addAndMakeVisible (loadIR); loadIR.onClick = [this] { chooseIR(); };
    addAndMakeVisible (irLabel); irLabel.setText (p.getIRName(), juce::dontSendNotification); irLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (colourBox); colourBox.addItem ("Dictatorship", 1); colourBox.addItem ("Surf Party", 2); colourBox.addItem ("Fetish Red", 3);
    colourBox.setSelectedId (p.getPanelColourIndex() + 1, juce::dontSendNotification);
    colourBox.onChange = [this] { processor.setPanelColourIndex (colourBox.getSelectedId() - 1); repaint(); };
}

void PERCULATORAudioProcessorEditor::configureKnob (juce::Slider& s, const juce::String& suffix)
{
    addAndMakeVisible (s); s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 74, 20); s.setTextValueSuffix (suffix);
}

void PERCULATORAudioProcessorEditor::paint (juce::Graphics& g)
{
    const juce::Colour panel[] { juce::Colour (0xff65753e), juce::Colour (0xff9dd9c1), juce::Colour (0xffb40812) };
    auto c = panel[processor.getPanelColourIndex()];
    g.fillAll (juce::Colour (0xff111211));
    auto r = getLocalBounds().toFloat().reduced (14.f);
    g.setColour (c); g.fillRoundedRectangle (r, 18.f);
    g.setColour (c.brighter (0.22f)); g.drawRoundedRectangle (r, 18.f, 3.f);
    g.setColour (c.getPerceivedBrightness() > 0.55f ? juce::Colours::black : juce::Colours::whitesmoke);
    g.setFont (juce::FontOptions (42.f, juce::Font::bold)); g.drawText ("P E R C U L A T O R", 48, 505, 600, 52, juce::Justification::centredLeft);
    g.setFont (juce::FontOptions (15.f, juce::Font::bold)); g.drawText ("DUAL POLARITY HARMONIC GENERATOR", 52, 554, 520, 24, juce::Justification::centredLeft);
    g.setFont (juce::FontOptions (16.f, juce::Font::bold));
    const char* names[] {"HARMONICS","BALANCE","INPUT","BIAS","MIX","OUTPUT","IR MIX","IR LEVEL"};
    juce::Rectangle<int> labels[] {{105,40,200,25},{390,40,200,25},{60,300,130,25},{205,300,130,25},{350,300,130,25},{495,300,130,25},{760,300,130,25},{910,300,130,25}};
    for (int i=0;i<8;++i) g.drawText (names[i], labels[i], juce::Justification::centred);
    g.drawText ("CABINET IR", 735, 42, 300, 25, juce::Justification::centred);
    g.drawText ("PANEL COLOR", 770, 515, 140, 24, juce::Justification::centredRight);
}

void PERCULATORAudioProcessorEditor::resized()
{
    harmonics.setBounds (105,70,200,210); balance.setBounds (390,70,200,210);
    input.setBounds (60,330,130,145); bias.setBounds (205,330,130,145); mix.setBounds (350,330,130,145); output.setBounds (495,330,130,145);
    irLabel.setBounds (730,80,315,42); loadIR.setBounds (935,132,110,34); irOn.setBounds (730,132,90,34); studio.setBounds (605,123,110,60);
    irMix.setBounds (760,330,130,145); irLevel.setBounds (910,330,130,145); phase.setBounds (835,205,120,32); bypass.setBounds (965,205,100,32);
    colourBox.setBounds (920,512,150,30);
}

bool PERCULATORAudioProcessorEditor::isInterestedInFileDrag (const juce::StringArray& files)
{ return files.size() == 1 && juce::File (files[0]).hasFileExtension ("wav;aif;aiff;flac"); }
void PERCULATORAudioProcessorEditor::filesDropped (const juce::StringArray& files, int, int)
{ if (files.size()) { processor.loadImpulseResponse (juce::File (files[0])); irLabel.setText (processor.getIRName(), juce::dontSendNotification); } }
void PERCULATORAudioProcessorEditor::chooseIR()
{
    chooser = std::make_unique<juce::FileChooser> ("Load cabinet impulse response", juce::File{}, "*.wav;*.aif;*.aiff;*.flac");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this] (const juce::FileChooser& fc) { auto f = fc.getResult(); if (f.existsAsFile()) { processor.loadImpulseResponse (f); irLabel.setText (processor.getIRName(), juce::dontSendNotification); } });
}
