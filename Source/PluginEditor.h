#pragma once
#include "PluginProcessor.h"

class VocalChainEditor : public juce::AudioProcessorEditor
{
public:
    explicit VocalChainEditor (VocalChainProcessor&);
    ~VocalChainEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // Ein Regler / Dropdown / Schalter, automatisch passend zum Parameter-Typ
    class Control : public juce::Component
    {
    public:
        Control (juce::AudioProcessorValueTreeState& apvts, const juce::String& id);
        void resized() override;
    private:
        using APVTS = juce::AudioProcessorValueTreeState;
        juce::Label label;
        juce::Slider slider;
        juce::ComboBox combo;
        juce::ToggleButton toggle;
        std::unique_ptr<APVTS::SliderAttachment> sliderAtt;
        std::unique_ptr<APVTS::ComboBoxAttachment> comboAtt;
        std::unique_ptr<APVTS::ButtonAttachment> buttonAtt;
    };

    struct Section
    {
        juce::String title;
        juce::OwnedArray<Control> controls;
    };

    juce::Rectangle<int> sectionBounds (int index) const;

    juce::LookAndFeel_V4 laf;
    juce::OwnedArray<Section> sections;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalChainEditor)
};
