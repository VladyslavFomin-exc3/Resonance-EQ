#pragma once

#include <JuceHeader.h>

namespace hreq::util
{
float clampFloat(float value, float minValue, float maxValue) noexcept;
int clampInt(int value, int minValue, int maxValue) noexcept;
float lerp(float a, float b, float t) noexcept;
float logLerp(float a, float b, float t) noexcept;
float smoothstep(float t) noexcept;
float dbToGainSafe(float db) noexcept;
float sanitize(float value, float fallback = 0.0f) noexcept;
} // namespace hreq::util
