#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

AudioProcessorValueTreeState::ParameterLayout VocalChainProcessor::createLayout()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto F = [&] (const char* id, const char* name, float lo, float hi, float def, const char* unit, float skew = 1.0f)
    {
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name,
                        NormalisableRange<float> (lo, hi, 0.01f, skew), def,
                        AudioParameterFloatAttributes().withLabel (unit)));
    };
    auto B = [&] (const char* id, const char* name, bool def)
    {
        p.push_back (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def));
    };
    auto C = [&] (const char* id, const char* name, StringArray items, int def)
    {
        p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, items, def));
    };

    // Autotune
    B ("pitch_on", "Autotune", true);
    C ("pitch_key", "Key", { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 0);
    C ("pitch_scale", "Scale", { "Chromatic", "Major", "Minor" }, 0);
    F ("pitch_speed", "Retune", 0.0f, 400.0f, 8.0f, "ms", 0.4f);
    F ("pitch_amount", "Amount", 0.0f, 100.0f, 100.0f, "%");
    // EQ 1 (Cleanup)
    B ("eq1_on", "EQ 1", true);
    F ("eq1_hpf", "Low Cut", 20.0f, 500.0f, 100.0f, "Hz", 0.4f);
    F ("eq1_mudf", "Mud Freq", 100.0f, 800.0f, 300.0f, "Hz", 0.5f);
    F ("eq1_mudg", "Mud Cut", -12.0f, 6.0f, -3.0f, "dB");
    // De-Esser
    B ("ds_on", "De-Esser", true);
    F ("ds_freq", "Freq", 3000.0f, 12000.0f, 6500.0f, "Hz", 0.5f);
    F ("ds_thresh", "Thresh", -60.0f, 0.0f, -30.0f, "dB");
    // Compressor
    B ("cp_on", "Compressor", true);
    F ("cp_thresh", "Thresh", -50.0f, 0.0f, -18.0f, "dB");
    F ("cp_ratio", "Ratio", 1.0f, 20.0f, 4.0f, ":1", 0.5f);
    F ("cp_attack", "Attack", 0.1f, 100.0f, 5.0f, "ms", 0.4f);
    F ("cp_release", "Release", 10.0f, 1000.0f, 80.0f, "ms", 0.4f);
    F ("cp_makeup", "Makeup", 0.0f, 24.0f, 3.0f, "dB");
    // Distortion
    B ("sat_on", "Distortion", true);
    C ("sat_type", "Type", { "Soft", "Hard", "Fold" }, 0);
    F ("sat_drive", "Drive", 0.0f, 36.0f, 6.0f, "dB");
    F ("sat_mix", "Mix", 0.0f, 100.0f, 60.0f, "%");
    // EQ 2 (Sound)
    B ("eq2_on", "EQ 2", true);
    F ("eq2_low", "Low Shelf", -12.0f, 12.0f, 0.0f, "dB");
    F ("eq2_presf", "Pres Freq", 1000.0f, 9000.0f, 4000.0f, "Hz", 0.5f);
    F ("eq2_pres", "Presence", -12.0f, 12.0f, 2.0f, "dB");
    F ("eq2_air", "Air", -12.0f, 12.0f, 3.0f, "dB");
    // Delay
    B ("dl_on", "Delay", true);
    F ("dl_time", "Time", 20.0f, 1000.0f, 300.0f, "ms", 0.5f);
    F ("dl_fb", "Feedback", 0.0f, 95.0f, 30.0f, "%");
    F ("dl_low", "Low Cut", 20.0f, 1000.0f, 200.0f, "Hz", 0.4f);
    F ("dl_high", "High Cut", 1000.0f, 20000.0f, 5000.0f, "Hz", 0.4f);
    F ("dl_mix", "Mix", 0.0f, 100.0f, 15.0f, "%");
    // Reverb
    B ("rv_on", "Reverb", true);
    F ("rv_size", "Size", 0.0f, 100.0f, 40.0f, "%");
    F ("rv_damp", "Damping", 0.0f, 100.0f, 50.0f, "%");
    F ("rv_width", "Width", 0.0f, 100.0f, 100.0f, "%");
    F ("rv_mix", "Mix", 0.0f, 100.0f, 18.0f, "%");
    // Output
    B ("out_limiter", "Limiter", true);
    F ("out_gain", "Output", -24.0f, 12.0f, 0.0f, "dB");

    return { p.begin(), p.end() };
}

