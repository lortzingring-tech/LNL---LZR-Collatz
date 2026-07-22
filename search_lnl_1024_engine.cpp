// Lange Number Line Project (LNL)
// Fixed-width 1024-bit Collatz search with an embedded opening/endgame book,
// active-limb arithmetic, batched halving and exact bounded reuse caches.
// Version: LNL-LZR-CE-2
// Copyright (c) 2026 Mike Lange. All rights reserved.

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "lnl_opening_book_100k.hpp"

struct U1024 {
    std::array<std::uint64_t, 16> w{};
    std::uint8_t used = 0;

    bool operator==(const U1024& other) const noexcept {
        if (used != other.used) return false;
        for (unsigned i = 0; i < used; ++i) if (w[i] != other.w[i]) return false;
        return true;
    }

    int cmp(const U1024& other) const noexcept {
        if (used != other.used) return used < other.used ? -1 : 1;
        for (int i = static_cast<int>(used) - 1; i >= 0; --i) {
            const auto index = static_cast<std::size_t>(i);
            if (w[index] != other.w[index]) return w[index] < other.w[index] ? -1 : 1;
        }
        return 0;
    }

    bool odd() const noexcept { return (w[0] & 1U) != 0U; }
    bool zero() const noexcept { return used == 0U; }
    bool one() const noexcept { return used == 1U && w[0] == 1U; }
    bool fitsU32(std::uint32_t limit) const noexcept {
        return used <= 1U && w[0] <= limit;
    }
    std::uint32_t toU32() const noexcept { return static_cast<std::uint32_t>(w[0]); }

    void shr1() noexcept {
        if (used == 0U) return;
        for (unsigned i = 0; i + 1U < used; ++i) {
            w[i] = (w[i] >> 1U) | (w[i + 1U] << 63U);
        }
        w[used - 1U] >>= 1U;
        normalize();
    }

    void shr(unsigned amount) noexcept {
        if (amount == 0U) return;
        if (amount >= 1024U || amount >= static_cast<unsigned>(used) * 64U) {
            w.fill(0);
            used = 0U;
            return;
        }
        const unsigned oldUsed = used;
        if (amount < 64U) {
            for (unsigned i = 0; i + 1U < used; ++i) {
                w[i] = (w[i] >> amount) | (w[i + 1U] << (64U - amount));
            }
            w[used - 1U] >>= amount;
            normalize();
            return;
        }
        const unsigned words = amount / 64U;
        const unsigned bits = amount % 64U;
        const unsigned newCapacity = oldUsed - words;
        for (unsigned i = 0; i < newCapacity; ++i) {
            const unsigned source = i + words;
            std::uint64_t value = w[source] >> bits;
            if (bits != 0U && source + 1U < oldUsed) value |= w[source + 1U] << (64U - bits);
            w[i] = value;
        }
        for (unsigned i = newCapacity; i < oldUsed; ++i) w[i] = 0U;
        used = static_cast<std::uint8_t>(newCapacity);
        normalize();
    }

    unsigned trailingZeros() const {
        for (unsigned i = 0; i < used; ++i) {
            if (w[i] != 0U) return i * 64U + std::countr_zero(w[i]);
        }
        throw std::runtime_error("trailingZeros called for zero");
    }

    void up() {
        if (used == 0U) throw std::runtime_error("3n+1 called for zero");
        unsigned __int128 carry = 1U;
        for (unsigned i = 0; i < used; ++i) {
            const auto value = static_cast<unsigned __int128>(w[i]) * 3U + carry;
            w[i] = static_cast<std::uint64_t>(value);
            carry = value >> 64U;
        }
        if (carry != 0U) {
            if (used == w.size()) throw std::overflow_error("1024-bit overflow in 3n+1");
            w[used] = static_cast<std::uint64_t>(carry);
            ++used;
        }
    }

    std::uint32_t div10() noexcept {
        unsigned __int128 remainder = 0;
        for (int i = static_cast<int>(used) - 1; i >= 0; --i) {
            const auto index = static_cast<std::size_t>(i);
            const auto value = (remainder << 64U) | w[index];
            w[index] = static_cast<std::uint64_t>(value / 10U);
            remainder = value % 10U;
        }
        normalize();
        return static_cast<std::uint32_t>(remainder);
    }

