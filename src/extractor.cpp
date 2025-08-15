#include "extractor.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <regex>
#include <stdexcept>
#include <string>
#include <unordered_set>

namespace {

bool commandExists(const std::string& command) {
  std::string test = "command -v " + command + " >/dev/null 2>&1";
  int rc = std::system(test.c_str());
  return rc == 0;
}

std::string trim(const std::string& s) {
  size_t start = 0;
  while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) start++;
  size_t end = s.size();
  while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) end--;
  return s.substr(start, end - start);
}

std::string runPdftotextCaptureStdout(const std::string& pdfPath) {
  std::string cmd = "pdftotext -layout -nopgbrk -q \"" + pdfPath + "\" -";
  std::string output;

  FILE* pipe = popen(cmd.c_str(), "r");
  if (!pipe) {
    throw std::runtime_error("Failed to open pipe to pdftotext");
  }

  char buffer[4096];
  while (true) {
    size_t n = std::fread(buffer, 1, sizeof(buffer), pipe);
    if (n > 0) output.append(buffer, n);
    if (n < sizeof(buffer)) break;
  }

  int rc = pclose(pipe);
  if (rc != 0) {
    throw std::runtime_error("pdftotext returned non-zero exit code");
  }

  return output;
}

} // namespace

std::string extractPdfText(const std::string& pdfPath) {
  if (!commandExists("pdftotext")) {
    throw std::runtime_error(
      "pdftotext not found. Please install poppler-utils (e.g., apt-get install -y poppler-utils)."
    );
  }
  return runPdftotextCaptureStdout(pdfPath);
}

ExtractedData parseExtractedText(const std::string& text) {
  ExtractedData data;

  std::regex lineBreak("\r?\n");
  std::sregex_token_iterator it(text.begin(), text.end(), lineBreak, -1);
  std::sregex_token_iterator end;
  for (; it != end; ++it) {
    std::string line = trim(*it);
    if (!line.empty()) data.lines.push_back(line);
  }

  std::regex kvPattern("^\\s*([A-Za-z][\\w\\s./#-]{0,64})\\s*:\\s*(.+)$");
  std::regex emailPattern("[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,}");
  std::regex phonePattern("(?:\\+?\\d{1,3}[\\s.-]?)?(?:\\(\\d{2,4}\\)|\\d{2,4})[\\s.-]?\\d{3,4}[\\s.-]?\\d{3,4}");
  std::regex datePattern("(\\b\\d{4}[-/]\\d{1,2}[-/]\\d{1,2}\\b)|(\\b\\d{1,2}[-/]\\d{1,2}[-/]\\d{2,4}\\b)");
  std::regex currencyPattern("(?:USD|EUR|GBP|INR|JPY|CNY|CAD|AUD)?\\s?[$€£₹]?-?\\d{1,3}(?:,?\\d{3})*(?:\\.\\d{2})?");

  std::unordered_set<std::string> seenEmail;
  std::unordered_set<std::string> seenPhone;
  std::unordered_set<std::string> seenDate;
  std::unordered_set<std::string> seenAmount;

  for (const std::string& line : data.lines) {
    std::smatch m;
    if (std::regex_match(line, m, kvPattern)) {
      std::string key = trim(m[1].str());
      std::string value = trim(m[2].str());
      if (!key.empty() && !value.empty()) {
        data.keyValues[key] = value;
      }
    }

    auto scanMatches = [&](const std::regex& re, std::unordered_set<std::string>& seen, std::vector<std::string>& out) {
      auto begin = std::sregex_iterator(line.begin(), line.end(), re);
      auto rend = std::sregex_iterator();
      for (auto it2 = begin; it2 != rend; ++it2) {
        std::string found = trim(it2->str());
        if (!found.empty() && seen.insert(found).second) out.push_back(found);
      }
    };

    scanMatches(emailPattern, seenEmail, data.emails);
    scanMatches(phonePattern, seenPhone, data.phoneNumbers);
    scanMatches(datePattern, seenDate, data.dates);
    scanMatches(currencyPattern, seenAmount, data.currencyAmounts);
  }

  return data;
}