#pragma once

#include <string>
#include <vector>

struct Table {
  int pageNumber;
  std::vector<std::string> headers;
  std::vector<std::vector<std::string>> rows;
};

// Extract tables by invoking `pdftotext -bbox-layout` to get word bounding boxes,
// then clustering words into rows/columns heuristically.
// If lastPage < firstPage or lastPage == -1, processes until end.
std::vector<Table> extractTablesFromPdf(const std::string& pdfPath,
                                        int firstPage = 1,
                                        int lastPage = -1);

// Write tables into CSV files in outDir as table_<page>_<index>.csv
void writeTablesAsCsv(const std::vector<Table>& tables, const std::string& outDir);