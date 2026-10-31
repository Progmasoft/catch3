// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0

// The runner reads JSON from its workers, from history files and from
// reports; none of them may crash it. A document that parses must also
// print and parse back to the same text.

#include <Progmasoft/Runner/Json.hpp>

#include <cstdint>
#include <cstdlib>
#include <string_view>

extern "C" int
LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size)
{
    const std::string_view text{reinterpret_cast<const char *>(data), size};
    auto parsed = Progmasoft::Catch3::Runner::ParseJson(text);
    if (parsed)
    {
        const auto printed = parsed->ToString();
        auto again = Progmasoft::Catch3::Runner::ParseJson(printed);
        if (!again || again->ToString() != printed)
        {
            std::abort();
        }
    }
    return 0;
}
