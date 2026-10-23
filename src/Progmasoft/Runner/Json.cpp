// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#include "Json.hpp"

#include <clocale>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <optional>

namespace Progmasoft::Catch3::Runner
{
    namespace
    {
        // The empty values that accessors return for a value of another
        // kind. They are never destroyed, so that no destructor runs at exit
        // while a worker may still be reading a message.
        const std::string &
        EmptyString()
        {
            static const auto *value = new std::string();
            return *value;
        }

        const std::vector<JsonValue> &
        EmptyItems()
        {
            static const auto *value = new std::vector<JsonValue>();
            return *value;
        }

        const JsonMembers &
        EmptyMembers()
        {
            static const auto *value = new JsonMembers();
            return *value;
        }

        const JsonValue &
        NullValue()
        {
            static const auto *value = new JsonValue();
            return *value;
        }

        /// Documents nested deeper than this are refused, so a hostile or
        /// broken file cannot exhaust the stack of the parser.
        constexpr int kMaximumDepth = 256;

        void
        AppendUtf8(std::string &output, std::uint32_t codePoint)
        {
            if (codePoint < 0x80)
            {
                output.push_back(static_cast<char>(codePoint));
            }
            else if (codePoint < 0x800)
            {
                output.push_back(static_cast<char>(0xC0 | (codePoint >> 6)));
                output.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
            }
            else if (codePoint < 0x10000)
            {
                output.push_back(static_cast<char>(0xE0 | (codePoint >> 12)));
                output.push_back(
                    static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
                output.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
            }
            else
            {
                output.push_back(static_cast<char>(0xF0 | (codePoint >> 18)));
                output.push_back(
                    static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F)));
                output.push_back(
                    static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
                output.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
            }
        }

        /// Reads a JSON number exactly. The grammar is checked here; strtod
        /// does the rounding, with the point replaced by the decimal
        /// separator of the current locale, which a test may have changed.
        std::optional<double>
        ToDouble(std::string_view digits)
        {
            std::size_t index = 0;
            const auto skipDigits = [&] {
                const std::size_t start = index;
                while (index < digits.size() && digits[index] >= '0' &&
                       digits[index] <= '9')
                {
                    ++index;
                }
                return index > start;
            };
            if (index < digits.size() && digits[index] == '-')
            {
                ++index;
            }
            if (!skipDigits())
            {
                return std::nullopt;
            }
            std::size_t point = std::string_view::npos;
            if (index < digits.size() && digits[index] == '.')
            {
                point = index++;
                if (!skipDigits())
                {
                    return std::nullopt;
                }
            }
            if (index < digits.size() &&
                (digits[index] == 'e' || digits[index] == 'E'))
            {
                ++index;
                if (index < digits.size() &&
                    (digits[index] == '+' || digits[index] == '-'))
                {
                    ++index;
                }
                if (!skipDigits())
                {
                    return std::nullopt;
                }
            }
            if (index != digits.size())
            {
                return std::nullopt;
            }
            std::string copy{digits};
            if (point != std::string_view::npos)
            {
                const char *separator = std::localeconv()->decimal_point;
                copy.replace(point,
                             1,
                             separator == nullptr || *separator == '\0'
                                 ? "."
                                 : separator);
            }
            return std::strtod(copy.c_str(), nullptr);
        }

        class Parser final
        {
        public:
            explicit Parser(std::string_view text)
                : text_{text}
            {
            }

            Expected<JsonValue>
            ParseDocument()
            {
                SkipWhitespace();
                auto value = ParseValue(0);
                if (!value)
                {
                    return value;
                }
                SkipWhitespace();
                if (position_ != text_.size())
                {
                    return Fail("unexpected text after the value");
                }
                return value;
            }

        private:
            Error
            Fail(const std::string &message) const
            {
                return Error{"invalid JSON at offset " +
                             std::to_string(position_) + ": " + message};
            }

            void
            SkipWhitespace()
            {
                while (position_ < text_.size())
                {
                    const char character = text_[position_];
                    if (character != ' ' && character != '\t' &&
                        character != '\n' && character != '\r')
                    {
                        return;
                    }
                    ++position_;
                }
            }

