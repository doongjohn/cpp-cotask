#pragma once

#include <source_location>
#include <string>
#include <format>

namespace cotask::utils {

inline auto with_location(std::string_view str,
                          const std::source_location loc = std::source_location::current()) -> std::string {
  const auto function_name = loc.function_name();
  const auto file_name = loc.file_name();
  const auto line = loc.line();
  const auto column = loc.column();
  return std::format("{}\n└> called from `{}` {}:{}:{}\n", str, function_name, file_name, line, column);
}

} // namespace cotask::utils