    static U1024 fromU64(std::uint64_t value) noexcept {
        U1024 result;
        result.w[0] = value;
        result.used = value == 0U ? 0U : 1U;
        return result;
    }

private:
    void normalize() noexcept {
        while (used != 0U && w[used - 1U] == 0U) --used;
    }
};

static std::string decimal(U1024 value) {
    if (value.zero()) return "0";
    std::string result;
    while (!value.zero()) result.push_back(static_cast<char>('0' + value.div10()));
    std::reverse(result.begin(), result.end());
    return result;
}

static std::uint64_t mix64(std::uint64_t value) noexcept {
    value ^= value >> 30U;
    value *= 0xbf58476d1ce4e5b9ULL;
    value ^= value >> 27U;
    value *= 0x94d049bb133111ebULL;
    return value ^ (value >> 31U);
}

struct U1024Hash {
    std::size_t operator()(const U1024& value) const noexcept {
        std::uint64_t hash = 0x4c4e4c2d54542d31ULL ^ value.used;
        for (unsigned i = 0; i < value.used; ++i) {
            hash ^= mix64(value.w[i] + 0x9e3779b97f4a7c15ULL * (i + 1U));
            hash = std::rotl(hash, 11);
        }
        return static_cast<std::size_t>(mix64(hash));
    }
};

static std::uint64_t splitmix(std::uint64_t& state) noexcept {
    std::uint64_t z = (state += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27U)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31U);
}

// Collision-free across block ids while samples remain below one billion.
static U1024 counterStart(unsigned bits, std::uint64_t block, std::uint64_t sample) {
    if (bits < 2U || bits > 900U) throw std::range_error("bits must be 2..900");
    if (sample >= 1000000000ULL || block > 9000000000ULL) {
        throw std::range_error("counter block/sample outside supported range");
    }
    const std::uint64_t index = block * 1000000000ULL + sample;
    U1024 value;
    value.w[0] = index * 2U + 1U;
    std::uint64_t state = index ^ 0x4c4e4c2d434e5452ULL;
    const unsigned limbs = (bits + 63U) / 64U;
    value.used = static_cast<std::uint8_t>(limbs);
    for (unsigned i = 1; i < limbs; ++i) value.w[i] = splitmix(state);
    const unsigned remainder = bits % 64U;
    if (remainder != 0U) value.w[limbs - 1U] &= (std::uint64_t{1} << remainder) - 1U;
    value.w[(bits - 1U) / 64U] |= std::uint64_t{1} << ((bits - 1U) % 64U);
    return value;
}

struct TrajectoryResult {
    U1024 peak{};
    std::uint64_t steps = 0;
    std::uint64_t peakStep = 0;
    std::uint64_t kernelIterations = 0;
    std::uint64_t evaluatedTransitions = 0;
    bool openingHit = false;
    bool transpositionHit = false;
};

static void updatePeak(TrajectoryResult& result, const U1024& candidate, std::uint64_t step) noexcept {
    if (candidate.cmp(result.peak) > 0) {
        result.peak = candidate;
        result.peakStep = step;
    }
}

static TrajectoryResult analyzeScalar(const U1024& start) {
    U1024 value = start;
    TrajectoryResult result;
    result.peak = start;
    while (!value.one()) {
        if (value.odd()) value.up(); else value.shr1();
        ++result.steps;
        ++result.kernelIterations;
        ++result.evaluatedTransitions;
        updatePeak(result, value, result.steps);
        if (result.steps > 100000000ULL) throw std::runtime_error("trajectory safety limit exceeded");
    }
    return result;
}

static TrajectoryResult analyzeBatched(const U1024& start) {
    U1024 value = start;
    TrajectoryResult result;
    result.peak = start;
    while (!value.one()) {
        ++result.kernelIterations;
        if (!value.odd()) {
            const unsigned divisions = value.trailingZeros();
            value.shr(divisions);
            result.steps += divisions;
            result.evaluatedTransitions += divisions;
        } else {
            value.up();
            ++result.steps;
            ++result.evaluatedTransitions;
            updatePeak(result, value, result.steps);
            const unsigned divisions = value.trailingZeros();
            value.shr(divisions);
            result.steps += divisions;
            result.evaluatedTransitions += divisions;
        }
        if (result.steps > 100000000ULL) throw std::runtime_error("trajectory safety limit exceeded");
    }
    return result;
}

