// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#pragma once

#include <cassert>
#include <optional>
#include <string>
#include <utility>
#include <variant>

namespace Progmasoft::Catch3::Runner
{
    /// A failure with a message written for the person who has to act on it.
    class Error final
    {
    public:
        explicit Error(std::string message)
            : message_{std::move(message)}
        {
        }

        [[nodiscard]] const std::string &
        Message() const noexcept
        {
            return message_;
        }

        /// The same failure, with what was being done in front of it.
        [[nodiscard]] Error
        Context(const std::string &context) const
        {
            return Error{context + ": " + message_};
        }

    private:
        std::string message_;
    };

    /// A value, or the error that prevented it.
    template<typename Value>
    class [[nodiscard]] Expected final
    {
    public:
        Expected(Value value)
            : state_{std::in_place_index<0>, std::move(value)}
        {
        }

        Expected(Error error)
            : state_{std::in_place_index<1>, std::move(error)}
        {
        }

        [[nodiscard]] explicit
        operator bool() const noexcept
        {
            return state_.index() == 0;
        }

        [[nodiscard]] Value &
        operator*() &
        {
            assert(*this);
            return std::get<0>(state_);
        }

        [[nodiscard]] const Value &
        operator*() const &
        {
            assert(*this);
            return std::get<0>(state_);
        }

        [[nodiscard]] Value &&
        operator*() &&
        {
            assert(*this);
            return std::get<0>(std::move(state_));
        }

        [[nodiscard]] Value *
        operator->()
        {
            assert(*this);
            return &std::get<0>(state_);
        }

        [[nodiscard]] const Value *
        operator->() const
        {
            assert(*this);
            return &std::get<0>(state_);
        }

        [[nodiscard]] const Error &
        GetError() const
        {
            assert(!*this);
            return std::get<1>(state_);
        }

    private:
        std::variant<Value, Error> state_;
    };

    /// Success, or the error that happened instead.
    class [[nodiscard]] Status final
    {
    public:
        Status() = default;

        Status(Error error)
            : error_{std::move(error)}
        {
        }

        [[nodiscard]] static Status
        Ok()
        {
            return Status{};
        }

        [[nodiscard]] explicit
        operator bool() const noexcept
        {
            return !error_.has_value();
        }

        [[nodiscard]] const Error &
        GetError() const
        {
            assert(error_.has_value());
            return *error_;
        }

    private:
        std::optional<Error> error_;
    };
} // namespace Progmasoft::Catch3::Runner
