// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#pragma once

#include <catch2/catch_tostring.hpp>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Progmasoft::Catch3
{

    /// What one run of a property body recorded besides its verdict.
    ///
    /// The engine installs a fresh record before each run of the body, on
    /// the running thread, and reads it afterwards. The functions below
    /// write into the record of the current thread; called outside a
    /// property they do nothing.
    struct TrialRecord final
    {
        struct Coverage final
        {
            std::string Label;
            double Required = 0.0;
            bool Held = false;
        };

        struct Score final
        {
            std::string Label;
            double Value = 0.0;
        };

        std::vector<std::string> Notes;
        std::vector<std::string> Labels;
        std::vector<Coverage> Covers;
        std::vector<Score> Targets;
        std::optional<std::string> Failure;
        bool Discarded = false;
    };

    namespace Detail
    {

        /// The record of the property body running on this thread, or null.
        [[nodiscard]] TrialRecord *
        CurrentTrialRecord() noexcept;

        /// Installs a record for the lifetime of the scope.
        class TrialRecordScope final
        {
        public:
            explicit TrialRecordScope(TrialRecord &record) noexcept;
            ~TrialRecordScope();
            TrialRecordScope(const TrialRecordScope &) = delete;
            TrialRecordScope &operator=(const TrialRecordScope &) = delete;

        private:
            TrialRecord *previous_;
        };

        void
        RecordPropertyFailure(std::string_view expression,
                              std::string_view file,
                              std::size_t line);

        /// Fails the current trial with a message, as the relation checks
        /// do when they compare two results.
        void
        RecordPropertyFailureMessage(std::string message);

        /// The name a property gets when PropertyOptions::Name is empty;
        /// the CATCH3_ macros set it to the test case name and the line.
        [[nodiscard]] std::string
        DefaultPropertyName();

        class PropertyNameScope final
        {
        public:
            explicit PropertyNameScope(std::string name);
            ~PropertyNameScope();
            PropertyNameScope(const PropertyNameScope &) = delete;
            PropertyNameScope &operator=(const PropertyNameScope &) = delete;

        private:
            std::string previous_;
        };

    } // namespace Detail

    /// Adds a line to the failure report of the current trial.
    void
    Note(std::string text);

    /// Adds a value to the failure report, rendered as Catch2 renders it.
    template<typename ValueType>
    void
    Note(std::string_view name, const ValueType &value)
    {
        Note(std::string(name) + " = "
             + Catch::StringMaker<ValueType>::convert(value));
    }

    /// Counts the current trial under label; the counts are reported with
    /// PropertyOptions::ShowStatistics and with a Cover failure.
    void
    Classify(bool condition, std::string label);

    /// Requires that condition holds in at least requiredPercentage of the
    /// trials; otherwise the property fails with kInsufficientCoverage.
    void
    Cover(double requiredPercentage, bool condition, std::string label);

    /// Reports a score that the engine tries to maximize after the regular
    /// trials: it mutates the highest-scoring input to search for a higher
    /// score, which finds inputs that random sampling rarely reaches.
    void
    Target(double score, std::string label = {});

} // namespace Progmasoft::Catch3

/// Fails the current property trial, without unwinding, when the condition
/// is false. Use it in a property body that returns void.
#define CATCH3_PROPERTY_ASSERT( ... )                                     \
    do {                                                                  \
        if ( !static_cast<bool>( __VA_ARGS__ ) ) {                        \
            ::Progmasoft::Catch3::Detail::RecordPropertyFailure(          \
                #__VA_ARGS__, __FILE__, static_cast<std::size_t>( __LINE__ ) ); \
            return;                                                       \
        }                                                                 \
    } while ( false )

/// Discards the current trial when the condition is false. A discarded
/// trial is neither a pass nor a failure; see PropertyOptions::MaxDiscardRatio.
#define CATCH3_PROPERTY_ASSUME( ... )                                     \
    do {                                                                  \
        if ( !static_cast<bool>( __VA_ARGS__ ) ) {                        \
            if ( auto *catch3Record =                                     \
                     ::Progmasoft::Catch3::Detail::CurrentTrialRecord() ) \
                catch3Record->Discarded = true;                           \
            return;                                                       \
        }                                                                 \
    } while ( false )
