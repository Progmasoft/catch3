// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#include <Progmasoft/Catch3/Strategy/Context.hpp>

#include <string>
#include <utility>

namespace Progmasoft::Catch3
{

    namespace
    {

        thread_local TrialRecord *currentRecord = nullptr;
        thread_local std::string defaultPropertyName;

    } // namespace

    namespace Detail
    {

        TrialRecord *
        CurrentTrialRecord() noexcept
        {
            return currentRecord;
        }

        TrialRecordScope::TrialRecordScope(TrialRecord &record) noexcept
            : previous_(currentRecord)
        {
            currentRecord = &record;
        }

        TrialRecordScope::~TrialRecordScope()
        {
            currentRecord = previous_;
        }

        void
        RecordPropertyFailure(std::string_view expression,
                              std::string_view file,
                              std::size_t line)
        {
            if (currentRecord == nullptr)
            {
                return;
            }
            // Only the first failure of a trial counts: it is the one the
            // body stopped at.
            if (!currentRecord->Failure.has_value())
            {
                currentRecord->Failure = "CATCH3_PROPERTY_ASSERT( "
                                         + std::string(expression) + " ) at "
                                         + std::string(file) + ":"
                                         + std::to_string(line);
            }
        }

        void
        RecordPropertyFailureMessage(std::string message)
        {
            if (currentRecord != nullptr && !currentRecord->Failure.has_value())
            {
                currentRecord->Failure = std::move(message);
            }
        }

        std::string
        DefaultPropertyName()
        {
            return defaultPropertyName;
        }

        PropertyNameScope::PropertyNameScope(std::string name)
            : previous_(std::exchange(defaultPropertyName, std::move(name)))
        {}

        PropertyNameScope::~PropertyNameScope()
        {
            defaultPropertyName = std::move(previous_);
        }

    } // namespace Detail

    void
    Note(std::string text)
    {
        if (currentRecord != nullptr)
        {
            currentRecord->Notes.push_back(std::move(text));
        }
    }

    void
    Classify(bool condition, std::string label)
    {
        if (currentRecord != nullptr && condition)
        {
            currentRecord->Labels.push_back(std::move(label));
        }
    }

    void
    Cover(double requiredPercentage, bool condition, std::string label)
    {
        if (currentRecord != nullptr)
        {
            currentRecord->Covers.push_back(
                { std::move(label), requiredPercentage, condition });
        }
    }

    void
    Target(double score, std::string label)
    {
        if (currentRecord != nullptr)
        {
            currentRecord->Targets.push_back({ std::move(label), score });
        }
    }

} // namespace Progmasoft::Catch3
