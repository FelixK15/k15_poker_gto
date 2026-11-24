#pragma once

#include <chrono>
#include <functional>
#include <thread>

#include "gto/v1/common.pb.h"

namespace gto::server::testing {

/// Nuts or air against bluff-catchers: tiny, solves in milliseconds, and has
/// a known answer (EV 70 / 30, IP bets 30% of the time).
inline v1::Spot quick_spot() {
  v1::Spot spot;
  spot.set_board("Kc 8d 5h 3s 2d");
  spot.set_oop_range("88");
  spot.set_ip_range("KK, QJo");
  spot.set_pot(100);
  spot.set_effective_stack(100);
  spot.mutable_tree()->mutable_river()->mutable_ip()->add_bet_pct(100);
  return spot;
}

/// A turn spot large enough that a solve keeps running until it is stopped.
inline v1::Spot slow_spot() {
  v1::Spot spot;
  spot.set_board("Ks Ts 7h 4h");
  spot.set_oop_range("TT,77,44,AKo,KQs,KJs,QJs,J9s,98s,A5s,65s");
  spot.set_ip_range("AA,KK,KTs,AQs,QTs,JTs,T9s,87s,A4s,99,88");
  spot.set_pot(100);
  spot.set_effective_stack(300);
  for (v1::StreetSizing* street :
       {spot.mutable_tree()->mutable_turn(), spot.mutable_tree()->mutable_river()}) {
    for (v1::BetSizing* sizing : {street->mutable_oop(), street->mutable_ip()}) {
      sizing->add_bet_pct(66);
      sizing->add_raise_pct(75);
    }
  }
  spot.mutable_tree()->set_raise_cap(1);
  return spot;
}

/// Polls until `done` holds. Returns false on timeout.
inline bool wait_until(const std::function<bool()>& done,
                       std::chrono::milliseconds timeout = std::chrono::seconds(30)) {
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while (!done()) {
    if (std::chrono::steady_clock::now() > deadline) return false;
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  return true;
}

}  // namespace gto::server::testing
