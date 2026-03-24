#pragma once

#include <JuceHeader.h>

namespace hreq::util
{
/**
 * @brief Clamp a float between min and max.
 * @param value Input value.
 * @param minValue Minimum allowed value.
 * @param maxValue Maximum allowed value.
 * @return Clamped value.
 */
float clampFloat(float value, float minValue, float maxValue) noexcept;

/**
 * @brief Clamp an int between min and max.
 * @param value Input value.
 * @param minValue Minimum allowed value.
 * @param maxValue Maximum allowed value.
 * @return Clamped value.
 */
int clampInt(int value, int minValue, int maxValue) noexcept;

/**
 * @brief Linear interpolation.
 * @param a Start value.
 * @param b End value.
 * @param t Interpolation factor [0, 1].
 * @return Interpolated value.
 */
float lerp(float a, float b, float t) noexcept;

/**
 * @brief Logarithmic interpolation for perceptual scaling.
 * @param a Start value.
 * @param b End value.
 * @param t Interpolation factor [0, 1].
 * @return Interpolated value on log curve.
 */
float logLerp(float a, float b, float t) noexcept;

/**
 * @brief Smoothstep interpolation function.
 * @param t Parameter, typically in [0,1].
 * @return Smoothed value.
 */
float smoothstep(float t) noexcept;

/**
 * @brief Convert dB to gain and avoid numeric instabilities.
 * @param db Gain in decibels.
 * @return Linear gain.
 */
float dbToGainSafe(float db) noexcept;

/**
 * @brief Validate value; returns fallback if NaN/inf.
 *
 * Usage:
 * @code
 * float x = sanitize(maybeNan, 0.0f);
 * @endcode
 *
 * @param value Input value.
 * @param fallback Value to use if input is invalid.
 * @return Valid value.
 */
float sanitize(float value, float fallback = 0.0f) noexcept;
} // namespace hreq::util
