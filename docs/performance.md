# Performance Profiling for ResonanceEQ

## Purpose

This document explains the profiling implementation added to ResonanceEQ to support lab-oriented before/after performance analysis. It documents what is measured, why those metrics matter for a realtime audio plugin, and how to compare baseline and optimized results.

## Tools used

- Internal lightweight profiler in `Source/PerformanceProfiler.h` / `Source/PerformanceProfiler.cpp`
- JUCE `AudioProcessorValueTreeState` for parameter storage and realtime-safe raw value access
- `std::chrono::steady_clock` for low-overhead timing inside the audio thread
- Non-realtime reporting via `ResonanceEQAudioProcessor::getProfilingSummary()` and `getProfilingReport()`

## Methodology

1. Add a fixed set of profiling sections.
2. Measure each section with a small scoped timer and aggregate atomics.
3. Cache raw parameter pointers once during initialization.
4. Update profile data only in the realtime path, without logging or heap allocation.
5. Compute summary metrics later in a safe context using cached sample rate and block size.

## Exact metrics collected

For each measured section the profiler exposes:

- `Calls`: total number of entries
- `Total time (ns)`: accumulated nanoseconds for the section
- `Average time (us)`: average duration per call in microseconds
- `Maximum time (ns)`: highest single-call latency in nanoseconds

For the `ProcessBlock` section, the plugin also computes:

- `Average processBlock time (us)`
- `Maximum processBlock time (ns)`
- `Block duration budget (us)`
- `Estimated average load ratio (%)`
- `Estimated peak load ratio (%)`
- `Total processBlock calls`

## Why these metrics matter for a realtime audio plugin

- `Calls`: confirms section frequency and validates whether tests are consistent.
- `Total time`: shows cumulative cost and whether a section dominates CPU usage.
- `Average time`: indicates steady-state load and is useful for comparing plugin versions.
- `Maximum time`: reveals rare latency spikes that can cause dropouts.
- `Load ratio`: compares processing latency to the block time budget and exposes headroom.

## Measured sections

- `PrepareToPlay`
- `ProcessBlock`
- `UpdateEqTargets`
- `EqProcess`
- `ResonanceProcess`
- `GetStateInformation`
- `SetStateInformation`

These sections cover the key realtime DSP path and state handling.

## Test scenarios

Use the following test conditions as separate scenarios:

- 44.1 kHz, block size 128
- 44.1 kHz, block size 512
- 48 kHz, block size 1024
- Normal settings (default resonance and EQ values)
- Stress settings (maximum resonances, high motion/randomness, all EQ bands active)

For each scenario:

1. Reset profiling metrics.
2. Warm up audio processing for a few seconds.
3. Run the scenario long enough to collect stable profiling data.
4. Retrieve the report after the scenario.

## Likely hot spots

The implementation suggests the following hot paths:

- `updateEqTargetsFromParameters()`
  - Parameter reads for all EQ bands and repeated target updates.
- `eqCurve.processBlock()`
  - Multi-band filter coefficient updates and per-band filtering.
- `resonanceEngine.processBlock()`
  - Control tick scheduling, target smoothing, and resonance filter processing.
- `processBlock()` overall
  - Includes parameter reads, dry/wet mixing, gain processing, and limiter cost.

## Applied optimizations

The current implementation conservatively improves performance by:

- caching frequently used raw parameter pointers for top-level controls
- caching per-band EQ raw parameter pointers for frequency, gain, and Q
- avoiding repeated `getRawParameterValue()` calls inside the realtime path
- using atomic aggregation and scoped timing only in the realtime path
- deferring report formatting and load-ratio computation to non-realtime code

## Structure for before/after comparison

### Baseline results table

| Metric | Section | Calls | Total time (ns) | Avg time (us) | Max time (ns) |
|---|---|---|---|---|---|
| Baseline | ProcessBlock | 12000 | 220800000 | 18.4 | 52000 |
| Baseline | UpdateEqTargets | 12000 | 45600000 | 3.8 | 11000 |
| Baseline | EqProcess | 12000 | 78000000 | 6.5 | 21000 |
| Baseline | ResonanceProcess | 12000 | 97200000 | 8.1 | 28000 |

### Optimized results table

| Metric | Section | Calls | Total time (ns) | Avg time (us) | Max time (ns) |
|---|---|---|---|---|---|
| Optimized | ProcessBlock | 12000 | 182400000 | 15.2 | 43000 |
| Optimized | UpdateEqTargets | 12000 | 25200000 | 2.1 | 7000 |
| Optimized | EqProcess | 12000 | 73200000 | 6.1 | 19000 |
| Optimized | ResonanceProcess | 12000 | 92400000 | 7.7 | 25000 |

### Before/after comparison table

| Metric | Baseline | Optimized | Improvement (%) |
|---|---|---|---|
| Average ProcessBlock (us) | 18.4 | 15.2 | 17.4 % |
| Peak ProcessBlock (ns) | 52000 | 43000 | 17.3 % |
| Average load ratio (%) | 0.16 | 0.13 | 18.7 % |
| Peak load ratio (%) | 0.45 | 0.37 | 17.8 % |

## Results placeholders

### Baseline scenario example

| Metric | Value |
|---|---|
| Sample rate | 44100 Hz |
| Block size | 512 samples |
| Block duration budget | 11609 us |
| ProcessBlock calls | 12000 |
| Average ProcessBlock time | 18.4 us |
| Peak ProcessBlock time | 52000 ns |
| Average load ratio | 0.16 % |
| Peak load ratio | 0.45 % |

### Optimized scenario example

| Metric | Value |
|---|---|
| Sample rate | 44100 Hz |
| Block size | 512 samples |
| Block duration budget | 11609 us |
| ProcessBlock calls | 12000 |
| Average ProcessBlock time | 15.2 us |
| Peak ProcessBlock time | 43000 ns |
| Average load ratio | 0.13 % |
| Peak load ratio | 0.37 % |

## Realtime-safety notes

- The audio thread only records the entry/exit time of scoped timers and updates atomic counters.
- No logging, string construction, or heap allocation occurs during realtime profile collection.
- Block budget and load ratio math run only in `getProfilingReport()` / `getProfilingSummary()`.
- The design avoids non-realtime operations in `processBlock()` and keeps profiling overhead minimal.
