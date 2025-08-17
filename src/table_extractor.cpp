#include "table_extractor.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <regex>
#include <stdexcept>
#include <string>

namespace {

struct WordBox {
  int pageNumber;
  double xMin;
  double yMin;
  double xMax;
  double yMax;
  std::string text;
};

std::string trim(const std::string& s) {
  size_t a = 0, b = s.size();
  while (a < b && std::isspace(static_cast<unsigned char>(s[a]))) a++;
  while (b > a && std::isspace(static_cast<unsigned char>(s[b-1]))) b--;
  return s.substr(a, b - a);
}

std::string decodeEntities(const std::string& in) {
  std::string out;
  out.reserve(in.size());
  for (size_t i = 0; i < in.size(); ++i) {
    if (in[i] == '&') {
      size_t j = in.find(';', i + 1);
      if (j != std::string::npos) {
        std::string ent = in.substr(i + 1, j - (i + 1));
        std::string rep;
        if (ent == "amp") rep = "&";
        else if (ent == "lt") rep = "<";
        else if (ent == "gt") rep = ">";
        else if (ent == "quot") rep = "\"";
        else if (ent == "apos") rep = "'";
        else if (!ent.empty() && ent[0] == '#') {
          try {
            if (ent.size() > 1 && (ent[1] == 'x' || ent[1] == 'X')) {
              unsigned int code = std::stoul(ent.substr(2), nullptr, 16);
              if (code <= 0x7F) rep.push_back(static_cast<char>(code));
            } else {
              unsigned int code = std::stoul(ent.substr(1), nullptr, 10);
              if (code <= 0x7F) rep.push_back(static_cast<char>(code));
            }
          } catch (...) {}
        }
        if (!rep.empty()) {
          out += rep; i = j; continue;
        }
      }
    }
    out.push_back(in[i]);
  }
  return out;
}

bool commandExists(const std::string& command) {
  std::string test = "command -v " + command + " >/dev/null 2>&1";
  return std::system(test.c_str()) == 0;
}

std::string runPdftotextBboxLayout(const std::string& pdfPath, int firstPage, int lastPage) {
  if (!commandExists("pdftotext")) {
    throw std::runtime_error("pdftotext not found; install poppler-utils");
  }
  std::string cmd = "pdftotext -bbox-layout";
  if (firstPage > 0) {
    cmd += " -f " + std::to_string(firstPage);
  }
  if (lastPage > 0) {
    cmd += " -l " + std::to_string(lastPage);
  }
  cmd += " -q \"" + pdfPath + "\" -";

  FILE* pipe = popen(cmd.c_str(), "r");
  if (!pipe) throw std::runtime_error("Failed to run pdftotext -bbox-layout");
  std::string out;
  char buf[8192];
  while (true) {
    size_t n = std::fread(buf, 1, sizeof(buf), pipe);
    if (n > 0) out.append(buf, n);
    if (n < sizeof(buf)) break;
  }
  int rc = pclose(pipe);
  if (rc != 0) throw std::runtime_error("pdftotext -bbox-layout returned error");
  return out;
}

std::vector<WordBox> parseWords(const std::string& xmlish) {
  std::vector<WordBox> words;
  // Capture page number from surrounding <page ... number="N"> when available; fall back to incremental.
  int currentPage = 1;
  std::regex pageOpen("<page[^>]*?number=\"([0-9]+)\"[^>]*>");
  std::regex pageClose("</page>");
  std::regex wordRe("<word[^>]*?xMin=\"([0-9.]+)\"[^>]*?yMin=\"([0-9.]+)\"[^>]*?xMax=\"([0-9.]+)\"[^>]*?yMax=\"([0-9.]+)\"[^>]*>(.*?)</word>");

  std::sregex_iterator it(xmlish.begin(), xmlish.end(), wordRe);
  std::sregex_iterator end;

  // For page tracking, we do a manual scan as well
  size_t pos = 0;
  while (pos < xmlish.size()) {
    std::smatch m;
    std::string tail = xmlish.substr(pos);
    if (std::regex_search(tail, m, pageOpen) && m.position() == 0) {
      currentPage = std::stoi(m[1].str());
      pos += m.position() + m.length();
      continue;
    }
    if (std::regex_search(tail, m, wordRe) && m.position() == 0) {
      WordBox w;
      w.pageNumber = currentPage;
      w.xMin = std::stod(m[1].str());
      w.yMin = std::stod(m[2].str());
      w.xMax = std::stod(m[3].str());
      w.yMax = std::stod(m[4].str());
      w.text = decodeEntities(m[5].str());
      words.push_back(std::move(w));
      pos += m.position() + m.length();
      continue;
    }
    if (std::regex_search(tail, m, pageClose) && m.position() == 0) {
      pos += m.position() + m.length();
      continue;
    }
    // Advance by one to avoid infinite loop on unmatched content
    pos += 1;
  }

  return words;
}

static double median(std::vector<double> v) {
  if (v.empty()) return 0.0;
  std::nth_element(v.begin(), v.begin() + v.size()/2, v.end());
  return v[v.size()/2];
}

struct RowGroup { double yCenter; std::vector<WordBox*> words; };

std::vector<RowGroup> clusterRows(std::vector<WordBox>& wordsOnPage) {
  std::vector<RowGroup> rows;
  if (wordsOnPage.empty()) return rows;

  std::vector<double> heights; heights.reserve(wordsOnPage.size());
  for (const auto& w : wordsOnPage) heights.push_back(w.yMax - w.yMin);
  double hMed = median(heights);
  double tol = hMed > 0 ? hMed * 0.8 : 6.0;

  std::sort(wordsOnPage.begin(), wordsOnPage.end(), [](const WordBox& a, const WordBox& b) {
    double ya = (a.yMin + a.yMax) * 0.5;
    double yb = (b.yMin + b.yMax) * 0.5;
    if (ya == yb) return a.xMin < b.xMin;
    return ya < yb; // bottom to top
  });

  for (auto& w : wordsOnPage) {
    double yc = (w.yMin + w.yMax) * 0.5;
    if (rows.empty() || std::abs(yc - rows.back().yCenter) > tol) {
      rows.push_back(RowGroup{yc, {}});
    }
    rows.back().words.push_back(&w);
    // update running yCenter as average
    rows.back().yCenter = (rows.back().yCenter * (rows.back().words.size() - 1) + yc) / rows.back().words.size();
  }

  // Sort words in each row by x
  for (auto& r : rows) {
    std::sort(r.words.begin(), r.words.end(), [](const WordBox* a, const WordBox* b){ return a->xMin < b->xMin; });
  }
  return rows;
}

std::vector<double> clusterColumns(const std::vector<RowGroup>& rows) {
  std::vector<double> centers;
  std::vector<double> widths;
  for (const auto& r : rows) {
    for (const auto* w : r.words) {
      centers.push_back((w->xMin + w->xMax) * 0.5);
      widths.push_back(w->xMax - w->xMin);
    }
  }
  if (centers.empty()) return {};
  double wMed = median(widths);
  double tol = std::max(8.0, wMed * 1.2);
  std::sort(centers.begin(), centers.end());
  std::vector<double> colCenters;
  double acc = centers.front();
  int count = 1;
  for (size_t i = 1; i < centers.size(); ++i) {
    if (centers[i] - centers[i-1] <= tol) {
      acc += centers[i]; count++;
    } else {
      colCenters.push_back(acc / count);
      acc = centers[i]; count = 1;
    }
  }
  colCenters.push_back(acc / count);
  return colCenters;
}

std::vector<std::vector<std::string>> buildGrid(const std::vector<RowGroup>& rows, const std::vector<double>& colCenters) {
  const size_t numCols = colCenters.size();
  std::vector<std::vector<std::string>> grid;
  grid.reserve(rows.size());
  for (const auto& r : rows) {
    std::vector<std::string> row(numCols);
    for (const auto* w : r.words) {
      // assign to nearest column center
      double xc = (w->xMin + w->xMax) * 0.5;
      size_t bestIdx = 0;
      double bestDist = std::abs(xc - colCenters[0]);
      for (size_t c = 1; c < numCols; ++c) {
        double d = std::abs(xc - colCenters[c]);
        if (d < bestDist) { bestDist = d; bestIdx = c; }
      }
      if (!row[bestIdx].empty()) row[bestIdx] += ' ';
      row[bestIdx] += w->text;
    }
    grid.push_back(std::move(row));
  }
  return grid;
}

} // namespace

