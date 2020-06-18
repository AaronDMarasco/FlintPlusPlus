#include "Polyfill.hpp"

#include <sys/stat.h>

#include <algorithm>
#include <fstream>
#include <sstream>

// Conditional includes for folder traversal
#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif

using namespace std;

namespace flint {

// Quick checks for path names (previously macros)
template<typename T>
inline auto fs_isnot_specialdir(const T& file) -> bool {  // was FS_ISNOT_LINK (???)
  return file.compare(".") and file.compare("..");
}

template<typename T>
inline auto fs_isnot_git(const T& file) -> bool {
  return file.compare(".git");
}

/**
 * Checks if a given path is a file or directory
 *
 * @param path
 *        The path to test
 * @return
 *        Returns a flag representing what the path was
 */
auto fsObjectExists(const string& path) -> FSType {
  struct stat info;
  if (stat(path.c_str(), &info))
    // Cannot Access
    return FSType::NO_ACCESS;
  if (info.st_mode & S_IFDIR)
    // Is a Directory
    return FSType::IS_DIR;
  if (info.st_mode & S_IFREG)
    // Is a File
    return FSType::IS_FILE;
  return FSType::NO_ACCESS;
};

/**
 * Checks if a given path contains a .nolint file
 *
 * @param path
 *        The path to test
 * @return
 *        Returns a bool of whether a .nolint file was found or not
 */
auto fsContainsNoLint(const string& path) -> bool {
  const string fileName{path + FS_SEP + ".nolint"};
  return fsObjectExists(fileName) == FSType::IS_FILE;
};

/**
 * Parses a directory and returns a list of its contents
 *
 * @param path
 *        The path to search
 * @param dirs
 *        A vector to fill with objects
 * @return
 *        Returns true if any valid objects were found
 */
auto fsGetDirContents(const string& path, vector<string>& dirs) -> bool {
  dirs.clear();

#ifdef _WIN32
  // windows.h Implementation of directory traversal for Windows systems
  HANDLE          dir;
  WIN32_FIND_DATA fileData;

  if ((dir = FindFirstFile((path + FS_SEP + "*").c_str(), &fileData)) == INVALID_HANDLE_VALUE) {
    return false; /* No files found */
  }

  do {
    const string fsObj = fileData.cFileName;

    if (fs_isnot_specialdir(fsObj) && fs_isnot_git(fsObj)) {
      const string fileName = path + FS_SEP + fsObj;
      dirs.push_back(move(fileName));
    }
  } while (FindNextFile(dir, &fileData));

  FindClose(dir);
#else
  // dirent.h Implementation of directory traversal for POSIX systems
  if (DIR* pDIR = opendir(path.c_str())) {
    while (struct dirent* entry = readdir(pDIR)) {
      const string fsObj{entry->d_name};
      if (fs_isnot_specialdir(fsObj) && fs_isnot_git(fsObj)) dirs.emplace_back(path + FS_SEP + fsObj);
    }
    closedir(pDIR);
  }

  stable_sort(dirs.begin(), dirs.end());
#endif
  return !dirs.empty();
};

/**
 * Attempts to load a file into a std::string
 *
 * @param path
 *        The file to load
 * @param file
 *        The string to load into
 * @return
 *        Returns a bool of whether the load was successful
 */
auto getFileContents(const string& path, string& file) -> bool {
  ifstream in(path);
  if (in) {
    stringstream buffer;
    buffer << in.rdbuf();
    file = buffer.str();

    return true;
  }
  return false;
};

#if 0
/**
 * Tests if a given string starts with a prefix
 *
 * @param str
 *        The string to search
 * @param prefix
 *        The prefix to search for
 * @return
 *        Returns true if str starts with an instance of prefix
 */
template <class T>
bool startsWith(const T& str, const T& prefix) {
  return equal(begin(prefix), end(prefix), begin(str));
};
#endif

/**
 * Tests if a given string starts with a C-string prefix
 *
 * @param str_iter
 *        The string position to start search
 * @param prefix
 *        The prefix (C-string) to search for
 * @return
 *        Returns true if str starts with an instance of prefix
 */
auto startsWith(string::const_iterator str_iter, const char* prefix) -> bool {
  while (*prefix != '\0' && *prefix == *str_iter) {
    ++prefix;
    ++str_iter;
  }

  return *prefix == '\0';
};

/**
 * Escapes a C++ std::string
 *
 * @param input
 *        The string to sanitize
 * @return
 *        Returns a string with no escape characters
 */
auto escapeString(const string& input) -> string {
  string output;
  output.reserve(input.length());

  for (const auto c: input) {
    switch (c) {
      case '\n':
        output += R"(\n)";
        break;
      case '\t':
        output += R"(\t)";
        break;
      case '\r':
        output += R"(\r)";
        break;
      case '\\':
        output += R"(\\)";
        break;
      case '"':
        output += R"(\")";
        break;

      default:
        output += c;
    }
  }
  return output;
};
};  // namespace flint
