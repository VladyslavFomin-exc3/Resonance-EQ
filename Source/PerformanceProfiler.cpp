#include "PerformanceProfiler.h"

#include <array>

const char* PerformanceProfiler::getSectionName(Section section) noexcept
{
    switch (section)
    {
        case Section::ProcessBlock: return "ProcessBlock";
        case Section::UpdateEqTargets: return "UpdateEqTargets";
        case Section::EqProcess: return "EqProcess";
        case Section::ResonanceProcess: return "ResonanceProcess";
        case Section::PrepareToPlay: return "PrepareToPlay";
        case Section::GetStateInformation: return "GetStateInformation";
        case Section::SetStateInformation: return "SetStateInformation";
        default: return "Unknown";
    }
}

void PerformanceProfiler::reset() noexcept
{
    for (auto& metric : metrics)
    {
        metric.callCount.store(0, std::memory_order_relaxed);
        metric.totalTimeNs.store(0, std::memory_order_relaxed);
        metric.maxTimeNs.store(0, std::memory_order_relaxed);
    }
}

void PerformanceProfiler::record(Section section, std::uint64_t elapsedNs) noexcept
{
    const auto index = static_cast<size_t>(section);
    auto& current = metrics[index];
    current.callCount.fetch_add(1, std::memory_order_relaxed);
    current.totalTimeNs.fetch_add(elapsedNs, std::memory_order_relaxed);

    auto previousMax = current.maxTimeNs.load(std::memory_order_relaxed);
    while (previousMax < elapsedNs &&
           !current.maxTimeNs.compare_exchange_weak(previousMax, elapsedNs, std::memory_order_relaxed))
    {
        // loop until maxTimeNs is at least elapsedNs
    }
}

PerformanceProfiler::Snapshot PerformanceProfiler::getSnapshot(Section section) const noexcept
{
    const auto index = static_cast<size_t>(section);
    const auto& current = metrics[index];

    const std::uint64_t callCount = current.callCount.load(std::memory_order_relaxed);
    const std::uint64_t totalTimeNs = current.totalTimeNs.load(std::memory_order_relaxed);
    const std::uint64_t maxTimeNs = current.maxTimeNs.load(std::memory_order_relaxed);

    const double averageTimeUs = callCount == 0 ? 0.0 : (static_cast<double>(totalTimeNs) / static_cast<double>(callCount)) / 1000.0;

    return Snapshot{section, callCount, totalTimeNs, maxTimeNs, averageTimeUs};
}

std::array<PerformanceProfiler::Snapshot, static_cast<size_t>(PerformanceProfiler::Section::Count)>
PerformanceProfiler::getSnapshots() const noexcept
{
    std::array<Snapshot, static_cast<size_t>(Section::Count)> snapshots;
    for (int i = 0; i < sectionCount(); ++i)
    {
        snapshots[static_cast<size_t>(i)] = getSnapshot(static_cast<Section>(i));
    }
    return snapshots;
}

juce::String PerformanceProfiler::getReport() const
{
    const auto snapshots = getSnapshots();

    juce::String report;
    report << "Profiling Summary\n";
    report << "=================\n";

    for (const auto& snapshot : snapshots)
    {
        report << getSectionName(snapshot.section) << ":\n";
        report << "  Calls: " << static_cast<int64_t>(snapshot.callCount) << "\n";
        report << "  Total: " << static_cast<int64_t>(snapshot.totalTimeNs) << " ns\n";
        report << "  Avg: " << snapshot.averageTimeUs << " us\n";
        report << "  Max: " << static_cast<int64_t>(snapshot.maxTimeNs) << " ns\n";
    }

    return report;
}

PerformanceProfiler::ScopedTimer::ScopedTimer(PerformanceProfiler& profilerToUse, Section sectionToMeasure) noexcept
    : profiler(profilerToUse), section(sectionToMeasure), startTime(std::chrono::steady_clock::now())
{
}

PerformanceProfiler::ScopedTimer::~ScopedTimer() noexcept
{
    const auto endTime = std::chrono::steady_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime).count();
    profiler.record(section, static_cast<std::uint64_t>(elapsed));
}
