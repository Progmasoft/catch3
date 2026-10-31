// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0
#pragma once

#include "Expected.hpp"

#include <cstdint>
#include <initializer_list>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Progmasoft::Catch3::Runner
{
    class JsonValue;

    /// The members of an object, in the order they were added or read.
    using JsonMembers = std::vector<std::pair<std::string, JsonValue>>;

    /// One JSON value: the messages between the runner and its workers, the
    /// reports, the history and the coverage exports of LLVM are all JSON.
    ///
    /// Numbers are kept as `double`, which represents every integer the
    /// runner writes (counts, microseconds, 32-bit seeds) exactly.
    class JsonValue final
    {
    public:
        enum class Kind : std::uint8_t
        {
            kNull,
            kBoolean,
            kNumber,
            kString,
            kArray,
            kObject,
        };

        JsonValue() = default;
        JsonValue(std::nullptr_t) {}
        JsonValue(bool value);
        JsonValue(double value);
        JsonValue(int value);
        JsonValue(unsigned value);
        JsonValue(long value);
        JsonValue(unsigned long value);
        JsonValue(long long value);
        JsonValue(unsigned long long value);
        JsonValue(const char *value);
        JsonValue(std::string value);
        JsonValue(std::string_view value);

        JsonValue(const JsonValue &other);
        JsonValue(JsonValue &&other) noexcept;
        JsonValue &
        operator=(const JsonValue &other);
        JsonValue &
        operator=(JsonValue &&other) noexcept;
        ~JsonValue();

        [[nodiscard]] static JsonValue
        Array(std::vector<JsonValue> items = {});

        [[nodiscard]] static JsonValue
        Object(JsonMembers members = {});

        [[nodiscard]] Kind
        GetKind() const noexcept
        {
            return kind_;
        }

        [[nodiscard]] bool
        IsNull() const noexcept
        {
            return kind_ == Kind::kNull;
        }

        [[nodiscard]] bool
        IsObject() const noexcept
        {
            return kind_ == Kind::kObject;
        }

        [[nodiscard]] bool
        IsArray() const noexcept
        {
            return kind_ == Kind::kArray;
        }

        [[nodiscard]] bool
        IsString() const noexcept
        {
            return kind_ == Kind::kString;
        }

        [[nodiscard]] bool
        IsNumber() const noexcept
        {
            return kind_ == Kind::kNumber;
        }

        /// The value as a Boolean; anything else reads as `fallback`.
        [[nodiscard]] bool
        AsBoolean(bool fallback = false) const noexcept;

        [[nodiscard]] double
        AsNumber(double fallback = 0.0) const noexcept;

        [[nodiscard]] std::int64_t
        AsInteger(std::int64_t fallback = 0) const noexcept;

        [[nodiscard]] std::uint64_t
        AsUnsigned(std::uint64_t fallback = 0) const noexcept;

        /// The text of a string; anything else reads as the empty string.
        [[nodiscard]] const std::string &
        AsString() const noexcept;

        /// The items of an array; anything else has none.
        [[nodiscard]] const std::vector<JsonValue> &
        Items() const noexcept;

        [[nodiscard]] std::vector<JsonValue> &
        MutableItems();

        /// The members of an object; anything else has none.
        [[nodiscard]] const JsonMembers &
        Members() const noexcept;

        [[nodiscard]] JsonMembers &
        MutableMembers();

        /// The member named `key`, or null when there is none.
        [[nodiscard]] const JsonValue &
        operator[](std::string_view key) const noexcept;

        /// The member named `key` of an object, added when missing.
        JsonValue &
        Set(std::string_view key, JsonValue value);

        /// Appends to an array.
        void
        Append(JsonValue value);

        /// The compact JSON text of the value, on one line.
        [[nodiscard]] std::string
        ToString() const;

        /// The JSON text with two spaces of indentation per level.
        [[nodiscard]] std::string
        ToPrettyString() const;

    private:
        void
        Destroy() noexcept;

        void
        CopyFrom(const JsonValue &other);

        void
        MoveFrom(JsonValue &&other) noexcept;

        Kind kind_ = Kind::kNull;
        bool boolean_ = false;
        double number_ = 0.0;
        std::string string_;
        std::unique_ptr<std::vector<JsonValue>> items_;
        std::unique_ptr<JsonMembers> members_;
    };

    /// Reads one JSON document; whitespace may surround it.
    [[nodiscard]] Expected<JsonValue>
    ParseJson(std::string_view text);

    /// Appends `text` to `output` as a JSON string literal.
    void
    AppendJsonString(std::string &output, std::string_view text);
} // namespace Progmasoft::Catch3::Runner