            bool
            Consume(std::string_view word)
            {
                if (text_.substr(position_, word.size()) == word)
                {
                    position_ += word.size();
                    return true;
                }
                return false;
            }

            Expected<JsonValue>
            ParseValue(int depth)
            {
                if (depth > kMaximumDepth)
                {
                    return Fail("nested too deeply");
                }
                if (position_ >= text_.size())
                {
                    return Fail("a value was expected");
                }
                const char character = text_[position_];
                if (character == '{')
                {
                    return ParseObject(depth);
                }
                if (character == '[')
                {
                    return ParseArray(depth);
                }
                if (character == '"')
                {
                    auto text = ParseString();
                    if (!text)
                    {
                        return text.GetError();
                    }
                    return JsonValue{std::move(*text)};
                }
                if (Consume("true"))
                {
                    return JsonValue{true};
                }
                if (Consume("false"))
                {
                    return JsonValue{false};
                }
                if (Consume("null"))
                {
                    return JsonValue{};
                }
                return ParseNumber();
            }

            Expected<JsonValue>
            ParseNumber()
            {
                const std::size_t start = position_;
                if (position_ < text_.size() && text_[position_] == '-')
                {
                    ++position_;
                }
                while (position_ < text_.size())
                {
                    const char character = text_[position_];
                    const bool numeric =
                        (character >= '0' && character <= '9') ||
                        character == '.' || character == 'e' ||
                        character == 'E' || character == '+' ||
                        character == '-';
                    if (!numeric)
                    {
                        break;
                    }
                    ++position_;
                }
                if (position_ == start)
                {
                    return Fail("a value was expected");
                }
                const std::string_view digits =
                    text_.substr(start, position_ - start);
                const auto value = ToDouble(digits);
                if (!value)
                {
                    position_ = start;
                    return Fail("malformed number");
                }
                return JsonValue{*value};
            }

            Expected<std::uint32_t>
            ParseHexQuad()
            {
                if (position_ + 4 > text_.size())
                {
                    return Fail("truncated \\u escape");
                }
                std::uint32_t value = 0;
                for (int index = 0; index < 4; ++index)
                {
                    const char character = text_[position_++];
                    value <<= 4;
                    if (character >= '0' && character <= '9')
                    {
                        value |= static_cast<std::uint32_t>(character - '0');
                    }
                    else if (character >= 'a' && character <= 'f')
                    {
                        value |=
                            static_cast<std::uint32_t>(character - 'a' + 10);
                    }
                    else if (character >= 'A' && character <= 'F')
                    {
                        value |=
                            static_cast<std::uint32_t>(character - 'A' + 10);
                    }
                    else
                    {
                        return Fail("malformed \\u escape");
                    }
                }
                return value;
            }

            Expected<std::string>
            ParseString()
            {
                ++position_; // The opening quote.
                std::string result;
                while (position_ < text_.size())
                {
                    const char character = text_[position_++];
                    if (character == '"')
                    {
                        return result;
                    }
                    if (static_cast<unsigned char>(character) < 0x20)
                    {
                        return Fail("control character in a string");
                    }
                    if (character != '\\')
                    {
                        result.push_back(character);
                        continue;
                    }
                    if (position_ >= text_.size())
                    {
                        break;
                    }
                    const char escape = text_[position_++];
                    switch (escape)
                    {
                        case '"':
                        case '\\':
                        case '/':
                            result.push_back(escape);
                            break;
                        case 'b':
                            result.push_back('\b');
                            break;
                        case 'f':
                            result.push_back('\f');
                            break;
                        case 'n':
                            result.push_back('\n');
                            break;
                        case 'r':
                            result.push_back('\r');
                            break;
                        case 't':
                            result.push_back('\t');
                            break;
                        case 'u':
                        {
                            auto high = ParseHexQuad();
                            if (!high)
                            {
                                return high.GetError();
                            }
                            std::uint32_t codePoint = *high;
                            if (codePoint >= 0xD800 && codePoint <= 0xDBFF &&
                                Consume("\\u"))
                            {
                                auto low = ParseHexQuad();
                                if (!low)
                                {
                                    return low.GetError();
                                }
                                if (*low >= 0xDC00 && *low <= 0xDFFF)
                                {
                                    codePoint = 0x10000 +
                                                ((codePoint - 0xD800) << 10) +
                                                (*low - 0xDC00);
                                }
                            }
                            AppendUtf8(result, codePoint);
                            break;
                        }
                        default:
                            return Fail("unknown escape");
                    }
                }
                return Fail("unterminated string");
            }

