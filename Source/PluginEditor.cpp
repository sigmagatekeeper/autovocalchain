#include "PluginEditor.h"

using namespace juce;

static constexpr int cellW = 95, cellH = 92, headerH = 20, gap = 8, margin = 12, titleH = 38;
static constexpr int cellsPerSection = 6;
static constexpr int secW = cellW * cellsPerSection, secH = cellH + headerH;
static const Colour accent (0xffb44cff);

VocalChainEditor::Control::Control (AudioProcessorValueTreeState& apvts, const String& id)
{
    auto* prm = apvts.getParameter (id);
    label.setText (prm->getName (24), dontSendNotification);
    label.setJustificationType (Justification::centred);
    label.setFont (Font (FontOptions (12.0f)));
    addAndMakeVisible (label);

    if (auto* c = dynamic_cast<AudioParameterChoice*> (prm))
    {
        combo.addItemList (c->choices, 1);
        addAndMakeVisible (combo);
        comboAtt = std::make_unique<APVTS::ComboBoxAttachment> (apvts, id, combo);
    }
    else if (dynamic_cast<AudioParameterBool*> (prm) != nullptr)
    {
        addAndMakeVisible (toggle);
        buttonAtt = std::make_unique<APVTS::ButtonAttachment> (apvts, id, toggle);
    }
    else
    {
        slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (Slider::TextBoxBelow, false, 80, 16);
        slider.setTextValueSuffix (" " + prm->getLabel());
        addAndMakeVisible (slider);
        sliderAtt = std::make_unique<APVTS::SliderAttachment> (apvts, id, slider);
    }
}

void VocalChainEditor::Control::resized()
{
    auto r = getLocalBounds().reduced (2);
    label.setBounds (r.removeFromTop (16));
    slider.setBounds (r);
    combo.setBounds (r.withSizeKeepingCentre (r.getWidth() - 10, 26));
    toggle.setBounds (r.withSizeKeepingCentre (30, 30));
}

VocalChainEditor::VocalChainEditor (VocalChainProcessor& p) : AudioProcessorEditor (&p)
{
    laf.setColourScheme (LookAndFeel_V4::getMidnightColourScheme());
    laf.setColour (Slider::rotarySliderFillColourId, accent);
    laf.setColour (Slider::thumbColourId, Colours::white);
    laf.setColour (ToggleButton::tickColourId, accent);
    setLookAndFeel (&laf);

    struct Def { const char* title; std::vector<const char*> ids; };
    const std::vector<Def> defs = {
        { "AUTOTUNE",   { "pitch_on", "pitch_key", "pitch_scale", "pitch_speed", "pitch_amount" } },
        { "EQ 1  (CLEANUP)", { "eq1_on", "eq1_hpf", "eq1_mudf", "eq1_mudg" } },
        { "DE-ESSER",   { "ds_on", "ds_freq", "ds_thresh" } },
        { "COMPRESSOR", { "cp_on", "cp_thresh", "cp_ratio", "cp_attack", "cp_release", "cp_makeup" } },
        { "DISTORTION", { "sat_on", "sat_type", "sat_drive", "sat_mix" } },
        { "EQ 2  (SOUND)", { "eq2_on", "eq2_low", "eq2_presf", "eq2_pres", "eq2_air" } },
        { "DELAY",      { "dl_on", "dl_time", "dl_fb", "dl_low", "dl_high", "dl_mix" } },
        { "REVERB",     { "rv_on", "rv_size", "rv_damp", "rv_width", "rv_mix" } },
        { "OUTPUT",     { "out_limiter", "out_gain" } },
    };

    for (auto& d : defs)
    {
        auto* s = sections.add (new Section());
        s->title = d.title;
        for (auto* id : d.ids)
            addAndMakeVisible (s->controls.add (new Control (p.apvts, id)));
    }

    setSize (margin * 2 + secW * 2 + gap, titleH + 5 * (secH + gap) + margin);
}

VocalChainEditor::~VocalChainEditor() { setLookAndFeel (nullptr); }

Rectangle<int> VocalChainEditor::sectionBounds (int index) const
{
    const int col = index < 5 ? 0 : 1;
    const int row = index < 5 ? index : index - 5;
    return { margin + col * (secW + gap), titleH + row * (secH + gap), secW, secH };
}

void VocalChainEditor::paint (Graphics& g)
{
    g.fillAll (Colour (0xff14141b));
    g.setColour (accent);
    g.setFont (Font (FontOptions (24.0f, Font::bold)));
    g.drawText ("VOCALCHAIN", margin, 4, 300, 30, Justification::centredLeft);

    for (int i = 0; i < sections.size(); ++i)
    {
        auto r = sectionBounds (i);
        g.setColour (Colour (0xff1f1f2b));
        g.fillRoundedRectangle (r.toFloat(), 8.0f);
        g.setColour (Colours::white.withAlpha (0.75f));
        g.setFont (Font (FontOptions (12.0f, Font::bold)));
        g.drawText (sections[i]->title, r.getX() + 10, r.getY() + 2, r.getWidth() - 20, headerH, Justification::centredLeft);
    }
}

void VocalChainEditor::resized()
{
    for (int i = 0; i < sections.size(); ++i)
    {
        auto r = sectionBounds (i);
        r.removeFromTop (headerH);
        for (auto* c : sections[i]->controls)
            c->setBounds (r.removeFromLeft (cellW));
    }
}