// High-throughput mode for previously unseen search starts.  It uses batched
// halving and the immutable embedded book, but deliberately avoids building a
// dynamic trajectory path or writing a transposition table.
static TrajectoryResult analyzeFastEngine(const U1024& start) {
    constexpr std::uint64_t bookProbeStride = 32U;
    U1024 value = start;
    TrajectoryResult result;
    result.peak = start;
    while (true) {
        if (value.one()) return result;
        if (result.kernelIterations % bookProbeStride == 0U && value.fitsU32(LNL_OPENING_LIMIT)) {
            const auto& entry = LNL_OPENING_BOOK[value.toU32()];
            const U1024 suffixPeak = U1024::fromU64(entry.peak);
            if (suffixPeak.cmp(result.peak) > 0) {
                result.peak = suffixPeak;
                result.peakStep = result.steps + entry.peak_offset;
            }
            result.steps += entry.steps_to_one;
            result.openingHit = true;
            return result;
        }
        ++result.kernelIterations;
        if (!value.odd()) {
            const unsigned divisions = value.trailingZeros();
            value.shr(divisions);
            result.steps += divisions;
            result.evaluatedTransitions += divisions;
        } else {
            value.up();
            ++result.steps;
            ++result.evaluatedTransitions;
            updatePeak(result, value, result.steps);
            const unsigned divisions = value.trailingZeros();
            value.shr(divisions);
            result.steps += divisions;
            result.evaluatedTransitions += divisions;
        }
        if (result.steps > 100000000ULL) throw std::runtime_error("trajectory safety limit exceeded");
    }
}

// PeakQueen/LNL learning: reuse only a fully completed result whose complete
// 1024-bit start key matches.  A cache miss takes the unchanged engine-fast
// path.  FIFO replacement keeps memory bounded and never treats a residue or
// hash collision as an exact match.
class ExactResultCache {
public:
    explicit ExactResultCache(std::size_t maximumEntries) : maximumEntries_(maximumEntries) {
        if (maximumEntries_ != 0U) {
            table_.reserve(maximumEntries_);
            insertionOrder_.reserve(maximumEntries_);
        }
    }

    TrajectoryResult analyze(const U1024& start) {
        if (const auto found = table_.find(start); found != table_.end()) {
            TrajectoryResult result = found->second;
            result.kernelIterations = 0U;
            result.evaluatedTransitions = 0U;
            result.openingHit = false;
            result.transpositionHit = true;
            ++hits_;
            return result;
        }

        TrajectoryResult result = analyzeFastEngine(start);
        storeCompleted(start, result);
        return result;
    }

    std::size_t entries() const noexcept { return table_.size(); }
    std::uint64_t hits() const noexcept { return hits_; }

private:
    void storeCompleted(const U1024& start, const TrajectoryResult& result) {
        if (maximumEntries_ == 0U) return;
        if (table_.size() < maximumEntries_) {
            table_.try_emplace(start, result);
            insertionOrder_.push_back(start);
            return;
        }

        table_.erase(insertionOrder_[replacementCursor_]);
        table_.try_emplace(start, result);
        insertionOrder_[replacementCursor_] = start;
        replacementCursor_ = (replacementCursor_ + 1U) % maximumEntries_;
    }

    std::size_t maximumEntries_;
    std::unordered_map<U1024, TrajectoryResult, U1024Hash> table_;
    std::vector<U1024> insertionOrder_;
    std::size_t replacementCursor_ = 0U;
    std::uint64_t hits_ = 0U;
};

struct Summary {
    std::uint64_t steps = 0;
    U1024 peak{};
    std::uint64_t peakOffset = 0;
};

struct CachedSegment {
    U1024 start{};
    U1024 peak{};
    std::uint64_t cost = 0;
    std::uint64_t peakOffset = 0;
};

static Summary combine(const CachedSegment& segment, const Summary& child) {
    Summary parent;
    parent.steps = segment.cost + child.steps;
    parent.peak = segment.peak;
    parent.peakOffset = segment.peakOffset;
    if (child.peak.cmp(parent.peak) > 0) {
        parent.peak = child.peak;
        parent.peakOffset = segment.cost + child.peakOffset;
    }
    return parent;
}

