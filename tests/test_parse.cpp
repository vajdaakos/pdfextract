#include <catch2/catch_all.hpp>

#include "extractor.hpp"

#include <string>

TEST_CASE("parseExtractedText extracts key-values and entities", "[parse]") {
  const std::string sampleText =
    "Name: John Doe\n"
    "Email: john.doe@example.com\n"
    "+1-415-555-1234\n"
    "Invoice Date: 2024-07-05\n"
    "Total: $1,234.56\n"
    "Contact: john.doe@example.com\n";

  ExtractedData data = parseExtractedText(sampleText);

  REQUIRE(data.keyValues.at("Name") == "John Doe");
  REQUIRE(data.keyValues.at("Invoice Date") == "2024-07-05");
  REQUIRE(data.keyValues.at("Total") == "$1,234.56");

  REQUIRE_FALSE(data.emails.empty());
  REQUIRE(std::find(data.emails.begin(), data.emails.end(), "john.doe@example.com") != data.emails.end());
  // Deduplication expected
  size_t emailCount = std::count(data.emails.begin(), data.emails.end(), "john.doe@example.com");
  REQUIRE(emailCount == 1);

  // Phone
  auto phoneIt = std::find(data.phoneNumbers.begin(), data.phoneNumbers.end(), "+1-415-555-1234");
  REQUIRE(phoneIt != data.phoneNumbers.end());

  // Date and amount entity lists should include the parsed values as well
  REQUIRE(std::find(data.dates.begin(), data.dates.end(), "2024-07-05") != data.dates.end());
  REQUIRE(std::find(data.currencyAmounts.begin(), data.currencyAmounts.end(), "$1,234.56") != data.currencyAmounts.end());

  // Lines captured
  REQUIRE(data.lines.size() >= 5);
}