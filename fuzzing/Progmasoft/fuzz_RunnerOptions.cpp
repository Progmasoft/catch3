// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0

// The command line of a test binary: arguments separated by NUL bytes. The
// parser must answer with options or an error, never crash, and the worker
// arguments it derives must parse again.

#include <Progmasoft/Runner/Options.hpp>

#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

extern "C" int
LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size)
{
    std::vector<std::string> arguments(1);
    for (std::size_t index = 0; index < size; ++index)
    {
        if (data[index] == 0)
        {
            arguments.emplace_back();
        }
        else
        {
            arguments.back().push_back(static_cast<char>(data[index]));
        }
    }
    // Reading a file named by --input-file is not what is fuzzed here.
    for (const auto &argument : arguments)
    {
        if (argument.find("input-file") != std::string::npos ||
            (argument.size() >= 2 && argument[0] == '-' && argument[1] == 'f'))
        {
            return 0;
        }
    }
    auto options = Progmasoft::Catch3::Runner::ParseOptions(arguments);
    if (options)
    {
        const auto worker = Progmasoft::Catch3::Runner::ParseOptions(
            Progmasoft::Catch3::Runner::WorkerArguments(*options));
        if (!worker)
        {
            std::abort();
        }
    }
    return 0;
}
