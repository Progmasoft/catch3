// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#pragma once

#include <catch2/catch_tostring.hpp>

#include <Progmasoft/Catch3/Generator.hpp>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <optional>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

namespace Progmasoft::Catch3 {

    /// Controls a property run without changing the compatibility runner's CLI.
    struct PropertyOptions final {
        std::size_t Trials = 100;
        std::uint64_t Seed = 0xC47C4A53ULL;
        std::size_t MaxShrinkSteps = 1000;
    };

    enum class PropertyStatus : std::uint8_t {
        kPassed,
        kFailed,
        kErrored,
        kInvalidConfiguration,
    };

    struct PropertyFailure final {
        std::uint64_t RootSeed = 0;
        std::uint64_t TrialSeed = 0;
        std::size_t Trial = 0;
        std::size_t ShrinkSteps = 0;
        std::string Counterexample;
        std::string Reason;
    };

    /// Stable, machine-readable outcome with a deterministic human-readable
    /// report.
    struct PropertyResult final {
        PropertyStatus Status = PropertyStatus::kPassed;
        std::size_t TrialsRun = 0;
        std::optional<PropertyFailure> Failure;
        std::string Error;

        [[nodiscard]] bool Succeeded() const noexcept {
            return Status == PropertyStatus::kPassed;
        }

        [[nodiscard]] std::string Describe() const {
            std::ostringstream output;
            switch ( Status ) {
            case PropertyStatus::kPassed:
                output << "property passed: " << TrialsRun << " trials";
                return output.str();
            case PropertyStatus::kInvalidConfiguration:
                output << "invalid property configuration: " << Error;
                return output.str();
            case PropertyStatus::kErrored:
                output << "property execution errored after " << TrialsRun
                       << " trials: " << Error;
                break;
            case PropertyStatus::kFailed:
                output << "property failed after " << TrialsRun << " trials";
                break;
            }

            if ( Failure.has_value() ) {
                output << "; failing trial " << Failure->Trial << ", root seed "
                       << Failure->RootSeed << ", replay seed "
                       << Failure->TrialSeed << ", shrink checks "
                       << Failure->ShrinkSteps
                       << "\ncounterexample: " << Failure->Counterexample
                       << "\nreason: " << Failure->Reason;
            }
            return output.str();
        }
    };

    namespace Detail {

        template <typename ValueType>
        [[nodiscard]] std::string
        StringifyPropertyValue( const ValueType& value ) {
            return Catch::StringMaker<std::remove_cvref_t<ValueType>>::convert(
                value );
        }

        template <typename Predicate, typename ValueType>
        [[nodiscard]] std::optional<std::string>
        EvaluateProperty( Predicate& predicate, const ValueType& value ) {
            try {
                if ( static_cast<bool>( std::invoke( predicate, value ) ) ) {
                    return std::nullopt;
                }
                return "predicate returned false";
            } catch ( const std::exception& exception ) {
                return std::string( "predicate threw std::exception: " ) +
                       exception.what();
            } catch ( ... ) {
                return "predicate threw a non-standard exception";
            }
        }

        template <typename ValueType>
        [[nodiscard]] std::string
        DescribeValue( const ValueType& value ) noexcept {
            try {
                return StringifyPropertyValue( value );
            } catch ( const std::exception& exception ) {
                return std::string( "<stringification failed: " ) +
                       exception.what() + ">";
            } catch ( ... ) {
                return "<stringification failed with a non-standard exception>";
            }
        }

    } // namespace Detail

    /// Runs independently seeded trials and greedily accepts failing shrink
    /// candidates.
    template <typename ValueType, typename Predicate>
    requires std::invocable<Predicate&, const ValueType&>&&
        std::convertible_to<std::invoke_result_t<Predicate&, const ValueType&>,
                            bool> [[nodiscard]] PropertyResult
        CheckProperty( const Generator<ValueType>& generator,
                       Predicate predicate,
                       PropertyOptions options = {} ) {
        if ( options.Trials == 0 ) {
            return { PropertyStatus::kInvalidConfiguration,
                     0,
                     std::nullopt,
                     "Trials must be greater than zero" };
        }

        PropertyResult result;
        for ( std::size_t trial = 0; trial < options.Trials; ++trial ) {
            result.TrialsRun = trial + 1;
            const std::uint64_t trialSeed =
                Random::DeriveSeed( options.Seed, trial );
            Random random( trialSeed );
            std::optional<ValueType> sample;

            try {
                sample.emplace( generator.Generate( random ) );
            } catch ( const std::exception& exception ) {
                result.Status = PropertyStatus::kErrored;
                result.Error =
                    std::string( "generator threw std::exception: " ) +
                    exception.what();
                result.Failure = PropertyFailure{ options.Seed,
                                                  trialSeed,
                                                  trial + 1,
                                                  0,
                                                  "<not generated>",
                                                  result.Error };
                return result;
            } catch ( ... ) {
                result.Status = PropertyStatus::kErrored;
                result.Error = "generator threw a non-standard exception";
                result.Failure = PropertyFailure{ options.Seed,
                                                  trialSeed,
                                                  trial + 1,
                                                  0,
                                                  "<not generated>",
                                                  result.Error };
                return result;
            }

            auto reason = Detail::EvaluateProperty( predicate, *sample );
            if ( !reason.has_value() ) { continue; }

            std::size_t shrinkChecks = 0;
            try {
                bool improved = true;
                while ( improved && shrinkChecks < options.MaxShrinkSteps ) {
                    improved = false;
                    const std::vector<ValueType> candidates =
                        generator.Shrink( *sample );
                    for ( std::size_t candidateIndex = 0;
                          candidateIndex < candidates.size();
                          ++candidateIndex ) {
                        if ( shrinkChecks >= options.MaxShrinkSteps ) { break; }
                        ++shrinkChecks;
                        const ValueType candidate = static_cast<ValueType>(
                            candidates[candidateIndex] );
                        auto candidateReason =
                            Detail::EvaluateProperty( predicate, candidate );
                        if ( candidateReason.has_value() ) {
                            sample.emplace( candidate );
                            reason = std::move( candidateReason );
                            improved = true;
                            break;
                        }
                    }
                }
            } catch ( const std::exception& exception ) {
                result.Status = PropertyStatus::kErrored;
                result.Error =
                    std::string( "shrinker threw std::exception: " ) +
                    exception.what();
                result.Failure =
                    PropertyFailure{ options.Seed,
                                     trialSeed,
                                     trial + 1,
                                     shrinkChecks,
                                     Detail::DescribeValue( *sample ),
                                     *reason };
                return result;
            } catch ( ... ) {
                result.Status = PropertyStatus::kErrored;
                result.Error = "shrinker threw a non-standard exception";
                result.Failure =
                    PropertyFailure{ options.Seed,
                                     trialSeed,
                                     trial + 1,
                                     shrinkChecks,
                                     Detail::DescribeValue( *sample ),
                                     *reason };
                return result;
            }

            result.Status = PropertyStatus::kFailed;
            result.Failure = PropertyFailure{ options.Seed,
                                              trialSeed,
                                              trial + 1,
                                              shrinkChecks,
                                              Detail::DescribeValue( *sample ),
                                              *reason };
            return result;
        }

        result.Status = PropertyStatus::kPassed;
        return result;
    }

} // namespace Progmasoft::Catch3
