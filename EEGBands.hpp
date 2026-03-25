#pragma once
#include <cmath>
#include <algorithm>

// ─────────────────────────────────────────────────────────────────────────────
// Direct Form II Transposed biquad
// ─────────────────────────────────────────────────────────────────────────────
struct Biquad {
    float b0=0,b1=0,b2=0,a1=0,a2=0,s1=0,s2=0;
    float process(float x) {
        float y = b0*x + s1;
        s1 = b1*x - a1*y + s2;
        s2 = b2*x - a2*y;
        return y;
    }
    void reset() { s1=s2=0; }
};

inline Biquad makeBPF(float f0, float Q, float Fs=250.f) {
    float w0    = 2.f * 3.14159265f * f0 / Fs;
    float sinW  = std::sin(w0), cosW = std::cos(w0);
    float alpha = sinW / (2.f * Q);
    float inv   = 1.f / (1.f + alpha);
    Biquad b;
    b.b0 =  sinW * 0.5f * inv;
    b.b1 =  0.f;
    b.b2 = -b.b0;
    b.a1 = -2.f * cosW * inv;
    b.a2 = (1.f - alpha) * inv;
    return b;
}

// DC-blocking highpass ~1.2 Hz — removes slow drift for blink detection
struct HighPass1 {
    float xPrev=0, yPrev=0, R=0.97f;
    float process(float x) {
        float y = R * (yPrev + x - xPrev);
        xPrev = x; yPrev = y;
        return y;
    }
};

inline float alphaFromMs(float ms, float Fs=250.f) {
    return 1.f - std::exp(-1.f / (Fs * ms / 1000.f));
}

// ─────────────────────────────────────────────────────────────────────────────
// BandEnvelope: BPF → |x| → asymmetric EMA → output EMA
// ─────────────────────────────────────────────────────────────────────────────
struct BandEnvelope {
    Biquad bpf;
    float  env=0.f, output=0.f;
    float  aA, aR, aOut;

    BandEnvelope(float f0, float Q,
                 float Fs=250.f,
                 float attackMs=80.f,
                 float releaseMs=1200.f,
                 float outSmoothMs=200.f)
        : bpf(makeBPF(f0, Q, Fs))
        , aA  (alphaFromMs(attackMs,    Fs))
        , aR  (alphaFromMs(releaseMs,   Fs))
        , aOut(alphaFromMs(outSmoothMs, Fs))
    {}

    void feed(float x) {
        float a = std::abs(bpf.process(x));
        env += ((a > env) ? aA : aR) * (a - env);
    }

    float get() {
        output += aOut * (env - output);
        return output;
    }

    void reset() { bpf.reset(); env=output=0.f; }
};

// ─────────────────────────────────────────────────────────────────────────────
// BlinkDetector — fires true on blink onset
// ─────────────────────────────────────────────────────────────────────────────
struct BlinkDetector {
    HighPass1 hp;
    float rmsEMA=0.f, rmsAlpha;
    float threshold=4.0f;
    int   cooldown=0, cooldownSamps;
    int   warmup=0,   warmupSamps;

    explicit BlinkDetector(float Fs=250.f,
                           float cooldownMs=500.f,
                           float baselineMs=2000.f,
                           float warmupMs=1500.f)
        : rmsAlpha     (alphaFromMs(baselineMs, Fs))
        , cooldownSamps((int)(Fs * cooldownMs / 1000.f))
        , warmupSamps  ((int)(Fs * warmupMs   / 1000.f))
    {
        threshold = 2.8f;  // было 4.0 — теперь реагирует на обычное моргание
    }

    bool process(float x) {
        float xabs = std::abs(hp.process(x));
        rmsEMA += rmsAlpha * (xabs - rmsEMA);
        if (warmup < warmupSamps) { ++warmup; return false; }
        if (cooldown > 0)         { --cooldown; return false; }
        if (rmsEMA < 1e-6f)       { return false; }
        if (xabs > threshold * rmsEMA) {
            cooldown = cooldownSamps;
            return true;
        }
        return false;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// BlinkEnvelope — float 0-1 channel for TouchDesigner
// On blink: jumps to 1.0 instantly, then decays over decayMs
// Much more useful in TD than a momentary int trigger
// ─────────────────────────────────────────────────────────────────────────────
struct BlinkEnvelope {
    float value   = 0.f;
    float aDecay;

    explicit BlinkEnvelope(float Fs=250.f, float decayMs=350.f)
        : aDecay(alphaFromMs(decayMs, Fs))
    {}

    // call every sample; pass blink=true when BlinkDetector fires
    void feed(bool blink) {
        if (blink) value = 1.0f;
        else       value -= aDecay * value;  // exponential decay toward 0
    }

    float get() const { return value; }
};

// ─────────────────────────────────────────────────────────────────────────────
// FastRMS — очень короткое окно (~50ms), реагирует на всё мгновенно
// Это "arousal" канал: напряжение лица, концентрация, любое движение
// Не научно чистый, зато визуально самый живой из всего что можно получить
// с фронтальных электродов
// ─────────────────────────────────────────────────────────────────────────────
struct FastRMS {
    float rms   = 0.f;
    float slow  = 0.f;
    float aFast, aSlow;

    explicit FastRMS(float Fs=250.f, float fastMs=20.f, float slowMs=2000.f)
        : aFast(alphaFromMs(fastMs, Fs))
        , aSlow(alphaFromMs(slowMs, Fs))
    {}

    void feed(float x) {
        float x2 = x * x;
        rms  += aFast * (x2 - rms);
        slow += aSlow * (x2 - slow);
    }

    // превышение над базовым уровнем — агрессивнее убираем фон
    float get() const {
        float v = rms - slow * 0.6f;
        return v > 0.f ? std::sqrt(v) : 0.f;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// AutoNorm01 — maps arbitrary positive floats to 0-1
// Expands bounds instantly, contracts very slowly
// ─────────────────────────────────────────────────────────────────────────────
struct AutoNorm01 {
    float lo=-1.f, hi=1.f;
    bool  inited=false;

    float update(float x) {
        if (!inited) { lo=x*0.5f; hi=x*1.5f+1e-6f; inited=true; }
        if (x < lo) lo = x;
        else         lo += 0.00005f * (x - lo);
        if (x > hi) hi = x;
        else         hi += 0.00005f * (x - hi);
        if (hi - lo < 1e-9f) return 0.f;
        return std::clamp((x - lo) / (hi - lo), 0.f, 1.f);
    }
};
