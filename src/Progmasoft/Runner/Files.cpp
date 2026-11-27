// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: LGPL-3.0-or-later
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

#include <algorithm>
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

    std::string
    DeclaredInputsDigest(std::vector<std::string> paths,
                         std::vector<std::string> names)
    {
        if (paths.empty() && names.empty())
        {
            return {};
        }
        std::sort(paths.begin(), paths.end());
        paths.erase(std::unique(paths.begin(), paths.end()), paths.end());
        std::sort(names.begin(), names.end());
        names.erase(std::unique(names.begin(), names.end()), names.end());

        Sha256 digest;
        const auto file = [&digest](const std::string &label,
                                    const std::filesystem::path &path) {
            const auto contents = Sha256File(path);
            digest.Update("file\n" + label + "\n"
                          + (contents ? *contents : std::string{ "unreadable" })
                          + "\n");
        };
        for (const std::string &path : paths)
        {
            std::error_code error;
            const std::filesystem::path root{ path };
            if (std::filesystem::is_regular_file(root, error))
            {
                file(path, root);
                continue;
            }
            if (!std::filesystem::is_directory(root, error))
            {
                digest.Update("missing\n" + path + "\n");
                continue;
            }
            // Every regular file below the directory, in a fixed order, by
            // its path relative to the directory. Links to directories are
            // not followed, so a loop cannot make the walk endless.
            std::vector<std::filesystem::path> found;
            std::filesystem::recursive_directory_iterator walk{
                root,
                std::filesystem::directory_options::skip_permission_denied,
                error
            };
            for (const std::filesystem::recursive_directory_iterator end;
                 !error && walk != end;
                 walk.increment(error))
            {
                if (walk->is_regular_file(error))
                {
                    found.push_back(walk->path());
                }
            }
            std::sort(found.begin(), found.end());
            digest.Update("directory\n" + path + "\n");
            for (const auto &entry : found)
            {
                file(entry.lexically_relative(root).generic_string(), entry);
            }
            if (error)
            {
                digest.Update("incomplete\n");
            }
        }
        for (const std::string &name : names)
        {
            digest.Update(HasEnvironment(name.c_str())
                              ? "variable\n" + name + "\n"
                                    + Sha256Hex(ReadEnvironment(name.c_str()))
                                    + "\n"
                              : "unset\n" + name + "\n");
        }
        return digest.HexDigest();
    }
} // namespace Progmasoft::Catch3::Runner
