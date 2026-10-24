#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "PitchCorrector.h"

using Filt   = juce::dsp::IIR::Filter<float>;
using Coeffs = juce::dsp::IIR::Coefficients<float>;
using Dup    = juce::dsp::ProcessorDuplicator<Filt, Coeffs>;

// ---------- De-Esser: zieht nur das Hochpass-Band runter, wenn es zu laut wird ----------
struct DeEsser
{
    Filt hp[2];
    float env = 0.0f, atk = 0.0f, rel = 0.0f;
    double sr = 44100.0;

    void prepare (double s)
    {
        sr = s;
        env = 0.0f;
        atk = std::exp (-1.0f / (0.0005f * (float) s));
        rel = std::exp (-1.0f / (0.04f * (float) s));
        hp[0].reset(); hp[1].reset();
    }

    void process (float* L, float* R, int n, float freq, float threshDb)
    {
        auto c = Coeffs::makeHighPass (sr, freq, 0.8f);
        *hp[0].coefficients = *c;
        *hp[1].coefficients = *c;
        for (int i = 0; i < n; ++i)
        {
            float hl = hp[0].processSample (L[i]);
            float hr = R ? hp[1].processSample (R[i]) : hl;
            float det = juce::jmax (std::abs (hl), std::abs (hr));
            env = det > env ? atk * env + (1.0f - atk) * det : rel * env + (1.0f - rel) * det;
            float db = juce::Decibels::gainToDecibels (env, -100.0f);
            float gr = db > threshDb ? juce::jmin (15.0f, (db - threshDb) * 0.75f) : 0.0f;
            float g = juce::Decibels::decibelsToGain (-gr);
            L[i] -= hl * (1.0f - g);
            if (R) R[i] -= hr * (1.0f - g);
        }
    }
};

// ---------- Delay mit Low-/High-Cut im Feedback ----------
struct DelayFx
{
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> line;
    Filt hp[2], lp[2];
    juce::SmoothedValue<float> time[2];
    double sr = 44100.0;

    void prepare (const juce::dsp::ProcessSpec& s)
    {
        sr = s.sampleRate;
        line.setMaximumDelayInSamples ((int) (s.sampleRate * 1.2) + 4);
        line.prepare (s);
        for (auto& t : time) { t.reset (s.sampleRate, 0.1); t.setCurrentAndTargetValue ((float) (0.3 * s.sampleRate)); }
        for (int c = 0; c < 2; ++c) { hp[c].reset(); lp[c].reset(); }
    }

    void process (juce::AudioBuffer<float>& buf, int nch, int n,
                  float timeMs, float fb, float lowHz, float highHz, float mix)
    {
        auto hpC = Coeffs::makeHighPass (sr, lowHz, 0.707f);
        auto lpC = Coeffs::makeLowPass  (sr, highHz, 0.707f);
        const float ds = juce::jmin ((float) (timeMs * 0.001 * sr), (float) (sr * 1.1));
        for (int c = 0; c < nch; ++c)
        {
            *hp[c].coefficients = *hpC;
            *lp[c].coefficients = *lpC;
            time[c].setTargetValue (ds);
            float* d = buf.getWritePointer (c);
            for (int i = 0; i < n; ++i)
            {
                float del = line.popSample (c, time[c].getNextValue());
                del = lp[c].processSample (hp[c].processSample (del));
                line.pushSample (c, d[i] + del * fb);
                d[i] += del * mix;
            }
        }
    }
};

class VocalChainProcessor : public juce::AudioProcessor
{
public:
    VocalChainProcessor();
    ~VocalChainProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "VocalChain"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts;

private:
    struct Prm
    {
        std::atomic<float> *pitchOn, *key, *scale, *speed, *amount,
            *eq1On, *eq1Hpf, *eq1MudF, *eq1MudG,
            *dsOn, *dsFreq, *dsThresh,
            *cpOn, *cpThresh, *cpRatio, *cpAtk, *cpRel, *cpMake,
            *satOn, *satType, *satDrive, *satMix,
            *eq2On, *eq2Low, *eq2PresF, *eq2Pres, *eq2Air,
            *dlOn, *dlTime, *dlFb, *dlLow, *dlHigh, *dlMix,
            *rvOn, *rvSize, *rvDamp, *rvWidth, *rvMix,
            *limOn, *outGain;
    } prm;

    double sr = 44100.0;
    PitchCorrector pitch;
    Dup eq1Hpf, eq1Mud, eq2Low, eq2Pres, eq2Air;
    DeEsser deesser;
    juce::dsp::Compressor<float> comp;
    float dcX[2] = { 0, 0 }, dcY[2] = { 0, 0 };
    DelayFx delay;
    juce::Reverb reverb;
    juce::AudioBuffer<float> wetBuf;
    Filt revHp[2];
    juce::dsp::Limiter<float> limiter;
    float lastGain = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalChainProcessor)
};