VocalChainProcessor::VocalChainProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    auto g = [this] (const char* id) { return apvts.getRawParameterValue (id); };
    prm = { g ("pitch_on"), g ("pitch_key"), g ("pitch_scale"), g ("pitch_speed"), g ("pitch_amount"),
            g ("eq1_on"), g ("eq1_hpf"), g ("eq1_mudf"), g ("eq1_mudg"),
            g ("ds_on"), g ("ds_freq"), g ("ds_thresh"),
            g ("cp_on"), g ("cp_thresh"), g ("cp_ratio"), g ("cp_attack"), g ("cp_release"), g ("cp_makeup"),
            g ("sat_on"), g ("sat_type"), g ("sat_drive"), g ("sat_mix"),
            g ("eq2_on"), g ("eq2_low"), g ("eq2_presf"), g ("eq2_pres"), g ("eq2_air"),
            g ("dl_on"), g ("dl_time"), g ("dl_fb"), g ("dl_low"), g ("dl_high"), g ("dl_mix"),
            g ("rv_on"), g ("rv_size"), g ("rv_damp"), g ("rv_width"), g ("rv_mix"),
            g ("out_limiter"), g ("out_gain") };
}

bool VocalChainProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    auto out = l.getMainOutputChannelSet();
    return (out == AudioChannelSet::mono() || out == AudioChannelSet::stereo())
           && out == l.getMainInputChannelSet();
}

void VocalChainProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    dsp::ProcessSpec spec { sampleRate, (uint32) samplesPerBlock, 2 };

    pitch.prepare (sampleRate);
    for (auto* d : { &eq1Hpf, &eq1Mud, &eq2Low, &eq2Pres, &eq2Air }) { d->prepare (spec); d->reset(); }
    deesser.prepare (sampleRate);
    comp.prepare (spec);
    delay.prepare (spec);
    reverb.setSampleRate (sampleRate);
    reverb.reset();
    wetBuf.setSize (2, jmax (1, samplesPerBlock));
    for (auto& f : revHp) f.reset();
    limiter.prepare (spec);
    limiter.setThreshold (-0.3f);
    limiter.setRelease (80.0f);
    lastGain = Decibels::decibelsToGain (prm.outGain->load());
    for (int c = 0; c < 2; ++c) dcX[c] = dcY[c] = 0.0f;
}

void VocalChainProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int nch = jmin (2, buffer.getNumChannels());
    if (n == 0 || nch == 0) return;

    auto on = [] (std::atomic<float>* a) { return a->load() > 0.5f; };
    auto fc = [this] (float f) { return jlimit (10.0f, (float) sr * 0.45f, f); };

    float* L = buffer.getWritePointer (0);
    float* R = nch > 1 ? buffer.getWritePointer (1) : nullptr;

    // 1) Autotune
    if (on (prm.pitchOn))
    {
        pitch.setParams ((int) prm.key->load(), (int) prm.scale->load(),
                         prm.speed->load(), prm.amount->load() * 0.01f);
        pitch.process (L, R, n);
    }

    dsp::AudioBlock<float> block (buffer);
    auto sub = block.getSubsetChannelRange (0, (size_t) nch);
    dsp::ProcessContextReplacing<float> ctx (sub);

    // 2) EQ 1 (Cleanup)
    if (on (prm.eq1On))
    {
        *eq1Hpf.state = *Coeffs::makeHighPass (sr, fc (prm.eq1Hpf->load()), 0.707f);
        *eq1Mud.state = *Coeffs::makePeakFilter (sr, fc (prm.eq1MudF->load()), 1.0f,
                                                 Decibels::decibelsToGain (prm.eq1MudG->load()));
        eq1Hpf.process (ctx);
        eq1Mud.process (ctx);
    }

    // 3) De-Esser
    if (on (prm.dsOn))
        deesser.process (L, R, n, fc (prm.dsFreq->load()), prm.dsThresh->load());

    // 4) Kompressor
    if (on (prm.cpOn))
    {
        comp.setThreshold (prm.cpThresh->load());
        comp.setRatio (prm.cpRatio->load());
        comp.setAttack (prm.cpAtk->load());
        comp.setRelease (prm.cpRel->load());
        comp.process (ctx);
        sub.multiplyBy (Decibels::decibelsToGain (prm.cpMake->load()));
    }

    // 5) Distortion
    if (on (prm.satOn))
    {
        const int type = (int) prm.satType->load();
        const float driveLin = Decibels::decibelsToGain (prm.satDrive->load());
        const float comp_ = 1.0f / std::sqrt (driveLin);
        const float mix = prm.satMix->load() * 0.01f;
        for (int c = 0; c < nch; ++c)
        {
            float* d = buffer.getWritePointer (c);
            for (int i = 0; i < n; ++i)
            {
                const float x = d[i];
                float y = x * driveLin;
                if (type == 0)      y = std::tanh (y + 0.12f) - 0.11942f;
                else if (type == 1) y = jlimit (-1.0f, 1.0f, y);
                else                y = std::sin (y);
                y *= comp_;
                const float o = y - dcX[c] + 0.995f * dcY[c]; // DC-Blocker
                dcX[c] = y;
                dcY[c] = o;
                d[i] = x + (o - x) * mix;
            }
        }
    }

    // 6) EQ 2 (Sound)
    if (on (prm.eq2On))
    {
        *eq2Low.state  = *Coeffs::makeLowShelf  (sr, 120.0f, 0.707f, Decibels::decibelsToGain (prm.eq2Low->load()));
        *eq2Pres.state = *Coeffs::makePeakFilter (sr, fc (prm.eq2PresF->load()), 0.9f, Decibels::decibelsToGain (prm.eq2Pres->load()));
        *eq2Air.state  = *Coeffs::makeHighShelf (sr, fc (10000.0f), 0.707f, Decibels::decibelsToGain (prm.eq2Air->load()));
        eq2Low.process (ctx);
        eq2Pres.process (ctx);
        eq2Air.process (ctx);
    }

    // 7) Delay
    if (on (prm.dlOn))
        delay.process (buffer, nch, n, prm.dlTime->load(), prm.dlFb->load() * 0.01f,
                       fc (prm.dlLow->load()), fc (prm.dlHigh->load()), prm.dlMix->load() * 0.01f);

    // 8) Reverb
    if (on (prm.rvOn))
    {
        if (wetBuf.getNumSamples() < n) wetBuf.setSize (2, n, false, false, true);
        for (int c = 0; c < nch; ++c) wetBuf.copyFrom (c, 0, buffer, c, 0, n);

        auto hpC = Coeffs::makeHighPass (sr, 250.0f, 0.707f);
        for (int c = 0; c < nch; ++c)
        {
            *revHp[c].coefficients = *hpC;
            float* w = wetBuf.getWritePointer (c);
            for (int i = 0; i < n; ++i) w[i] = revHp[c].processSample (w[i]);
        }

        Reverb::Parameters rp;
        rp.roomSize = prm.rvSize->load() * 0.01f;
        rp.damping  = prm.rvDamp->load() * 0.01f;
        rp.width    = prm.rvWidth->load() * 0.01f;
        rp.wetLevel = 0.33f;
        rp.dryLevel = 0.0f;
        rp.freezeMode = 0.0f;
        reverb.setParameters (rp);

        if (nch == 2) reverb.processStereo (wetBuf.getWritePointer (0), wetBuf.getWritePointer (1), n);
        else          reverb.processMono (wetBuf.getWritePointer (0), n);

        const float mix = prm.rvMix->load() * 0.01f;
        for (int c = 0; c < nch; ++c) buffer.addFrom (c, 0, wetBuf, c, 0, n, mix);
    }

    // 9) Output + Limiter
    const float g = Decibels::decibelsToGain (prm.outGain->load());
    buffer.applyGainRamp (0, n, lastGain, g);
    lastGain = g;
    if (on (prm.limOn)) limiter.process (ctx);
}

AudioProcessorEditor* VocalChainProcessor::createEditor() { return new VocalChainEditor (*this); }

void VocalChainProcessor::getStateInformation (MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void VocalChainProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (ValueTree::fromXml (*xml));
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new VocalChainProcessor(); }