class LNLSearchEngine {
public:
    explicit LNLSearchEngine(std::size_t maximumEntries = 250000U,
                             unsigned probeStride = 8U,
                             unsigned storeStride = 128U)
        : maximumEntries_(maximumEntries), probeStride_(probeStride), storeStride_(storeStride) {
        if (maximumEntries_ != 0U) table_.reserve(maximumEntries_);
    }

    TrajectoryResult analyze(const U1024& start) {
        U1024 value = start;
        std::vector<CachedSegment> segments;
        segments.reserve(32U);
        CachedSegment segment;
        segment.start = value;
        segment.peak = value;
        unsigned segmentEdges = 0;
        std::uint64_t totalEdges = 0;
        Summary suffix;
        bool openingHit = false;
        bool transpositionHit = false;
        std::uint64_t kernelIterations = 0;
        std::uint64_t evaluatedTransitions = 0;

        while (true) {
            if (value.fitsU32(LNL_OPENING_LIMIT)) {
                const auto& entry = LNL_OPENING_BOOK[value.toU32()];
                suffix.steps = entry.steps_to_one;
                suffix.peak = U1024::fromU64(entry.peak);
                suffix.peakOffset = entry.peak_offset;
                openingHit = true;
                break;
            }
            if (maximumEntries_ != 0U &&
                (totalEdges == 0U || totalEdges % probeStride_ == 0U)) {
                const auto found = table_.find(value);
                if (found != table_.end()) {
                    suffix = found->second;
                    transpositionHit = true;
                    ++transpositionHits_;
                    break;
                }
            }

            U1024 transitionPeak = value;
            std::uint64_t transitionPeakOffset = 0;
            unsigned cost = 0;
            if (!value.odd()) {
                const unsigned divisions = value.trailingZeros();
                cost = divisions;
                value.shr(divisions);
            } else {
                value.up();
                transitionPeak = value;
                transitionPeakOffset = 1U;
                const unsigned divisions = value.trailingZeros();
                cost = divisions + 1U;
                value.shr(divisions);
            }
            if (transitionPeak.cmp(segment.peak) > 0) {
                segment.peak = transitionPeak;
                segment.peakOffset = segment.cost + transitionPeakOffset;
            }
            segment.cost += cost;
            evaluatedTransitions += cost;
            ++kernelIterations;
            ++totalEdges;
            ++segmentEdges;
            if (segmentEdges == storeStride_) {
                segments.push_back(segment);
                segment = CachedSegment{};
                segment.start = value;
                segment.peak = value;
                segmentEdges = 0;
            }
            if (evaluatedTransitions > 100000000ULL) throw std::runtime_error("trajectory safety limit exceeded");
        }

        if (segment.cost != 0U) segments.push_back(segment);

        Summary result = suffix;
        for (std::size_t i = segments.size(); i-- > 0U;) {
            result = combine(segments[i], result);
            maybeStore(segments[i].start, result);
        }
        maybeStore(start, result);

        TrajectoryResult output;
        output.peak = result.peak;
        output.steps = result.steps;
        output.peakStep = result.peakOffset;
        output.kernelIterations = kernelIterations;
        output.evaluatedTransitions = evaluatedTransitions;
        output.openingHit = openingHit;
        output.transpositionHit = transpositionHit;
        return output;
    }

    std::size_t entries() const noexcept { return table_.size(); }
    std::uint64_t transpositionHits() const noexcept { return transpositionHits_; }

private:
    void maybeStore(const U1024& value, const Summary& summary) {
        if (maximumEntries_ == 0U || table_.size() >= maximumEntries_) return;
        table_.try_emplace(value, summary);
    }

    std::size_t maximumEntries_;
    unsigned probeStride_;
    unsigned storeStride_;
    std::unordered_map<U1024, Summary, U1024Hash> table_;
    std::uint64_t transpositionHits_ = 0;
};

static bool equalResult(const TrajectoryResult& left, const TrajectoryResult& right) noexcept {
    return left.steps == right.steps && left.peakStep == right.peakStep && left.peak == right.peak;
}

static double elapsedMilliseconds(std::chrono::steady_clock::time_point start) {
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
}