            Expected<JsonValue>
            ParseArray(int depth)
            {
                ++position_;
                JsonValue array = JsonValue::Array();
                SkipWhitespace();
                if (Consume("]"))
                {
                    return array;
                }
                while (true)
                {
                    SkipWhitespace();
                    auto item = ParseValue(depth + 1);
                    if (!item)
                    {
                        return item;
                    }
                    array.Append(std::move(*item));
                    SkipWhitespace();
                    if (Consume("]"))
                    {
                        return array;
                    }
                    if (!Consume(","))
                    {
                        return Fail("',' or ']' was expected");
                    }
                }
            }

            Expected<JsonValue>
            ParseObject(int depth)
            {
                ++position_;
                JsonValue object = JsonValue::Object();
                SkipWhitespace();
                if (Consume("}"))
                {
                    return object;
                }
                while (true)
                {
                    SkipWhitespace();
                    if (position_ >= text_.size() || text_[position_] != '"')
                    {
                        return Fail("a member name was expected");
                    }
                    auto key = ParseString();
                    if (!key)
                    {
                        return key.GetError();
                    }
                    SkipWhitespace();
                    if (!Consume(":"))
                    {
                        return Fail("':' was expected");
                    }
                    SkipWhitespace();
                    auto value = ParseValue(depth + 1);
                    if (!value)
                    {
                        return value;
                    }
                    object.MutableMembers().emplace_back(std::move(*key),
                                                         std::move(*value));
                    SkipWhitespace();
                    if (Consume("}"))
                    {
                        return object;
                    }
                    if (!Consume(","))
                    {
                        return Fail("',' or '}' was expected");
                    }
                }
            }

            std::string_view text_;
            std::size_t position_ = 0;
        };

        void
        AppendNumber(std::string &output, double value)
        {
            if (!std::isfinite(value))
            {
                // JSON has no infinities; null is the conventional stand-in.
                output += "null";
                return;
            }
            if (std::fabs(value) < 9.007199254740992e15)
            {
                const auto integral = static_cast<long long>(value);
                if (std::fabs(value - static_cast<double>(integral)) < 1e-9)
                {
                    output += std::to_string(integral);
                    return;
                }
            }
            char buffer[64];
            std::snprintf(buffer, sizeof(buffer), "%.17g", value);
            // The C library writes the decimal separator of the current
            // locale, which a test may have changed.
            for (char &character : buffer)
            {
                if (character == ',')
                {
                    character = '.';
                }
            }
            output += buffer;
        }

        void
        AppendIndent(std::string &output, int indent, int depth)
        {
            if (indent <= 0)
            {
                return;
            }
            output.push_back('\n');
            output.append(static_cast<std::size_t>(indent * depth), ' ');
        }
    } // namespace

    JsonValue::JsonValue(bool value)
        : kind_{Kind::kBoolean}
        , boolean_{value}
    {
    }

    JsonValue::JsonValue(double value)
        : kind_{Kind::kNumber}
        , number_{value}
    {
    }

    JsonValue::JsonValue(int value)
        : JsonValue{static_cast<double>(value)}
    {
    }

    JsonValue::JsonValue(unsigned value)
        : JsonValue{static_cast<double>(value)}
    {
    }

    JsonValue::JsonValue(long value)
        : JsonValue{static_cast<double>(value)}
    {
    }

    JsonValue::JsonValue(unsigned long value)
        : JsonValue{static_cast<double>(value)}
    {
    }

    JsonValue::JsonValue(long long value)
        : JsonValue{static_cast<double>(value)}
    {
    }

