// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#pragma once

#include <Progmasoft/Catch3/Random.hpp>

#include <functional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace Progmasoft::Catch3 {

    /// A value source paired with an ordered sequence of simpler candidates.
    ///
    /// The property runner owns the shrinking policy; generators only describe
    /// which candidates are valid and which order is most useful for
    /// minimization.
    template <typename ValueType>
    class Generator final {
    public:
        using GenerationFunction = std::function<ValueType( Random& )>;
        using ShrinkingFunction =
            std::function<std::vector<ValueType>( const ValueType& )>;

        Generator( GenerationFunction generate, ShrinkingFunction shrink ):
            generate_( std::move( generate ) ), shrink_( std::move( shrink ) ) {
            if ( !generate_ || !shrink_ ) {
                throw std::invalid_argument(
                    "a generator requires both callbacks" );
            }
        }

        [[nodiscard]] ValueType Generate( Random& random ) const {
            return generate_( random );
        }

        [[nodiscard]] std::vector<ValueType>
        Shrink( const ValueType& value ) const {
            return shrink_( value );
        }

    private:
        GenerationFunction generate_;
        ShrinkingFunction shrink_;
    };

    /// Adapts user callables into a reusable generator.
    template <typename ValueType,
              typename GenerateFunction,
              typename ShrinkFunction>
    [[nodiscard]] Generator<ValueType>
    MakeGenerator( GenerateFunction&& generate, ShrinkFunction&& shrink ) {
        return Generator<ValueType>(
            typename Generator<ValueType>::GenerationFunction(
                std::forward<GenerateFunction>( generate ) ),
            typename Generator<ValueType>::ShrinkingFunction(
                std::forward<ShrinkFunction>( shrink ) ) );
    }

} // namespace Progmasoft::Catch3
