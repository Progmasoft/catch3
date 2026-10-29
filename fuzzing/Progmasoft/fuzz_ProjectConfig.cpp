// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1

// The text of a catch3.json. Reading it must give options or an error, never
// crash, and the worker arguments derived from the options must parse again.

#include <Progmasoft/Runner/Options.hpp>
#include <Progmasoft/Runner/ProjectConfig.hpp>

#include <cstdint>
#include <cstdlib>
#include <string_view>

extern "C" int
LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size)
{
    namespace Runner = Progmasoft::Catch3::Runner;
    const std::string_view text{reinterpret_cast<const char *>(data), size};
    auto options = Runner::ApplyProjectConfig(text, "project", "catch3.json");
    if (options)
    {
        const auto worker =
            Runner::ParseOptions(Runner::WorkerArguments(*options));
        if (!worker)
        {
            std::abort();
        }
    }
    return 0;
}