    JsonValue::JsonValue(unsigned long long value)
        : JsonValue{static_cast<double>(value)}
    {
    }

    JsonValue::JsonValue(const char *value)
        : kind_{Kind::kString}
        , string_{value == nullptr ? "" : value}
    {
    }

    JsonValue::JsonValue(std::string value)
        : kind_{Kind::kString}
        , string_{std::move(value)}
    {
    }

    JsonValue::JsonValue(std::string_view value)
        : kind_{Kind::kString}
        , string_{value}
    {
    }

    JsonValue::JsonValue(const JsonValue &other)
    {
        CopyFrom(other);
    }

    JsonValue::JsonValue(JsonValue &&other) noexcept
    {
        MoveFrom(std::move(other));
    }

    JsonValue &
    JsonValue::operator=(const JsonValue &other)
    {
        if (this != &other)
        {
            JsonValue copy{other};
            Destroy();
            MoveFrom(std::move(copy));
        }
        return *this;
    }

    JsonValue &
    JsonValue::operator=(JsonValue &&other) noexcept
    {
        if (this != &other)
        {
            Destroy();
            MoveFrom(std::move(other));
        }
        return *this;
    }

    JsonValue::~JsonValue() = default;

    void
    JsonValue::Destroy() noexcept
    {
        kind_ = Kind::kNull;
        boolean_ = false;
        number_ = 0.0;
        string_.clear();
        items_.reset();
        members_.reset();
    }

    void
    JsonValue::CopyFrom(const JsonValue &other)
    {
        kind_ = other.kind_;
        boolean_ = other.boolean_;
        number_ = other.number_;
        string_ = other.string_;
        if (other.items_)
        {
            items_ = std::make_unique<std::vector<JsonValue>>(*other.items_);
        }
        if (other.members_)
        {
            members_ = std::make_unique<JsonMembers>(*other.members_);
        }
    }

    void
    JsonValue::MoveFrom(JsonValue &&other) noexcept
    {
        kind_ = other.kind_;
        boolean_ = other.boolean_;
        number_ = other.number_;
        string_ = std::move(other.string_);
        items_ = std::move(other.items_);
        members_ = std::move(other.members_);
        other.kind_ = Kind::kNull;
    }

    JsonValue
    JsonValue::Array(std::vector<JsonValue> items)
    {
        JsonValue value;
        value.kind_ = Kind::kArray;
        value.items_ = std::make_unique<std::vector<JsonValue>>(std::move(items));
        return value;
    }

    JsonValue
    JsonValue::Object(JsonMembers members)
    {
        JsonValue value;
        value.kind_ = Kind::kObject;
        value.members_ = std::make_unique<JsonMembers>(std::move(members));
        return value;
    }

    bool
    JsonValue::AsBoolean(bool fallback) const noexcept
    {
        return kind_ == Kind::kBoolean ? boolean_ : fallback;
    }

    double
    JsonValue::AsNumber(double fallback) const noexcept
    {
        return kind_ == Kind::kNumber ? number_ : fallback;
    }

    std::int64_t
    JsonValue::AsInteger(std::int64_t fallback) const noexcept
    {
        if (kind_ != Kind::kNumber || !std::isfinite(number_) ||
            number_ < -9.2e18 || number_ > 9.2e18)
        {
            return fallback;
        }
        return static_cast<std::int64_t>(number_);
    }

    std::uint64_t
    JsonValue::AsUnsigned(std::uint64_t fallback) const noexcept
    {
        if (kind_ != Kind::kNumber || !std::isfinite(number_) ||
            number_ < 0.0 || number_ > 1.8e19)
        {
            return fallback;
        }
        return static_cast<std::uint64_t>(number_);
    }

    const std::string &
    JsonValue::AsString() const noexcept
    {
        return kind_ == Kind::kString ? string_ : EmptyString();
    }

    const std::vector<JsonValue> &
    JsonValue::Items() const noexcept
    {
        return kind_ == Kind::kArray && items_ ? *items_ : EmptyItems();
    }

