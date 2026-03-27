#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <chrono>

class PerformanceProfiler
{
public:
    enum class Section : int
    {
        ProcessBlock = 0,
        UpdateEqTargets,
        EqProcess,
        ResonanceProcess,
        PrepareToPlay,
        GetStateInformation,
        SetStateInformation,
        Count
    };

    struct Metrics
    {
        std::atomic<std::uint64_t> callCount = 0;
        std::atomic<std::uint64_t> totalTimeNs = 0;
        std::atomic<std::uint64_t> maxTimeNs = 0;
    };

    struct Snapshot
    {
        Section section = Section::ProcessBlock;
        std::uint64_t callCount = 0;
        std::uint64_t totalTimeNs = 0;
        std::uint64_t maxTimeNs = 0;
        double averageTimeUs = 0.0;
    };

    PerformanceProfiler() = default;

    void reset() noexcept;
    void record(Section section, std::uint64_t elapsedNs) noexcept;
    Snapshot getSnapshot(Section section) const noexcept;
    std::array<Snapshot, static_cast<size_t>(Section::Count)> getSnapshots() const noexcept;
    juce::String getReport() const;

    struct ScopedTimer
    {
        ScopedTimer(PerformanceProfiler& profilerToUse, Section sectionToMeasure) noexcept;
        ~ScopedTimer() noexcept;

        ScopedTimer(const ScopedTimer&) = delete;
        ScopedTimer& operator=(const ScopedTimer&) = delete;

    private:
        PerformanceProfiler& profiler;
        Section section;
        std::chrono::steady_clock::time_point startTime;
    };

    static constexpr int sectionCount() noexcept { return static_cast<int>(Section::Count); }
    static const char* getSectionName(Section section) noexcept;

private:
    std::array<Metrics, static_cast<size_t>(Section::Count)> metrics{};
};
