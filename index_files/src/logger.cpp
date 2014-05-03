#include "filecrawler/logger.hpp"

namespace logging
{

std::mutex Logger::logMutex;

Logger Log::debug("Debug", false);
Logger Log::trace("Trace", false);
Logger Log::info("Info", false);
Logger Log::warn("Warn", true);
Logger Log::error("Error", true);

} // namespace logging
