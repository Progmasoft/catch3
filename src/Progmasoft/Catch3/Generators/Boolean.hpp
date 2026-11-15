// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <Progmasoft/Catch3/Generator.hpp>

namespace Progmasoft::Catch3 {

    /// Generates booleans and shrinks true to the simplest value, false.
    [[nodiscard]] inline Generator<bool> Boolean() {
        return MakeGenerator<bool>(
            []( Random& random ) { return ( random.Next() & 1U ) != 0; },
            []( const bool& value ) {
                return value ? std::vector<bool>{ false } : std::vector<bool>{};
            } );
    }

} // namespace Progmasoft::Catch3
