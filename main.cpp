#include "extractor.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char** argv)
{
  try {
    std::string pdfPath;
    if (argc >= 2) {
      pdfPath = argv[1];
    } else {
      pdfPath = "/workspace/sample.pdf";
    }

    if (!std::filesystem::exists(pdfPath)) {
      std::cerr << "PDF not found: " << pdfPath << "\n";
      std::cerr << "Provide the PDF path as the first argument or place it at /workspace/sample.pdf\n";
      return 2;
    }

    std::string text = extractPdfText(pdfPath);
    ExtractedData data = parseExtractedText(text);

    // Print a compact JSON-like view
    std::cout << "{\n";
    std::cout << "  \"keyValues\": {\n";
    for (auto it = data.keyValues.begin(); it != data.keyValues.end(); ) {
      std::cout << "    \"" << it->first << "\": \"" << it->second << "\"";
      ++it;
      std::cout << (it == data.keyValues.end() ? "\n" : ",\n");
    }
    std::cout << "  },\n";

    auto printArray = [&](const char* name, const std::vector<std::string>& arr) {
      std::cout << "  \"" << name << "\": [";
      for (size_t i = 0; i < arr.size(); ++i) {
        std::cout << "\"" << arr[i] << "\"" << (i + 1 == arr.size() ? "" : ", ");
      }
      std::cout << "],\n";
    };

    printArray("emails", data.emails);
    printArray("phoneNumbers", data.phoneNumbers);
    printArray("dates", data.dates);
    printArray("currencyAmounts", data.currencyAmounts);

    std::cout << "  \"lines\": [";
    for (size_t i = 0; i < data.lines.size(); ++i) {
      std::cout << "\"" << data.lines[i] << "\"" << (i + 1 == data.lines.size() ? "" : ", ");
    }
    std::cout << "]\n";
    std::cout << "}\n";

    return 0;
  } catch (const std::exception& ex) {
    std::cerr << "Error: " << ex.what() << "\n";
    return 1;
  }
}
