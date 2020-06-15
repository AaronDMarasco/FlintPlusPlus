#pragma once

/**
 * This file contains the definitions and utility functions
 * that were hidden away in boost or facebook's folly library
 */

#include <string>
#include <vector>

namespace flint {

#ifdef _MSC_VER
static const std::string FS_SEP{"\\"};
#define NOEXCEPT
#else
static const std::string FS_SEP{"/"};
#define NOEXCEPT noexcept
#endif

// File System object types
enum FSType { NO_ACCESS, IS_FILE, IS_DIR };

FSType fsObjectExists(const std::string& path);

bool fsContainsNoLint(const std::string& path);

bool fsGetDirContents(const std::string& path, std::vector<std::string>& dir);

bool getFileContents(const std::string& path, std::string& file);

#if 0
bool startsWith(const std::string &str, const std::string &prefix);
#endif

bool startsWith(std::string::const_iterator str_iter, const char* prefix);

std::string escapeString(const std::string& input);
};  // namespace flint
