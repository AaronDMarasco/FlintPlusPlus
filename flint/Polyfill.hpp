#pragma once

/**
 * This file contains the definitions and utility functions
 * that were hidden away in boost or facebook's folly library
 */

#include <string>
#include <vector>

namespace flint {

#ifdef _MSC_VER
const std::string FS_SEP{"\\"};
#define NOEXCEPT
#else
const std::string FS_SEP{"/"};
#define NOEXCEPT noexcept
#endif

// File System object types
enum FSType { NO_ACCESS, IS_FILE, IS_DIR };

auto fsObjectExists(const std::string& path) -> FSType;

auto fsContainsNoLint(const std::string& path) -> bool;

auto fsGetDirContents(const std::string& path, std::vector<std::string>& dir) -> bool;

auto getFileContents(const std::string& path, std::string& file) -> bool;

#if 0
bool startsWith(const std::string &str, const std::string &prefix);
#endif

auto startsWith(std::string::const_iterator str_iter, const char* prefix) -> bool;

auto escapeString(const std::string& input) -> std::string;
};  // namespace flint
