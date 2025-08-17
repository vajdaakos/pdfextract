#include "extractor.hpp"
#include "table_extractor.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char** argv)
{
  try {
    std::string pdfPath;
    bool extractTables = false;
    std::string tablesOutDir = "tables_out";

    for (int i = 1; i < argc; ++i) {
      std::string arg = argv[i];
      if (arg == "--tables") {
        extractTables = true;
      } else if (arg.rfind("--tables-out=", 0) == 0) {
        tablesOutDir = arg.substr(std::string("--tables-out=").size());
      } else if (pdfPath.empty()) {
        pdfPath = arg;
      }
    }

    if (pdfPath.empty()) {
      pdfPath = "/workspace/sample.pdf";
    }

    if (!std::filesystem::exists(pdfPath)) {
      std::cerr << "PDF not found: " << pdfPath << "\n";
      std::cerr << "Usage: " << argv[0] << " [--tables] [--tables-out=dir] <pdf_path>\n";
      return 2;
    }

    if (extractTables) {
      auto tables = extractTablesFromPdf(pdfPath);
      writeTablesAsCsv(tables, tablesOutDir);
      std::cout << "Extracted " << tables.size() << " table(s) to '" << tablesOutDir << "'\n";
      return 0;
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
