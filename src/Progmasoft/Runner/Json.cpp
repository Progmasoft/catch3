// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0
#include "Json.hpp"

#include "Yyjson.inl"

#include <cmath>
#include <cstdlib>

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
        /// broken file cannot exhaust the stack while it is converted.
        constexpr int kMaximumDepth = 256;

        /// Strings are bytes: test names and output need not be UTF-8, and
        /// they come back as they were written.
        constexpr yyjson_read_flag kReadFlags =
            YYJSON_READ_ALLOW_INVALID_UNICODE;

        /// JSON has no infinities; null is the conventional stand-in.
        constexpr yyjson_write_flag kWriteFlags =
            YYJSON_WRITE_ALLOW_INVALID_UNICODE |
            YYJSON_WRITE_INF_AND_NAN_AS_NULL;

        /// Converts a value of yyjson's tree; false when it is nested too
        /// deeply.
        bool
        FromYyjson(yyjson_val *value, int depth, JsonValue &result)
        {
            if (depth > kMaximumDepth)
            {
                return false;
            }
            switch (yyjson_get_type(value))
            {
                case YYJSON_TYPE_BOOL:
                    result = JsonValue{yyjson_get_bool(value)};
                    return true;
                case YYJSON_TYPE_NUM:
                    result = JsonValue{yyjson_get_num(value)};
                    return true;
                case YYJSON_TYPE_STR:
                    result = JsonValue{std::string_view{
                        yyjson_get_str(value), yyjson_get_len(value)}};
                    return true;
                case YYJSON_TYPE_ARR:
                {
                    JsonValue array = JsonValue::Array();
                    auto &items = array.MutableItems();
                    items.reserve(yyjson_arr_size(value));
                    yyjson_arr_iter iterator = yyjson_arr_iter_with(value);
                    while (yyjson_val *item = yyjson_arr_iter_next(&iterator))
                    {
                        items.emplace_back();
                        if (!FromYyjson(item, depth + 1, items.back()))
                        {
                            return false;
                        }
                    }
                    result = std::move(array);
                    return true;
                }
                case YYJSON_TYPE_OBJ:
                {
                    JsonValue object = JsonValue::Object();
                    auto &members = object.MutableMembers();
                    members.reserve(yyjson_obj_size(value));
                    yyjson_obj_iter iterator = yyjson_obj_iter_with(value);
                    while (yyjson_val *key = yyjson_obj_iter_next(&iterator))
                    {
                        members.emplace_back(
                            std::string{yyjson_get_str(key),
                                        yyjson_get_len(key)},
                            JsonValue{});
                        if (!FromYyjson(yyjson_obj_iter_get_val(key),
                                        depth + 1,
                                        members.back().second))
                        {
                            return false;
                        }
                    }
                    result = std::move(object);
                    return true;
                }
                default:
                    result = JsonValue{};
                    return true;
            }
        }

        /// A whole number within the exact range of a double is written as
        /// an integer, anything else as the shortest text that reads back as
        /// the same double.
        yyjson_mut_val *
        ToYyjsonNumber(yyjson_mut_doc *document, double value)
        {
            if (std::isfinite(value) &&
                std::fabs(value) < 9.007199254740992e15 &&
                value == std::trunc(value))
            {
                return yyjson_mut_sint(document,
                                       static_cast<std::int64_t>(value));
            }
            return yyjson_mut_real(document, value);
        }

        /// Builds yyjson's tree of a value. Strings are referenced, not
        /// copied: the tree is written before `value` can change.
        yyjson_mut_val *
        ToYyjson(yyjson_mut_doc *document, const JsonValue &value)
        {
            switch (value.GetKind())
            {
                case JsonValue::Kind::kNull:
                    return yyjson_mut_null(document);
                case JsonValue::Kind::kBoolean:
                    return yyjson_mut_bool(document, value.AsBoolean());
                case JsonValue::Kind::kNumber:
                    return ToYyjsonNumber(document, value.AsNumber());
                case JsonValue::Kind::kString:
                {
                    const std::string &text = value.AsString();
                    return yyjson_mut_strn(document, text.data(), text.size());
                }
                case JsonValue::Kind::kArray:
                {
                    yyjson_mut_val *array = yyjson_mut_arr(document);
                    for (const auto &item : value.Items())
                    {
                        yyjson_mut_arr_append(array, ToYyjson(document, item));
                    }
                    return array;
                }
                case JsonValue::Kind::kObject:
                {
                    yyjson_mut_val *object = yyjson_mut_obj(document);
                    for (const auto &[name, member] : value.Members())
                    {
                        yyjson_mut_obj_add(
                            object,
                            yyjson_mut_strn(document, name.data(), name.size()),
                            ToYyjson(document, member));
                    }
                    return object;
                }
            }
            return yyjson_mut_null(document);
        }

        std::string
        WriteJson(const JsonValue &value, yyjson_write_flag flags)
        {
            yyjson_mut_doc *document = yyjson_mut_doc_new(nullptr);
            if (document == nullptr)
            {
                return {};
            }
            yyjson_mut_doc_set_root(document, ToYyjson(document, value));
            std::size_t length = 0;
            char *text = yyjson_mut_write(document, flags, &length);
            std::string result =
                text == nullptr ? std::string{} : std::string{text, length};
            std::free(text);
            yyjson_mut_doc_free(document);
            return result;
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
        return WriteJson(*this, kWriteFlags);
    }

    std::string
    JsonValue::ToPrettyString() const
    {
        std::string output =
            WriteJson(*this, kWriteFlags | YYJSON_WRITE_PRETTY_TWO_SPACES);
        output.push_back('\n');
        return output;
    }

    Expected<JsonValue>
    ParseJson(std::string_view text)
    {
        yyjson_read_err error{};
        // Without YYJSON_READ_INSITU the text is only read.
        yyjson_doc *document = yyjson_read_opts(const_cast<char *>(text.data()),
                                                text.size(),
                                                kReadFlags,
                                                nullptr,
                                                &error);
        if (document == nullptr)
        {
            return Error{"invalid JSON at offset " + std::to_string(error.pos) +
                         ": " + (error.msg == nullptr ? "" : error.msg)};
        }
        JsonValue result;
        const bool converted =
            FromYyjson(yyjson_doc_get_root(document), 0, result);
        yyjson_doc_free(document);
        if (!converted)
        {
            return Error{"invalid JSON: nested deeper than " +
                         std::to_string(kMaximumDepth) + " levels"};
        }
        return result;
    }

    void
    AppendJsonString(std::string &output, std::string_view text)
    {
        output += WriteJson(JsonValue{text}, kWriteFlags);
    }
} // namespace Progmasoft::Catch3::Runner
