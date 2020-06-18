#include <iostream>
#include <string>

#include "Checks.hpp"
#include "ErrorReport.hpp"
#include "FileCategories.hpp"
#include "Ignored.hpp"
#include "Options.hpp"

using namespace std;
using namespace flint;

/**
 * Run lint on the given path
 *
 * @param errors
 *        An object to hold the error details
 * @param path
 *        The path to lint
 * @param loc
 *        Reference to a var to count the estimated number
 *        of lines linted
 * @param depth
 *        Tracks the recursion depth
 * @return
 *        Returns the number of errors found
 */
void checkEntry(ErrorReport& errors, const string& path, size_t& loc, size_t depth = 0) {
  const auto fsType = fsObjectExists(path);
  if (fsType == FSType::NO_ACCESS) {
    if (0 == depth) fprintf(stderr, "Explicitly requested file/path '%s' does not exist.\n\n", path.c_str());
    return;
  }

  if (fsType == FSType::IS_DIR) {
    if ((!Options.RECURSIVE && depth > 0) || fsContainsNoLint(path)) return;

    // For each object in the directory
    vector<string> dirs;
    if (!fsGetDirContents(path, dirs)) return;

    for (const auto& dir: dirs) checkEntry(errors, dir, loc, depth + 1);

    return;
  }

  if (getFileCategory(path) == FileCategory::UNKNOWN) return;

  string fileContents;
  if (!getFileContents(path, fileContents)) return;

  // Remove code that occurs in pairs of
  // "// %flint: pause" & "// %flint: resume"
  fileContents = removeIgnoredCode(fileContents, path);

  try {
    ErrorFile errorFile((Options.VERBOSE ? path : getFileName(path)));

    vector<Token>  tokens;
    vector<size_t> structures;
    loc += tokenize(fileContents, path, tokens, structures, errorFile);

    // Checks which note Errors
    checkBlacklistedIdentifiers(errorFile, path, tokens);
    checkInitializeFromItself(errorFile, path, tokens);
    checkIfEndifBalance(errorFile, path, tokens);
    checkMemset(errorFile, path, tokens);
    checkIncludeAssociatedHeader(errorFile, path, tokens);
    checkIncludeGuard(errorFile, path, tokens);
    checkInlHeaderInclusions(errorFile, path, tokens);

    if (!Options.CMODE) {
      checkMutexHolderHasName(errorFile, path, tokens);
      checkConstructors(errorFile, path, tokens, structures);
      checkCatchByReference(errorFile, path, tokens);
      checkThrowsHeapException(errorFile, path, tokens);
      checkUniquePtrUsage(errorFile, path, tokens);
    }

    // Checks which note Warnings
    if (Options.LEVEL >= Lint::WARNING) {
      checkBlacklistedSequences(errorFile, path, tokens);
      checkDefinedNames(errorFile, path, tokens);
      checkDeprecatedIncludes(errorFile, path, tokens);
      checkNamespaceScopedStatics(errorFile, path, tokens);
      checkUsingNamespaceDirectives(errorFile, path, tokens);

      if (!Options.CMODE) {
        checkSmartPtrUsage(errorFile, path, tokens);
        checkImplicitCast(errorFile, path, tokens, structures);
        checkProtectedInheritance(errorFile, path, tokens, structures);
        checkExceptionInheritance(errorFile, path, tokens, structures);
        checkVirtualDestructors(errorFile, path, tokens, structures);

        checkThrowSpecification(errorFile, path, tokens, structures);
      }
    }

#if 0
    // Checks which note Advice
    if (Options.LEVEL >= Lint::ADVICE) {
      // Deprecated due to too many false positives
      // checkIncrementers(errorFile, path, tokens);

      if (!Options.CMODE) {
        // Merged into banned identifiers
        // checkUpcaseNull(errorFile, path, tokens);
      }
    }
#endif

    errors.addFile(move(errorFile));
  }
  catch (exception const& e) {
    fprintf(stderr, "Exception thrown during checks on %s.\n%s\n\n", path.c_str(), e.what());
  }
};

/**
 * Program entry point
 */
auto main(int argc, char* argv[]) -> int {
  // Parse commandline flags
  vector<string> paths;
  parseArgs(argc, argv, paths);

  size_t totalLOC = 0;
  // Check each file
  ErrorReport errors;
  for (auto& path: paths) checkEntry(errors, path, totalLOC);

  // Print summary
  errors.print();
  if (!Options.JSON) cout << endl << "Estimated Lines of Code: " << to_string(totalLOC) << endl;

#ifdef _DEBUG
  // Stop visual studio from closing the window...
  system("PAUSE");
#endif

  return errors.getWarnings() or errors.getErrors();  // POSIX standard is zero for success
};
