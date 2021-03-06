#pragma once
#include <array>
#include <cassert>
#include <iostream>
#include <string>

#include "Options.hpp"
#include "Polyfill.hpp"
namespace flint {

/*
 * Class to represent a single "Error" that was found during linting
 */
class ErrorObject {
 private:
  // Members
  const Lint        m_type;
  const size_t      m_line;
  const std::string m_title, m_desc;

 public:
  // Constructor
  ErrorObject(Lint type, size_t line, std::string title, std::string desc)
      : m_type(type), m_line(line), m_title(move(title)), m_desc(move(desc)){};

  // Getter
  auto getType() const -> size_t { return m_type; };

  /*
   * Prints a single error of the report in either
   * JSON or Pretty Printed format
   *
   */
  void print(const std::string& path) const {
    static constexpr std::array<const char*, 3> levelStr{"[Error  ] ", "[Warning] ", "[Advice ] "};
    static constexpr std::array<const char*, 3> levelStrJSON{"Error", "Warning", "Advice"};

    if (Options.LEVEL < m_type) return;
    assert(m_type <= 3);

    // clang-format off
    if (Options.JSON) {
      std::cout <<
                "        {\n"
                "\t        \"level\"    : \"" << levelStrJSON[m_type]   << "\",\n"
                "\t        \"line\"     : "   << std::to_string(m_line) << ",\n"
                "\t        \"title\"    : \"" << escapeString(m_title)  << "\",\n"
                "\t        \"desc\"     : \"" << escapeString(m_desc)   << "\"\n"
                "        }";
      return;
    }
    // clang-format on
    std::cout << levelStr[m_type] << path << ':' << std::to_string(m_line) << ": " << m_title << std::endl;
  };
};

/*
 * Base Class for ErrorFile and ErrorReport which both have error counts
 */
class ErrorBase {
 protected:
  // Members
  size_t m_errors{0}, m_warnings{0}, m_advice{0};

 public:
  auto getErrors() const -> size_t { return m_errors; };
  auto getWarnings() const -> size_t { return m_warnings; };
  auto getAdvice() const -> size_t { return m_advice; };
  auto getTotal() const -> size_t { return m_advice + m_warnings + m_errors; };
};

/*
 * Class to represent a single file's "Errors" that were found during linting
 */
class ErrorFile: public ErrorBase {
 private:
  // Members
  std::vector<ErrorObject> m_objs;
  const std::string        m_path;

 public:
  explicit ErrorFile(std::string path): ErrorBase(), m_path(move(path)){};

  void addError(ErrorObject&& error) {
    switch (error.getType()) {
      case Lint::WARNING:
        ++m_warnings;
        break;
      case Lint::ADVICE:
        ++m_advice;
        break;
      default:  // Lint::ERROR
        ++m_errors;
    }
    m_objs.push_back(std::move(error));
  };

  /*
   * Prints a single file of the report in either
   * JSON or Pretty Printed format
   */
  void print() const {
    // clang-format off
    if (Options.JSON) {
      std::cout <<
                "    {\n"
                "\t    \"path\"     : \"" << escapeString(m_path)          << "\",\n"
                "\t    \"errors\"   : "   << std::to_string(getErrors())   << ",\n"
                "\t    \"warnings\" : "   << std::to_string(getWarnings()) << ",\n"
                "\t    \"advice\"   : "   << std::to_string(getAdvice())   << ",\n"
                "\t    \"reports\"  : [\n";
      // clang-format on
      for (size_t i = 0, size = m_objs.size(); i < size; ++i) {
        if (i > 0) std::cout << ',' << std::endl;
        m_objs[i].print(m_path);
      }
      std::cout << "\n      ]\n    }";

      return;
    }

    for (const auto& m_obj: m_objs) { m_obj.print(m_path); }
  };
};

/*
 * Class to represent the whole report and all "Errors" that were found during linting
 */
class ErrorReport: public ErrorBase {
 private:
  // Members
  std::vector<ErrorFile> m_files;

 public:
  void addFile(ErrorFile file) {
    m_errors += file.getErrors();
    m_warnings += file.getWarnings();
    m_advice += file.getAdvice();

    m_files.push_back(std::move(file));
  };

  /*
   * Prints an entire report in either
   * JSON or Pretty Printed format
   */
  void print() const {
    if (Options.JSON) {
      // clang-format off
      std::cout << "{\n"
                "\t\"errors\"   : " << std::to_string(getErrors())   << ",\n"
                "\t\"warnings\" : " << std::to_string(getWarnings()) << ",\n"
                "\t\"advice\"   : " << std::to_string(getAdvice())   << ",\n"
                "\t\"files\"    : [\n";
      // clang-format on
      for (size_t i = 0, size = m_files.size(); i < size; ++i) {
        if (i > 0) std::cout << ',' << std::endl;
        m_files[i].print();
      }
      std::cout << "\n  ]\n}";

      return;
    }

    for (const auto& m_file: m_files) {
      if (m_file.getTotal() > 0) { m_file.print(); }
    }

    std::cout << "\nLint Summary: " << std::to_string(m_files.size())
              << " files\nErrors: " << std::to_string(getErrors());

    if (Options.LEVEL >= Lint::WARNING) std::cout << " Warnings: " << std::to_string(getWarnings());
    if (Options.LEVEL >= Lint::ADVICE) std::cout << " Advice: " << std::to_string(getAdvice());
    std::cout << std::endl;
  };
};

};  // namespace flint
