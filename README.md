# PDF Data Extractor (C++)

This small C++17 project extracts text from a PDF using `pdftotext` (Poppler) and parses common fields into a C++ structure.

## Build

```bash
cmake -S . -B build
cmake --build build -j
```

## Run

```bash
# Default path assumed if not provided: /workspace/sample.pdf
./build/pdf_extract /workspace/sample.pdf
```

If `pdftotext` is not installed, install it (Debian/Ubuntu):

```bash
apt-get update && apt-get install -y poppler-utils
```

## Output
The program prints a JSON-like summary and also writes a minimal TSV of key-value pairs.

## Notes
- You can adapt the parsing logic in `src/extractor.cpp` to match your PDF format (e.g., add field-specific regexes).
- For best results, ensure the sample PDF contains selectable text (not scanned images). For scans, use OCR (e.g., `ocrmypdf`).