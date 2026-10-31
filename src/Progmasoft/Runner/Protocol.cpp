// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0
#include "Protocol.hpp"

namespace Progmasoft::Catch3::Runner
{
    std::optional<std::string>
    LineReader::Next()
    {
        while (true)
        {
            const auto end = buffer_.find('\n', scanned_);
            if (end != std::string::npos)
            {
                std::string line = buffer_.substr(0, end);
                buffer_.erase(0, end + 1);
                scanned_ = 0;
                if (!line.empty() && line.back() == '\r')
                {
                    line.pop_back();
                }
                return line;
            }
            scanned_ = buffer_.size();
            if (ended_)
            {
                if (buffer_.empty())
                {
                    return std::nullopt;
                }
                // A last line without its end still counts.
                std::string line = std::move(buffer_);
                buffer_.clear();
                scanned_ = 0;
                return line;
            }
            char chunk[8192];
            const auto count = pipe_.Read(chunk, sizeof(chunk));
            if (count <= 0)
            {
                ended_ = true;
                continue;
            }
            buffer_.append(chunk, static_cast<std::size_t>(count));
        }
    }

    Status
    MessageWriter::Send(const JsonValue &message)
    {
        std::string line = message.ToString();
        line.push_back('\n');
        const std::lock_guard lock{mutex_};
        return pipe_.Write(line);
    }

    JsonValue
    Message(std::string_view kind)
    {
        JsonValue message = JsonValue::Object();
        message.Set("t", kind);
        return message;
    }
} // namespace Progmasoft::Catch3::Runner
