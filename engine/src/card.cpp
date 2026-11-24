#include "gto/card.h"

#include <cctype>

namespace gto {

namespace {

constexpr std::string_view kRanks = "23456789TJQKA";
constexpr std::string_view kSuits = "cdhs";

}  // namespace

int rank_from_char(char c) noexcept {
  const auto rank = kRanks.find(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
  return rank == std::string_view::npos ? -1 : static_cast<int>(rank);
}

char rank_to_char(int rank) noexcept { return kRanks[static_cast<std::size_t>(rank)]; }

std::string Card::to_string() const {
  return {kRanks[static_cast<std::size_t>(rank())], kSuits[static_cast<std::size_t>(suit())]};
}

std::optional<Card> Card::parse(std::string_view text) noexcept {
  if (text.size() != 2) return std::nullopt;
  const char r = static_cast<char>(std::toupper(static_cast<unsigned char>(text[0])));
  const char s = static_cast<char>(std::tolower(static_cast<unsigned char>(text[1])));
  const auto rank = kRanks.find(r);
  const auto suit = kSuits.find(s);
  if (rank == std::string_view::npos || suit == std::string_view::npos) return std::nullopt;
  return Card(static_cast<int>(rank), static_cast<int>(suit));
}

std::optional<std::vector<Card>> parse_cards(std::string_view text) {
  std::vector<Card> cards;
  std::uint64_t seen = 0;
  std::size_t i = 0;
  while (i < text.size()) {
    if (std::isspace(static_cast<unsigned char>(text[i])) || text[i] == ',') {
      ++i;
      continue;
    }
    if (i + 2 > text.size()) return std::nullopt;
    const auto card = Card::parse(text.substr(i, 2));
    if (!card || (seen & card->mask()) != 0) return std::nullopt;
    seen |= card->mask();
    cards.push_back(*card);
    i += 2;
  }
  return cards;
}

}  // namespace gto