    std::vector<JsonValue> &
    JsonValue::MutableItems()
    {
        if (kind_ != Kind::kArray || !items_)
        {
            *this = Array();
        }
        return *items_;
    }

    const JsonMembers &
    JsonValue::Members() const noexcept
    {
        return kind_ == Kind::kObject && members_ ? *members_ : EmptyMembers();
    }

    JsonMembers &
    JsonValue::MutableMembers()
    {
        if (kind_ != Kind::kObject || !members_)
        {
            *this = Object();
        }
        return *members_;
    }

    const JsonValue &
    JsonValue::operator[](std::string_view key) const noexcept
    {
        for (const auto &[name, value] : Members())
        {
            if (name == key)
            {
                return value;
            }
        }
        return NullValue();
    }

    JsonValue &
    JsonValue::Set(std::string_view key, JsonValue value)
    {
        auto &members = MutableMembers();
        for (auto &[name, existing] : members)
        {
            if (name == key)
            {
                existing = std::move(value);
                return existing;
            }
        }
        members.emplace_back(std::string{key}, std::move(value));
        return members.back().second;
    }

    void
    JsonValue::Append(JsonValue value)
    {
        MutableItems().push_back(std::move(value));
    }

    std::string
    JsonValue::ToString() const
    {
        std::string output;
        Write(output, 0, 0);
        return output;
    }

    std::string
    JsonValue::ToPrettyString() const
    {
        std::string output;
        Write(output, 2, 0);
        output.push_back('\n');
        return output;
    }

    void
    JsonValue::Write(std::string &output, int indent, int depth) const
    {
        switch (kind_)
        {
            case Kind::kNull:
                output += "null";
                return;
            case Kind::kBoolean:
                output += boolean_ ? "true" : "false";
                return;
            case Kind::kNumber:
                AppendNumber(output, number_);
                return;
            case Kind::kString:
                AppendJsonString(output, string_);
                return;
            case Kind::kArray:
            {
                const auto &items = Items();
                output.push_back('[');
                for (std::size_t index = 0; index < items.size(); ++index)
                {
                    if (index > 0)
                    {
                        output.push_back(',');
                    }
                    AppendIndent(output, indent, depth + 1);
                    items[index].Write(output, indent, depth + 1);
                }
                if (!items.empty())
                {
                    AppendIndent(output, indent, depth);
                }
                output.push_back(']');
                return;
            }
            case Kind::kObject:
            {
                const auto &members = Members();
                output.push_back('{');
                for (std::size_t index = 0; index < members.size(); ++index)
                {
                    if (index > 0)
                    {
                        output.push_back(',');
                    }
                    AppendIndent(output, indent, depth + 1);
                    AppendJsonString(output, members[index].first);
                    output += indent > 0 ? ": " : ":";
                    members[index].second.Write(output, indent, depth + 1);
                }
                if (!members.empty())
                {
                    AppendIndent(output, indent, depth);
                }
                output.push_back('}');
                return;
            }
        }
    }

    Expected<JsonValue>
    ParseJson(std::string_view text)
    {
        return Parser{text}.ParseDocument();
    }

    void
    AppendJsonString(std::string &output, std::string_view text)
    {
        output.push_back('"');
        for (const char character : text)
        {
            switch (character)
            {
                case '"':
                    output += "\\\"";
                    break;
                case '\\':
                    output += "\\\\";
                    break;
                case '\n':
                    output += "\\n";
                    break;
                case '\r':
                    output += "\\r";
                    break;
                case '\t':
                    output += "\\t";
                    break;
                case '\b':
                    output += "\\b";
                    break;
                case '\f':
                    output += "\\f";
                    break;
                default:
                    if (static_cast<unsigned char>(character) < 0x20)
                    {
                        char buffer[8];
                        std::snprintf(buffer,
                                      sizeof(buffer),
                                      "\\u%04x",
                                      static_cast<unsigned>(character));
                        output += buffer;
                    }
                    else
                    {
                        output.push_back(character);
                    }
            }
        }
        output.push_back('"');
    }
} // namespace Progmasoft::Catch3::Runner
