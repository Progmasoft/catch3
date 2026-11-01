// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0

#include <Progmasoft/Catch3/Interfaces/IResultCapture.hpp>
#include <Progmasoft/Catch3/Internal/AssertionHandler.hpp>
#include <Progmasoft/Catch3/Internal/CompilerCapabilities.hpp>
#include <Progmasoft/Catch3/Internal/Context.hpp>
#include <Progmasoft/Catch3/Mock.hpp>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <exception>

namespace Progmasoft::Catch3::Detail
{
    namespace
    {
        constexpr auto kMacro = "CATCH3_EXPECT_CALL";

        /// Whether a test case is running that a report can belong to.
        [[nodiscard]] bool
        InTestCase()
        {
            const auto *capture = Catch::getCurrentContext().getResultCapture();
            return capture != nullptr && !capture->getCurrentTestName().empty();
        }

        void
        Report(const Catch::SourceLineInfo &line,
               const std::string &mock,
               Catch::ResultWas::OfType result,
               std::string message,
               Catch::ResultDisposition::Flags disposition)
        {
            Catch::AssertionHandler handler(kMacro, line, mock, disposition);
            handler.handleMessage(result, std::move(message));
            handler.complete();
        }

        void
        WriteToStandardError(const Catch::SourceLineInfo &line,
                             const std::string &mock,
                             const std::string &message)
        {
            std::fprintf(stderr,
                         "%s:%zu: %s: %s\n",
                         line.file,
                         line.line,
                         mock.c_str(),
                         message.c_str());
        }
    } // namespace

    void
    MockSequence::Add(const std::shared_ptr<MockExpectationState> &step)
    {
        steps.push_back(step);
    }

    std::string
    MockSequence::Enter(const MockExpectationState &step)
    {
        const auto position
            = std::find_if(steps.begin() + static_cast<std::ptrdiff_t>(current),
                           steps.end(),
                           [&step](const auto &candidate) {
                               return candidate.lock().get() == &step;
                           });
        std::string undone;
        if (position == steps.end())
        {
            return undone;
        }
        const auto target = static_cast<std::size_t>(position - steps.begin());
        for (; current < target; ++current)
        {
            const auto earlier = steps[current].lock();
            if (earlier == nullptr)
            {
                continue;
            }
            if (!earlier->Satisfied())
            {
                undone += (undone.empty() ? "call out of order: " : "\n  and ")
                          + step.Description + " came before "
                          + earlier->Description;
            }
            earlier->Retired = true;
        }
        return undone;
    }

    std::unique_lock<std::recursive_mutex>
    LockMocks()
    {
        static std::recursive_mutex mutex;
        return std::unique_lock<std::recursive_mutex>{ mutex };
    }

    void
    ReportMockFailure(const Catch::SourceLineInfo &line,
                      const std::string &mock,
                      std::string message,
                      MockReport report)
    {
        if (!InTestCase())
        {
            WriteToStandardError(line, mock, message);
            return;
        }
        // An aborting run throws out of any failure, and a destructor must
        // not let that through.
        CATCH_TRY
        {
            Report(line,
                   mock,
                   Catch::ResultWas::ExplicitFailure,
                   std::move(message),
                   Catch::ResultDisposition::ContinueOnFailure);
        }
        CATCH_CATCH_ALL
        {
            if (report == MockReport::kContinue)
            {
                std::rethrow_exception(std::current_exception());
            }
        }
    }

    void
    AbortMockCall(const Catch::SourceLineInfo &line,
                  const std::string &mock,
                  std::string message)
    {
        if (InTestCase())
        {
            Report(line,
                   mock,
                   Catch::ResultWas::ExplicitFailure,
                   std::move(message),
                   Catch::ResultDisposition::Normal);
        }
        else
        {
            WriteToStandardError(line, mock, message);
        }
        std::abort();
    }

    void
    ReportMockSuccess(const Catch::SourceLineInfo &line,
                      const std::string &mock,
                      std::string message)
    {
        if (InTestCase())
        {
            Report(line,
                   mock,
                   Catch::ResultWas::Ok,
                   std::move(message),
                   Catch::ResultDisposition::ContinueOnFailure);
        }
    }

    std::string
    DescribeCalls(std::size_t minimum, std::size_t maximum)
    {
        const auto calls = [](std::size_t count) {
            return std::to_string(count) + (count == 1 ? " call" : " calls");
        };
        if (minimum == 0 && maximum == kUnlimitedCalls)
        {
            return "any number of calls";
        }
        if (minimum == maximum)
        {
            return maximum == 0 ? "no calls" : "exactly " + calls(minimum);
        }
        if (maximum == kUnlimitedCalls)
        {
            return "at least " + calls(minimum);
        }
        if (minimum == 0)
        {
            return "at most " + calls(maximum);
        }
        return std::to_string(minimum) + " to " + calls(maximum);
    }

    std::string
    DescribeCount(std::size_t calls)
    {
        switch (calls)
        {
            case 0:
                return "never called";
            case 1:
                return "called once";
            default:
                return "called " + std::to_string(calls) + " times";
        }
    }
} // namespace Progmasoft::Catch3::Detail
