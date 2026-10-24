#pragma once
#include <vector>
#include <cmath>
#include <algorithm>

// Echtzeit-Autotune: YIN-Pitch-Detection + Delay-Line-Pitch-Shifter.
// Kein JUCE noetig, reines C++.
class PitchCorrector
{
public:
    void prepare (double sr)
    {
        sampleRate = sr;
        hist.assign (histSize, 0.0f);
        buf[0].assign (bufSize, 0.0f);
        buf[1].assign (bufSize, 0.0f);
        histPos = writePos = 0;
        winSize = std::min ((float) (sr * 0.04), (float) (bufSize - 8));
        hop = sr >= 88200.0 ? 1024 : 512;
        tauMin = std::max (2, (int) (sr / 1000.0));
        tauMax = std::min ((int) (sr / 70.0), histSize - W - 4);
        scratch.assign ((size_t) (W + tauMax + 2), 0.0f);
        dn.assign ((size_t) (tauMax + 2), 1.0f);
        hopCounter = 0;
        phase = 0.5f;
        smoothedCents = targetCents = 0.0f;
        unvoiced = 100;
    }

    void setParams (int keyIn, int scaleIn, float speedMs, float amount01)
    {
        key = keyIn;
        scaleIdx = scaleIn;
        amount = amount01;
        coef = speedMs < 1.0f ? 1.0f : 1.0f - std::exp (-1.0f / (speedMs * 0.001f * (float) sampleRate));
    }

    // L und R werden direkt veraendert. R darf nullptr sein (mono).
    void process (float* L, float* R, int n)
    {
        const float twoPi = 6.2831853f;
        for (int i = 0; i < n; ++i)
        {
            float mono = R ? 0.5f * (L[i] + R[i]) : L[i];
            hist[(size_t) histPos] = mono;
            histPos = (histPos + 1) & histMask;
            if (++hopCounter >= hop) { hopCounter = 0; analyse(); }

            smoothedCents += (targetCents - smoothedCents) * coef;

            float ratio;
            if (unvoiced > 3 && std::abs (smoothedCents) < 5.0f)
            {
                // Stimmlos: Phase zum "sauberen" Punkt (nur 1 Tap aktiv) wandern lassen
                float err = 0.5f - phase;
                ratio = std::abs (err) < 0.002f ? 1.0f : (err > 0.0f ? 0.95f : 1.05f);
            }
            else
                ratio = std::exp2 (smoothedCents / 1200.0f);
            ratio = std::min (2.0f, std::max (0.5f, ratio));

            float phase2 = phase + 0.5f;
            if (phase2 >= 1.0f) phase2 -= 1.0f;
            float w1 = 0.5f - 0.5f * std::cos (twoPi * phase);
            float w2 = 0.5f - 0.5f * std::cos (twoPi * phase2);
            float d1 = phase * winSize, d2 = phase2 * winSize;

            buf[0][(size_t) writePos] = L[i];
            if (R) buf[1][(size_t) writePos] = R[i];

            L[i] = w1 * readBuf (0, d1) + w2 * readBuf (0, d2);
            if (R) R[i] = w1 * readBuf (1, d1) + w2 * readBuf (1, d2);

            writePos = (writePos + 1) & bufMask;

            phase += (1.0f - ratio) / winSize;
            while (phase >= 1.0f) phase -= 1.0f;
            while (phase < 0.0f)  phase += 1.0f;
        }
    }

    float getDetectedHz() const { return lastHz; }

private:
    static constexpr int histSize = 8192, histMask = histSize - 1;
    static constexpr int bufSize = 16384, bufMask = bufSize - 1;
    static constexpr int W = 1024;

    double sampleRate = 44100.0;
    std::vector<float> hist, buf[2], scratch, dn;
    int histPos = 0, writePos = 0, hop = 512, hopCounter = 0, tauMin = 44, tauMax = 600;
    float winSize = 1764.0f, phase = 0.5f;
    float smoothedCents = 0.0f, targetCents = 0.0f, coef = 1.0f, amount = 1.0f, lastHz = 0.0f;
    int key = 0, scaleIdx = 0, unvoiced = 100;

    float readBuf (int ch, float d) const
    {
        float pos = (float) writePos - d;
        if (pos < 0.0f) pos += (float) bufSize;
        int i0 = (int) pos;
        float fr = pos - (float) i0;
        const auto& b = buf[ch];
        return b[(size_t) (i0 & bufMask)] * (1.0f - fr) + b[(size_t) ((i0 + 1) & bufMask)] * fr;
    }

    bool allowed (int note) const
    {
        static const int masks[3] = { 0xFFF, 0xAB5, 0x5AD }; // Chromatic, Major, Minor
        int pc = (((note - key) % 12) + 12) % 12;
        return (masks[std::min (2, std::max (0, scaleIdx))] >> pc) & 1;
    }

    void analyse()
    {
        const int N = W + tauMax;
        int start = (histPos - N) & histMask;
        for (int k = 0; k < N; ++k)
            scratch[(size_t) k] = hist[(size_t) ((start + k) & histMask)];

        float f0 = yin();
        lastHz = f0;
        if (f0 > 0.0f)
        {
            unvoiced = 0;
            float midi = 69.0f + 12.0f * std::log2 (f0 / 440.0f);
            int base = (int) std::floor (midi);
            float best = 1e9f, bestNote = midi;
            for (int k = base - 6; k <= base + 7; ++k)
                if (allowed (k))
                {
                    float dd = std::abs ((float) k - midi);
                    if (dd < best) { best = dd; bestNote = (float) k; }
                }
            targetCents = std::min (700.0f, std::max (-700.0f, (bestNote - midi) * 100.0f * amount));
        }
        else
        {
            if (unvoiced < 1000) ++unvoiced;
            if (unvoiced > 3) targetCents = 0.0f;
        }
    }

    float yin()
    {
        float e = 0.0f;
        for (int j = 0; j < W; ++j) e += scratch[(size_t) j] * scratch[(size_t) j];
        if (e / (float) W < 1.0e-5f) return 0.0f; // zu leise

        float running = 0.0f;
        dn[0] = 1.0f;
        for (int tau = 1; tau <= tauMax; ++tau)
        {
            float s = 0.0f;
            const float* a = scratch.data();
            const float* b = scratch.data() + tau;
            for (int j = 0; j < W; ++j) { float d = a[j] - b[j]; s += d * d; }
            running += s;
            dn[(size_t) tau] = running > 0.0f ? s * (float) tau / running : 1.0f;
        }

        int found = -1;
        for (int t = tauMin; t < tauMax - 1; ++t)
            if (dn[(size_t) t] < 0.15f)
            {
                while (t + 1 < tauMax - 1 && dn[(size_t) t + 1] < dn[(size_t) t]) ++t;
                found = t;
                break;
            }
        if (found < 1) return 0.0f;

        float a = dn[(size_t) found - 1], b = dn[(size_t) found], c = dn[(size_t) found + 1];
        float den = a + c - 2.0f * b;
        float shift = std::abs (den) > 1.0e-9f ? 0.5f * (a - c) / den : 0.0f;
        return (float) sampleRate / ((float) found + shift);
    }
};
