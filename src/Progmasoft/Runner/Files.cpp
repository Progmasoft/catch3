// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0
#include "Files.hpp"

#include "Hash.hpp"
#include "Process.hpp"

#if defined(_WIN32)
#    include <fcntl.h>
#    include <io.h>
#    include <share.h>
#    include <sys/stat.h>
#else
#    include <fcntl.h>
#    include <sys/stat.h>
#    include <unistd.h>
#endif

#include <atomic>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <system_error>
#include <thread>

namespace Progmasoft::Catch3::Runner
{
    namespace
    {
        /// Creates `path` only if it does not exist, in one atomic step.
        bool
        CreateExclusively(const std::filesystem::path &path)
        {
#if defined(_WIN32)
            int descriptor = -1;
            if (_wsopen_s(&descriptor,
                          path.wstring().c_str(),
                          _O_CREAT | _O_EXCL | _O_WRONLY,
                          _SH_DENYNO,
                          _S_IREAD | _S_IWRITE) != 0)
            {
                return false;
            }
            _close(descriptor);
            return true;
#else
            const int descriptor = open(path.c_str(),
                                        O_CREAT | O_EXCL | O_WRONLY | O_CLOEXEC,
                                        S_IRUSR | S_IWUSR);
            if (descriptor < 0)
            {
                return false;
            }
            close(descriptor);
            return true;
#endif
        }
    } // namespace

    FileLock::FileLock(const std::filesystem::path &file,
                       std::chrono::milliseconds wait,
                       std::chrono::milliseconds stale)
        : path_{file.string() + ".lock"}
    {
        std::error_code error;
        std::filesystem::create_directories(path_.parent_path(), error);
        const auto deadline = std::chrono::steady_clock::now() + wait;
        while (true)
        {
            if (CreateExclusively(path_))
            {
                held_ = true;
                return;
            }
            const auto written = std::filesystem::last_write_time(path_, error);
            if (!error &&
                std::filesystem::file_time_type::clock::now() - written > stale)
            {
                std::filesystem::remove(path_, error);
                continue;
            }
            if (std::chrono::steady_clock::now() >= deadline)
            {
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    FileLock::~FileLock()
    {
        if (held_)
        {
            std::error_code error;
            std::filesystem::remove(path_, error);
        }
    }

    Expected<std::string>
    ReadFile(const std::filesystem::path &file)
    {
        std::ifstream input(file, std::ios::binary);
        if (!input)
        {
            return Error{"cannot read '" + file.string() + "'"};
        }
        std::ostringstream contents;
        contents << input.rdbuf();
        if (input.bad())
        {
            return Error{"cannot read '" + file.string() + "'"};
        }
        return contents.str();
    }

    Status
    WriteFileAtomically(const std::filesystem::path &file,
                        std::string_view contents)
    {
        std::error_code error;
        if (file.has_parent_path())
        {
            std::filesystem::create_directories(file.parent_path(), error);
        }
        const auto temporary =
            std::filesystem::path{file.string() + "." + UniqueName() + ".tmp"};
        {
            std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
            output.write(contents.data(),
                         static_cast<std::streamsize>(contents.size()));
            if (!output)
            {
                output.close();
                std::filesystem::remove(temporary, error);
                return Error{"cannot write '" + file.string() + "'"};
            }
        }
        std::filesystem::rename(temporary, file, error);
        if (error)
        {
            std::error_code ignored;
            std::filesystem::remove(temporary, ignored);
            return Error{"cannot replace '" + file.string() +
                         "': " + error.message()};
        }
        return Status::Ok();
    }

    Status
    CreatePrivateFile(const std::filesystem::path &file)
    {
#if defined(_WIN32)
        std::error_code error;
        const auto status = std::filesystem::symlink_status(file, error);
        if (!error && std::filesystem::exists(status) &&
            !std::filesystem::is_regular_file(status))
        {
            return Error{"'" + file.string() + "' is not a regular file"};
        }
        int descriptor = -1;
        if (_wsopen_s(&descriptor,
                      file.wstring().c_str(),
                      _O_CREAT | _O_TRUNC | _O_WRONLY | _O_NOINHERIT,
                      _SH_DENYNO,
                      _S_IREAD | _S_IWRITE) != 0)
        {
            return Error{"cannot create '" + file.string() + "'"};
        }
        _close(descriptor);
        return Status::Ok();
#else
        int flags = O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC | O_NONBLOCK;
#    if defined(O_NOFOLLOW)
        flags |= O_NOFOLLOW;
#    endif
        const int descriptor = open(file.c_str(), flags, S_IRUSR | S_IWUSR);
        if (descriptor < 0)
        {
            return Error{"cannot create '" + file.string() + "'"};
        }
        struct stat information = {};
        if (fstat(descriptor, &information) != 0 ||
            !S_ISREG(information.st_mode))
        {
            close(descriptor);
            return Error{"'" + file.string() + "' is not a regular file"};
        }
        // A file that existed keeps its mode; make it private too.
        fchmod(descriptor, S_IRUSR | S_IWUSR);
        close(descriptor);
        return Status::Ok();
#endif
    }

    std::string
    UniqueName()
    {
        static std::atomic<unsigned> counter{0};
        std::ostringstream text;
        text << CurrentProcessId() << '-'
             << std::chrono::steady_clock::now().time_since_epoch().count()
             << '-' << std::this_thread::get_id() << '-' << ++counter;
        return ToHex(Fnv1a64(text.str()));
    }
} // namespace Progmasoft::Catch3::Runner
