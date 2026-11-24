#include "gto/range.h"

#include <algorithm>
#include <charconv>
#include <cstdio>
#include <vector>

namespace gto {

namespace {

enum class Kind { Pair, Suited, Offsuit, Both };

struct HandClass {
  int high = 0;
  int low = 0;
  Kind kind = Kind::Pair;
};

std::string_view trim(std::string_view text) {
  const auto is_space = [](char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; };
  while (!text.empty() && is_space(text.front())) text.remove_prefix(1);
  while (!text.empty() && is_space(text.back())) text.remove_suffix(1);
  return text;
}

std::optional<HandClass> parse_class(std::string_view text) {
  if (text.size() != 2 && text.size() != 3) return std::nullopt;
  const int a = rank_from_char(text[0]);
  const int b = rank_from_char(text[1]);
  if (a < 0 || b < 0) return std::nullopt;

  HandClass result;
  result.high = std::max(a, b);
  result.low = std::min(a, b);
  if (a == b) {
    if (text.size() != 2) return std::nullopt;
    result.kind = Kind::Pair;
  } else if (text.size() == 2) {
    result.kind = Kind::Both;
  } else if (text[2] == 's' || text[2] == 'S') {
    result.kind = Kind::Suited;
  } else if (text[2] == 'o' || text[2] == 'O') {
    result.kind = Kind::Offsuit;
  } else {
    return std::nullopt;
  }
  return result;
}

void apply_class(Range& range, const HandClass& hand, float weight) {
  for (int s1 = 0; s1 < Card::kNumSuits; ++s1) {
    for (int s2 = 0; s2 < Card::kNumSuits; ++s2) {
      bool include = false;
      switch (hand.kind) {
        case Kind::Pair: include = s1 < s2; break;
        case Kind::Suited: include = s1 == s2; break;
        case Kind::Offsuit: include = s1 != s2; break;
        case Kind::Both: include = true; break;
      }
      if (include) range.set_weight(Card(hand.high, s1), Card(hand.low, s2), weight);
    }
  }
}

/// Applies one item without its weight suffix. Returns false if malformed.
bool apply_item(Range& range, std::string_view item, float weight) {
  // Exact combo, e.g. "AhKh".
  if (item.size() == 4) {
    const auto a = Card::parse(item.substr(0, 2));
    const auto b = Card::parse(item.substr(2, 2));
    if (a && b) {
      if (*a == *b) return false;
      range.set_weight(*a, *b, weight);
      return true;
    }
  }

  // "X+": everything from X upwards.
  if (!item.empty() && item.back() == '+') {
    const auto hand = parse_class(item.substr(0, item.size() - 1));
    if (!hand) return false;
    if (hand->kind == Kind::Pair) {
      for (int rank = hand->high; rank < Card::kNumRanks; ++rank) {
        apply_class(range, {rank, rank, Kind::Pair}, weight);
      }
    } else {
      for (int low = hand->low; low < hand->high; ++low) {
        apply_class(range, {hand->high, low, hand->kind}, weight);
      }
    }
    return true;
  }

  // "X-Y": everything between X and Y.
  if (const auto dash = item.find('-'); dash != std::string_view::npos) {
    const auto first = parse_class(trim(item.substr(0, dash)));
    const auto second = parse_class(trim(item.substr(dash + 1)));
    if (!first || !second || first->kind != second->kind) return false;
    if (first->kind == Kind::Pair) {
      const auto [low, high] = std::minmax(first->high, second->high);
      for (int rank = low; rank <= high; ++rank) {
        apply_class(range, {rank, rank, Kind::Pair}, weight);
      }
    } else {
      if (first->high != second->high) return false;
      const auto [low, high] = std::minmax(first->low, second->low);
      for (int kicker = low; kicker <= high; ++kicker) {
        apply_class(range, {first->high, kicker, first->kind}, weight);
      }
    }
    return true;
  }

  const auto hand = parse_class(item);
  if (!hand) return false;
  apply_class(range, *hand, weight);
  return true;
}

std::string format_weight(float weight) {
  char buffer[32];
  std::snprintf(buffer, sizeof(buffer), "%.4g", static_cast<double>(weight));
  return buffer;
}

void append_item(std::string& out, const std::string& name, float weight) {
  if (!out.empty()) out += ',';
  out += name;
  if (weight != 1.0f) {
    out += ':';
    out += format_weight(weight);
  }
}

struct ComboTable {
  std::array<std::pair<std::uint8_t, std::uint8_t>, Range::kNumCombos> cards{};
  ComboTable() {
    std::size_t index = 0;
    for (int high = 1; high < Card::kDeckSize; ++high) {
      for (int low = 0; low < high; ++low) {
        cards[index++] = {static_cast<std::uint8_t>(low), static_cast<std::uint8_t>(high)};
      }
    }
  }
};

}  // namespace

int Range::combo_index(Card a, Card b) noexcept {
  const int low = std::min(a.index(), b.index());
  const int high = std::max(a.index(), b.index());
  return high * (high - 1) / 2 + low;
}

std::pair<Card, Card> Range::combo_cards(int index) noexcept {
  static const ComboTable table;
  const auto [low, high] = table.cards[static_cast<std::size_t>(index)];
  return {Card(low), Card(high)};
}

std::optional<Range> Range::parse(std::string_view text, std::string* error) {
  Range range;
  std::size_t start = 0;
  while (start <= text.size()) {
    std::size_t end = text.find(',', start);
    if (end == std::string_view::npos) end = text.size();
    const std::string_view token = trim(text.substr(start, end - start));
    start = end + 1;
    if (token.empty()) continue;

    std::string_view item = token;
    float weight = 1.0f;
    if (const auto colon = token.rfind(':'); colon != std::string_view::npos) {
      item = trim(token.substr(0, colon));
      const std::string_view number = trim(token.substr(colon + 1));
      double value = 0;
      const auto result = std::from_chars(number.data(), number.data() + number.size(), value);
      if (number.empty() || result.ec != std::errc{} ||
          result.ptr != number.data() + number.size() || !(value >= 0.0 && value <= 1.0)) {
        if (error) *error = "bad weight in range item '" + std::string(token) + "'";
        return std::nullopt;
      }
      weight = static_cast<float>(value);
    }

    if (!apply_item(range, item, weight)) {
      if (error) *error = "cannot read range item '" + std::string(token) + "'";
      return std::nullopt;
    }
  }
  return range;
}

std::string Range::to_string() const {
  std::string out;
  const auto emit = [&](int high, int low, Kind kind) {
    std::vector<int> combos;
    for (int s1 = 0; s1 < Card::kNumSuits; ++s1) {
      for (int s2 = 0; s2 < Card::kNumSuits; ++s2) {
        const bool include = kind == Kind::Pair ? s1 < s2 : kind == Kind::Suited ? s1 == s2 : s1 != s2;
        if (include) combos.push_back(combo_index(Card(high, s1), Card(low, s2)));
      }
    }
    const float first = weight(combos.front());
    const bool uniform =
        std::all_of(combos.begin(), combos.end(), [&](int c) { return weight(c) == first; });
    if (uniform) {
      if (first <= 0) return;
      std::string name{rank_to_char(high), rank_to_char(low)};
      if (kind == Kind::Suited) name += 's';
      if (kind == Kind::Offsuit) name += 'o';
      append_item(out, name, first);
      return;
    }
    for (const int combo : combos) {
      if (weight(combo) <= 0) continue;
      const auto [a, b] = combo_cards(combo);
      // Higher rank first reads more naturally: "AhKh", not "KhAh".
      append_item(out, b.to_string() + a.to_string(), weight(combo));
    }
  };

  for (int high = Card::kNumRanks - 1; high >= 0; --high) {
    emit(high, high, Kind::Pair);
    for (int low = high - 1; low >= 0; --low) {
      emit(high, low, Kind::Suited);
      emit(high, low, Kind::Offsuit);
    }
  }
  return out;
}

double Range::total_weight() const noexcept {
  double total = 0;
  for (const float weight : weights_) total += weight;
  return total;
}

std::string hand_class_name(Card a, Card b) {
  const Card high = a.rank() >= b.rank() ? a : b;
  const Card low = a.rank() >= b.rank() ? b : a;
  std::string name{rank_to_char(high.rank()), rank_to_char(low.rank())};
  if (high.rank() != low.rank()) name += high.suit() == low.suit() ? 's' : 'o';
  return name;
}

}  // namespace gto