static int verifyOpening() {
    std::uint64_t failures = 0;
    for (std::uint32_t n = 1; n <= LNL_OPENING_LIMIT; ++n) {
        const auto direct = analyzeScalar(U1024::fromU64(n));
        const auto& entry = LNL_OPENING_BOOK[n];
        if (direct.steps != entry.steps_to_one || direct.peakStep != entry.peak_offset ||
            direct.peak.cmp(U1024::fromU64(entry.peak)) != 0) {
            ++failures;
            if (failures <= 10U) std::cerr << "opening mismatch at " << n << '\n';
        }
    }
    std::cout << "{\"opening_rows\":" << LNL_OPENING_LIMIT
              << ",\"failures\":" << failures
              << ",\"max_steps\":" << LNL_OPENING_MAX_STEPS
              << ",\"max_peak_offset\":" << LNL_OPENING_MAX_PEAK_OFFSET
              << ",\"max_peak\":" << LNL_OPENING_MAX_PEAK << "}\n";
    return failures == 0U ? 0 : 1;
}

static int benchmark(std::uint64_t samples, unsigned bits, std::uint64_t block,
                     std::size_t maximumEntries) {
    if (samples == 0U) throw std::range_error("samples must be positive");
    std::vector<TrajectoryResult> reference;
    reference.reserve(static_cast<std::size_t>(samples));
    std::uint64_t scalarTransitions = 0;
    std::uint64_t scalarIterations = 0;
    auto started = std::chrono::steady_clock::now();
    for (std::uint64_t i = 0; i < samples; ++i) {
        auto result = analyzeScalar(counterStart(bits, block, i));
        scalarTransitions += result.evaluatedTransitions;
        scalarIterations += result.kernelIterations;
        reference.push_back(result);
    }
    const double scalarMs = elapsedMilliseconds(started);

    std::uint64_t failures = 0;
    std::uint64_t batchedTransitions = 0;
    std::uint64_t batchedIterations = 0;
    started = std::chrono::steady_clock::now();
    for (std::uint64_t i = 0; i < samples; ++i) {
        const auto result = analyzeBatched(counterStart(bits, block, i));
        batchedTransitions += result.evaluatedTransitions;
        batchedIterations += result.kernelIterations;
        if (!equalResult(result, reference[static_cast<std::size_t>(i)])) ++failures;
    }
    const double batchedMs = elapsedMilliseconds(started);

    std::uint64_t fastTransitions = 0;
    std::uint64_t fastIterations = 0;
    std::uint64_t fastOpeningHits = 0;
    started = std::chrono::steady_clock::now();
    for (std::uint64_t i = 0; i < samples; ++i) {
        const auto result = analyzeFastEngine(counterStart(bits, block, i));
        fastTransitions += result.evaluatedTransitions;
        fastIterations += result.kernelIterations;
        fastOpeningHits += result.openingHit ? 1U : 0U;
        if (!equalResult(result, reference[static_cast<std::size_t>(i)])) ++failures;
    }
    const double fastMs = elapsedMilliseconds(started);

    ExactResultCache exactCache(maximumEntries);
    std::uint64_t exactColdTransitions = 0;
    started = std::chrono::steady_clock::now();
    for (std::uint64_t i = 0; i < samples; ++i) {
        const auto result = exactCache.analyze(counterStart(bits, block, i));
        exactColdTransitions += result.evaluatedTransitions;
        if (!equalResult(result, reference[static_cast<std::size_t>(i)])) ++failures;
    }
    const double exactColdMs = elapsedMilliseconds(started);

    std::uint64_t exactWarmTransitions = 0;
    started = std::chrono::steady_clock::now();
    for (std::uint64_t i = 0; i < samples; ++i) {
        const auto result = exactCache.analyze(counterStart(bits, block, i));
        exactWarmTransitions += result.evaluatedTransitions;
        if (!equalResult(result, reference[static_cast<std::size_t>(i)])) ++failures;
    }
    const double exactWarmMs = elapsedMilliseconds(started);

    LNLSearchEngine engine(maximumEntries);
    std::uint64_t coldTransitions = 0;
    std::uint64_t coldIterations = 0;
    std::uint64_t coldOpeningHits = 0;
    std::uint64_t coldTranspositionHits = 0;
    started = std::chrono::steady_clock::now();
    for (std::uint64_t i = 0; i < samples; ++i) {
        const auto result = engine.analyze(counterStart(bits, block, i));
        coldTransitions += result.evaluatedTransitions;
        coldIterations += result.kernelIterations;
        coldOpeningHits += result.openingHit ? 1U : 0U;
        coldTranspositionHits += result.transpositionHit ? 1U : 0U;
        if (!equalResult(result, reference[static_cast<std::size_t>(i)])) ++failures;
    }
    const double coldMs = elapsedMilliseconds(started);

    std::uint64_t warmTransitions = 0;
    std::uint64_t warmIterations = 0;
    std::uint64_t warmTranspositionHits = 0;
    started = std::chrono::steady_clock::now();
    for (std::uint64_t i = 0; i < samples; ++i) {
        const auto result = engine.analyze(counterStart(bits, block, i));
        warmTransitions += result.evaluatedTransitions;
        warmIterations += result.kernelIterations;
        warmTranspositionHits += result.transpositionHit ? 1U : 0U;
        if (!equalResult(result, reference[static_cast<std::size_t>(i)])) ++failures;
    }
    const double warmMs = elapsedMilliseconds(started);

    const std::uint64_t checksum = reference.empty() ? 0U :
        reference.front().steps ^ reference.back().steps ^ reference.back().peak.w[0];
    std::cout << std::fixed << std::setprecision(6)
              << "{\n"
              << "  \"samples\": " << samples << ",\n"
              << "  \"bits\": " << bits << ",\n"
              << "  \"block\": " << block << ",\n"
              << "  \"opening_rows_embedded\": " << LNL_OPENING_LIMIT << ",\n"
              << "  \"scalar\": {\"milliseconds\": " << scalarMs
              << ", \"transitions\": " << scalarTransitions
              << ", \"kernel_iterations\": " << scalarIterations << "},\n"
              << "  \"batched\": {\"milliseconds\": " << batchedMs
              << ", \"transitions\": " << batchedTransitions
              << ", \"kernel_iterations\": " << batchedIterations
              << ", \"speedup_vs_scalar\": " << scalarMs / batchedMs << "},\n"
              << "  \"engine_fast_new\": {\"milliseconds\": " << fastMs
              << ", \"evaluated_transitions\": " << fastTransitions
              << ", \"kernel_iterations\": " << fastIterations
              << ", \"opening_hits\": " << fastOpeningHits
              << ", \"speedup_vs_scalar\": " << scalarMs / fastMs << "},\n"
              << "  \"exact_reuse_cold\": {\"milliseconds\": " << exactColdMs
              << ", \"evaluated_transitions\": " << exactColdTransitions
              << ", \"entries\": " << exactCache.entries()
              << ", \"speedup_vs_scalar\": " << scalarMs / exactColdMs << "},\n"
              << "  \"exact_reuse_warm\": {\"milliseconds\": " << exactWarmMs
              << ", \"evaluated_transitions\": " << exactWarmTransitions
              << ", \"cache_hits\": " << exactCache.hits()
              << ", \"speedup_vs_scalar\": " << scalarMs / exactWarmMs << "},\n"
              << "  \"engine_cold\": {\"milliseconds\": " << coldMs
              << ", \"evaluated_transitions\": " << coldTransitions
              << ", \"kernel_iterations\": " << coldIterations
              << ", \"opening_hits\": " << coldOpeningHits
              << ", \"transposition_hits\": " << coldTranspositionHits
              << ", \"speedup_vs_scalar\": " << scalarMs / coldMs << "},\n"
              << "  \"engine_warm\": {\"milliseconds\": " << warmMs
              << ", \"evaluated_transitions\": " << warmTransitions
              << ", \"kernel_iterations\": " << warmIterations
              << ", \"transposition_hits\": " << warmTranspositionHits
              << ", \"speedup_vs_scalar\": " << scalarMs / warmMs << "},\n"
              << "  \"transposition_entries\": " << engine.entries() << ",\n"
              << "  \"verification_failures\": " << failures << ",\n"
              << "  \"checksum\": " << checksum << "\n"
              << "}\n";
    return failures == 0U ? 0 : 1;
}