std::vector<Table> extractTablesFromPdf(const std::string& pdfPath, int firstPage, int lastPage) {
  std::string xmlish = runPdftotextBboxLayout(pdfPath, firstPage, lastPage);
  std::vector<WordBox> words = parseWords(xmlish);

  // Group by page
  std::vector<Table> tables;
  std::map<int, std::vector<WordBox>> pageWords;
  for (auto& w : words) pageWords[w.pageNumber].push_back(std::move(w));

  for (auto& kv : pageWords) {
    int pageNo = kv.first;
    std::vector<RowGroup> rows = clusterRows(kv.second);
    if (rows.size() < 2) continue;
    std::vector<double> cols = clusterColumns(rows);
    if (cols.size() < 2) continue; // need at least 2 columns to be a table
    auto grid = buildGrid(rows, cols);

    Table t;
    t.pageNumber = pageNo;
    if (!grid.empty()) {
      t.headers = grid.front();
      for (size_t r = 1; r < grid.size(); ++r) t.rows.push_back(std::move(grid[r]));
    }
    tables.push_back(std::move(t));
  }

  // Sort tables by page
  std::sort(tables.begin(), tables.end(), [](const Table& a, const Table& b){ return a.pageNumber < b.pageNumber; });

  return tables;
}

void writeTablesAsCsv(const std::vector<Table>& tables, const std::string& outDir) {
  if (!std::filesystem::exists(outDir)) {
    std::filesystem::create_directories(outDir);
  }
  int indexPerPage = 0;
  int prevPage = -1;
  for (const auto& t : tables) {
    if (t.pageNumber != prevPage) { prevPage = t.pageNumber; indexPerPage = 0; }
    std::string filename = outDir + "/table_" + std::to_string(t.pageNumber) + "_" + std::to_string(indexPerPage++) + ".csv";
    std::ofstream ofs(filename);
    auto writeRow = [&](const std::vector<std::string>& row) {
      for (size_t i = 0; i < row.size(); ++i) {
        std::string cell = row[i];
        bool needQuotes = cell.find(',') != std::string::npos || cell.find('"') != std::string::npos || cell.find('\n') != std::string::npos;
        if (needQuotes) {
          std::string escaped;
          for (char ch : cell) {
            if (ch == '"') escaped += '"';
            escaped += ch;
          }
          ofs << '"' << escaped << '"';
        } else {
          ofs << cell;
        }
        if (i + 1 < row.size()) ofs << ',';
      }
      ofs << "\n";
    };

    writeRow(t.headers);
    for (const auto& r : t.rows) writeRow(r);
  }
}