#pragma once

#include <string>

namespace flint {

// Indentifiers for files marked for linting
enum FileCategory {
  HEADER,
  INL_HEADER,
  SOURCE_C,
  SOURCE_CPP,
  UNKNOWN,
};

// File identifying functions...
auto getFileCategory(const std::string& path) -> FileCategory;

auto isHeader(const std::string& path) -> bool;
auto isSource(const std::string& path) -> bool;

auto getFileNameBase(const std::string& path) -> std::string;
auto getFileName(const std::string& path) -> std::string;
};  // namespace flint
