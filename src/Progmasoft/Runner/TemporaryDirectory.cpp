// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#include "TemporaryDirectory.hpp"

#include "Process.hpp"

#include <Progmasoft/Catch3/Runner.hpp>

#include <cstdlib>
#include <filesystem>
#include <mutex>
#include <system_error>

namespace Progmasoft::Catch3::Runner
{
    namespace
    {
        constexpr std::array<const char *, 3> kVariables = {
            "TMPDIR", "TMP", "TEMP"};

        std::mutex &
        CurrentMutex()
        {
            static auto *mutex = new std::mutex();
            return *mutex;
        }

        std::string &
        Current()
        {
            static auto *directory = new std::string();
            return *directory;
        }

        void
        SetVariable(const char *name, const std::optional<std::string> &value)
        {
#if defined(_WIN32)
            // An empty value removes the variable.
            _putenv_s(name, value ? value->c_str() : "");
#else
            if (value)
            {
                setenv(name, value->c_str(), 1);
            }
            else
            {
                unsetenv(name);
            }
#endif
        }
    } // namespace

    TemporaryDirectoryScope::TemporaryDirectoryScope(
        const std::string &directory)
    {
        if (directory.empty())
        {
            return;
        }
        std::error_code error;
        std::filesystem::create_directories(directory, error);
        if (error)
        {
            return;
        }
        active_ = true;
        for (std::size_t index = 0; index < kVariables.size(); ++index)
        {
            if (HasEnvironment(kVariables[index]))
            {
                previous_[index] = ReadEnvironment(kVariables[index]);
            }
            SetVariable(kVariables[index], directory);
        }
        const std::lock_guard lock{CurrentMutex()};
        Current() = directory;
    }

    TemporaryDirectoryScope::~TemporaryDirectoryScope()
    {
        if (!active_)
        {
            return;
        }
        for (std::size_t index = 0; index < kVariables.size(); ++index)
        {
            SetVariable(kVariables[index], previous_[index]);
        }
        const std::lock_guard lock{CurrentMutex()};
        Current().clear();
    }
} // namespace Progmasoft::Catch3::Runner

namespace Progmasoft::Catch3
{
    std::filesystem::path
    TestTemporaryDirectory()
    {
        {
            const std::lock_guard lock{Runner::CurrentMutex()};
            if (!Runner::Current().empty())
            {
                return std::filesystem::path{Runner::Current()};
            }
        }
        std::error_code error;
        auto directory = std::filesystem::temp_directory_path(error);
        return error ? std::filesystem::path{"."} : directory;
    }
} // namespace Progmasoft::Catch3