static int search(std::uint64_t samples, unsigned bits, std::uint64_t block,
                  const std::string& mode, std::size_t maximumEntries) {
    LNLSearchEngine engine(maximumEntries);
    ExactResultCache exactCache(maximumEntries);
    U1024 bestPeak{};
    U1024 bestStart{};
    std::uint64_t bestPeakStep = 0;
    std::uint64_t bestSteps = 0;
    std::uint64_t evaluatedTransitions = 0;
    std::uint64_t openingHits = 0;
    std::uint64_t transpositionHits = 0;
    const auto started = std::chrono::steady_clock::now();
    for (std::uint64_t i = 0; i < samples; ++i) {
        const U1024 start = counterStart(bits, block, i);
        TrajectoryResult result;
        if (mode == "scalar") result = analyzeScalar(start);
        else if (mode == "batched") result = analyzeBatched(start);
        else if (mode == "engine-fast") result = analyzeFastEngine(start);
        else if (mode == "engine-reuse") result = exactCache.analyze(start);
        else if (mode == "engine-cache") result = engine.analyze(start);
        else throw std::range_error(
            "mode must be scalar, batched, engine-fast, engine-reuse or engine-cache");
        evaluatedTransitions += result.evaluatedTransitions;
        openingHits += result.openingHit ? 1U : 0U;
        transpositionHits += result.transpositionHit ? 1U : 0U;
        if (result.peak.cmp(bestPeak) > 0) {
            bestPeak = result.peak;
            bestStart = start;
            bestPeakStep = result.peakStep;
            bestSteps = result.steps;
            std::cout << "candidate," << decimal(start) << ',' << decimal(result.peak)
                      << ',' << result.peakStep << ',' << result.steps << '\n';
        }
    }
    const double elapsed = elapsedMilliseconds(started);
    std::cout << std::fixed << std::setprecision(6)
              << "summary," << samples << ',' << decimal(bestStart) << ',' << decimal(bestPeak)
              << ',' << bestPeakStep << ',' << bestSteps << ",bits," << bits
              << ",block," << block << ",mode," << mode
              << ",milliseconds," << elapsed
              << ",evaluated_transitions," << evaluatedTransitions
              << ",opening_hits," << openingHits
              << ",transposition_hits," << transpositionHits
              << ",transposition_entries," << engine.entries()
              << ",exact_reuse_entries," << exactCache.entries() << '\n';
    return 0;
}

