#pragma once

#include <string>
#include <unordered_map>
#include <vector>

struct ExtractedData {
  std::unordered_map<std::string, std::string> keyValues;
  std::vector<std::string> emails;
  std::vector<std::string> phoneNumbers;
  std::vector<std::string> dates;
  std::vector<std::string> currencyAmounts;
  std::vector<std::string> lines;
};

// Returns full text of the PDF by invoking `pdftotext` if available.
// Throws std::runtime_error on failure.
std::string extractPdfText(const std::string& pdfPath);

// Parses the given text into structured fields using heuristics and optional rules.
ExtractedData parseExtractedText(const std::string& text);