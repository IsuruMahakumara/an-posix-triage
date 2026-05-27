#pragma once

#include <array>
#include <chrono>
#include <string_view>

namespace config {

inline constexpr std::array<std::string_view, 3> kTickerSet{"SOL", "ARB", "LINK"};
inline constexpr auto kIngestionInterval = std::chrono::milliseconds{10};

inline constexpr int kVolDeltaMin = -500;
inline constexpr int kVolDeltaMax = 500;
inline constexpr int kObiMin = 0;
inline constexpr int kObiMax = 100;

inline constexpr int kSignalThresholdVolDelta = 250;
inline constexpr int kSignalThresholdObi = 80;

inline constexpr std::string_view kPipePath{"/tmp/trading_pipeline.fifo"};

}  // namespace config
