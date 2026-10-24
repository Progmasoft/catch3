// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#pragma once

#include "Json.hpp"
#include "Process.hpp"

#include <mutex>
#include <optional>
#include <string>

namespace Progmasoft::Catch3::Runner
{
    /// The runner and its workers exchange one JSON object per line. Every
    /// object has a member "t" naming its kind.
    ///
    /// From the runner to a worker:
    ///   run    {index, attempt, temp}  run one test
    ///   quit   {}                      end the process
    ///
    /// From a worker to the runner:
    ///   hello     {pid, identity, tests, protocol}
    ///   start     {index, attempt}
    ///   section   {begin: true, name, file, line, depth, at}
    ///   section   {begin: false, depth, at, duration}
    ///   at        {location}           the last assertion started
    ///   assertion {an AssertionRecord}
    ///   benchmark {a BenchmarkRecord}
    ///   fatal     {message, location}  sent before the worker dies
    ///   end       {index, attempt, outcome, category, passed, failed,
    ///              duration, stdout, stderr}
    inline constexpr int kWorkerProtocolVersion = 1;

    /// Reads lines from a pipe.
    class LineReader final
    {
    public:
        explicit LineReader(PipeReader &pipe)
            : pipe_{pipe}
        {
        }

        /// The next line without its end, or nothing at the end of the
        /// stream.
        [[nodiscard]] std::optional<std::string>
        Next();

    private:
        PipeReader &pipe_;
        std::string buffer_;
        std::size_t scanned_ = 0;
        bool ended_ = false;
    };

    /// Writes messages to a pipe, one line each; safe from several
    /// threads, since a test may assert from threads of its own.
    class MessageWriter final
    {
    public:
        explicit MessageWriter(PipeWriter &pipe)
            : pipe_{pipe}
        {
        }

        Status
        Send(const JsonValue &message);

    private:
        PipeWriter &pipe_;
        std::mutex mutex_;
    };

    /// A message of kind `kind` with no other members yet.
    [[nodiscard]] JsonValue
    Message(std::string_view kind);
} // namespace Progmasoft::Catch3::Runner
