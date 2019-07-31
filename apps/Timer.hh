/*
   Copyright (c) 2019, Triad National Security, LLC. All rights reserved.

   This is open source software; you can redistribute it and/or modify it
   under the terms of the BSD-3 License. If software is modified to produce
   derivative works, such modified software should be clearly marked, so as
   not to confuse it with the version available from LANL. Full text of the
   BSD-3 License can be found in the LICENSE file of the repository.
*/

#ifndef TIMER_HH
#define TIMER_HH 1

#include <cassert>
#include <chrono>
#include <cmath>
#include <ostream>

class Timer {
  using CLOCK_T = std::chrono::system_clock;
  using REP_T = double;
  using DURATION_T = std::chrono::duration<REP_T>;

public:
  constexpr Timer() : running_{false}, accum_{DURATION_T::zero()} {}
  void start() {
    assert(!running_);
    running_ = true;
    start_tp_ = CLOCK_T::now();
  }
  void stop() {
    std::chrono::time_point<CLOCK_T> const stop_tp = CLOCK_T::now();
    assert(running_);
    running_ = false;
    accum_ += stop_tp - start_tp_;
  }
  constexpr REP_T seconds() const {
    assert(!running_);
    return accum_.count();
  }
  constexpr void clear() {
    running_ = false;
    accum_ = DURATION_T::zero();
  }
  std::ostream &print(std::ostream &os) const {
    auto prec = os.precision(3);
    double s = seconds();
    if (s > 60) {
      double const m = std::floor(s / 60.0);
      s -= (m * 60);
      os << static_cast<int>(m) << 'm';
    }
    if (s < 0.0001)
      os << s * 1000000.0 << "us";
    else if (s < 0.1)
      os << s * 1000.0 << "ms";
    else
      os << s << 's';
    os.precision(prec);
    return os;
  }

private:
  bool running_;
  DURATION_T accum_;
  std::chrono::time_point<CLOCK_T> start_tp_;
};

inline std::ostream &operator<<(std::ostream &os, const Timer &t) {
  return t.print(os);
}

#endif
