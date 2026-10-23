// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#pragma once

#include <Progmasoft/Catch3/ForAll.hpp>
#include <Progmasoft/Catch3/Strategy/Context.hpp>
#include <functional>
#include <string>
#include <utility>

// Ready-made property bodies for the oracles that need no specification of
// the expected result: differential, metamorphic and round-trip testing.
// Each one is passed to ForAll in place of a hand-written body:
//
//     ForAll(Gen::Vectors(Gen::Integers<int>()),
//            Equivalent(ReferenceSort, FastSort));

namespace Progmasoft::Catch3
{

    /// Differential testing: the candidate must return what the reference
    /// returns for every input. A mismatch reports both results.
    template<typename Reference, typename Candidate>
    [[nodiscard]] auto
    Equivalent(Reference reference, Candidate candidate)
    {
        return [reference, candidate](const auto &...input) mutable {
            const auto expected = std::invoke(reference, input...);
            const auto actual = std::invoke(candidate, input...);
            if (!(expected == actual))
            {
                Note("reference: " + Detail::DescribeValue(expected));
                Note("candidate: " + Detail::DescribeValue(actual));
                Detail::RecordPropertyFailureMessage(
                    "the candidate disagrees with the reference");
            }
        };
    }

    /// Metamorphic testing: for every input x, relation(function(x),
    /// function(transform(x))) must hold, for example that sorting a
    /// shuffled list gives the same result, or that doubling a length
    /// doubles a cost.
    template<typename Function, typename Transform, typename Relation>
    [[nodiscard]] auto
    Metamorphic(Function function, Transform transform, Relation relation)
    {
        return [function, transform, relation](const auto &input) mutable {
            const auto source = std::invoke(function, input);
            const auto followUpInput = std::invoke(transform, input);
            const auto followUp = std::invoke(function, followUpInput);
            if (!static_cast<bool>(std::invoke(relation, source, followUp)))
            {
                Note("follow-up input: " + Detail::DescribeValue(followUpInput));
                Note("source output: " + Detail::DescribeValue(source));
                Note("follow-up output: " + Detail::DescribeValue(followUp));
                Detail::RecordPropertyFailureMessage(
                    "the metamorphic relation does not hold");
            }
        };
    }

    /// decode(encode(x)) == x for every input.
    template<typename Encode, typename Decode>
    [[nodiscard]] auto
    RoundTrip(Encode encode, Decode decode)
    {
        return [encode, decode](const auto &input) mutable {
            const auto encoded = std::invoke(encode, input);
            const auto decoded = std::invoke(decode, encoded);
            if (!(decoded == input))
            {
                Note("encoded: " + Detail::DescribeValue(encoded));
                Note("decoded: " + Detail::DescribeValue(decoded));
                Detail::RecordPropertyFailureMessage(
                    "decoding the encoded input did not give it back");
            }
        };
    }

    /// function(function(x)) == function(x) for every input.
    template<typename Function>
    [[nodiscard]] auto
    Idempotent(Function function)
    {
        return [function](const auto &input) mutable {
            const auto once = std::invoke(function, input);
            const auto twice = std::invoke(function, once);
            if (!(twice == once))
            {
                Note("once: " + Detail::DescribeValue(once));
                Note("twice: " + Detail::DescribeValue(twice));
                Detail::RecordPropertyFailureMessage(
                    "applying the function twice changed the result");
            }
        };
    }

} // namespace Progmasoft::Catch3
