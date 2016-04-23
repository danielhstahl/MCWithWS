#ifndef __BLACKSCHOLES_H_INCLUDED__
#define __BLACKSCHOLES_H_INCLUDED__
#include <cmath>
#include "AutoDiff.h"

auto BSCall(
    const auto&,/*underlying*/
    const auto&, /*discount factor (in pure BS, discount=e^{-rt}) */
    const auto&, /*strike*/
    const auto& /*volatility*/
);
auto BSPut(
    const auto&,/*underlying*/
    const auto&, /*discount factor (in pure BS, discount=e^{-rt}) */
    const auto&, /*strike*/
    const auto& /*volatility*/
);
#include "BlackScholes.hpp"

#endif
