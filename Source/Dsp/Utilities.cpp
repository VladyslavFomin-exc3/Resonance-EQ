#include "Utilities.h"

#include <cmath>

namespace hreq::util
{
float clampFloat(const float value, const float minValue, const float maxValue) noexcept
{
    return juce::jlimit(minValue, maxValue, value);
}

int clampInt(const int value, const int minValue, const int maxValue) noexcept
{
    return juce::jlimit(minValue, maxValue, value);
}

float lerp(const float a, const float b, const float t) noexcept
{
    return a + (b - a) * juce::jlimit(0.0f, 1.0f, t);
}

float logLerp(const float a, const float b, const float t) noexcept
{
    const auto clampedA = juce::jmax(1.0e-6f, a);
    const auto clampedB = juce::jmax(1.0e-6f, b);
    const auto mix = juce::jlimit(0.0f, 1.0f, t);
    return std::exp(std::log(clampedA) + (std::log(clampedB) - std::log(clampedA)) * mix);
}

float smoothstep(const float t) noexcept
{
    const auto x = juce::jlimit(0.0f, 1.0f, t);
    return x * x * (3.0f - 2.0f * x);
}

float dbToGainSafe(const float db) noexcept
{
    return juce::Decibels::decibelsToGain(juce::jlimit(-120.0f, 24.0f, db));
}

float sanitize(const float value, const float fallback) noexcept
{
    if (std::isfinite(value))
        return value;

    return fallback;
}
} // namespace hreq::util