static void usage() {
    std::cout << "LNL/LZR 1024-bit C++ search engine V2\n\n"
              << "  ./search_lnl_1024_engine verify-book\n"
              << "  ./search_lnl_1024_engine benchmark [samples] [bits] [block] [tt_entries]\n"
              << "  ./search_lnl_1024_engine search [samples] [bits] [block] [scalar|batched|engine-fast|engine-reuse|engine-cache] [tt_entries]\n";
}

int main(int argc, char** argv) {
    try {
        const std::string command = argc > 1 ? argv[1] : "help";
        if (command == "help") { usage(); return 0; }
        if (command == "verify-book") return verifyOpening();
        if (command == "benchmark") {
            const std::uint64_t samples = argc > 2 ? std::stoull(argv[2]) : 10000ULL;
            const unsigned bits = argc > 3 ? static_cast<unsigned>(std::stoul(argv[3])) : 128U;
            const std::uint64_t block = argc > 4 ? std::stoull(argv[4]) : 20260720ULL;
            const std::size_t entries = argc > 5 ? static_cast<std::size_t>(std::stoull(argv[5])) : 250000U;
            return benchmark(samples, bits, block, entries);
        }
        if (command == "search") {
            const std::uint64_t samples = argc > 2 ? std::stoull(argv[2]) : 100000ULL;
            const unsigned bits = argc > 3 ? static_cast<unsigned>(std::stoul(argv[3])) : 333U;
            const std::uint64_t block = argc > 4 ? std::stoull(argv[4]) : 0ULL;
            const std::string mode = argc > 5 ? argv[5] : "engine-fast";
            const std::size_t entries = argc > 6 ? static_cast<std::size_t>(std::stoull(argv[6])) : 250000U;
            return search(samples, bits, block, mode, entries);
        }
        throw std::range_error("unknown command");
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 2;
    }
}
