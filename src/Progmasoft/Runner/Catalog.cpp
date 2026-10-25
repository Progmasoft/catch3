// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#include <algorithm>
#include <Progmasoft/Catch3/TestCaseInfo.hpp>
#include <Progmasoft/Catch3/TestSpec.hpp>
#include <Progmasoft/Catch3/Interfaces/IConfig.hpp>
#include <Progmasoft/Catch3/Interfaces/IRegistryHub.hpp>
#include <Progmasoft/Catch3/Interfaces/ITestCaseRegistry.hpp>
#include <Progmasoft/Catch3/Internal/TestCaseRegistryImpl.hpp>
#include <cmath>
#include <iterator>
#include <map>

#include "Catalog.hpp"
#include "Hash.hpp"

namespace Progmasoft::Catch3::Runner
{
    namespace
    {
        std::optional<double>
        ParseSeconds(std::string_view text)
        {
            if (text.empty())
            {
                return std::nullopt;
            }
            double value = 0.0;
            double scale = 0.0;
            for (const char character : text)
            {
                if (character == '.' && scale <= 0.0)
                {
                    scale = 1.0;
                    continue;
                }
                if (character < '0' || character > '9')
                {
                    return std::nullopt;
                }
                if (scale > 0.0)
                {
                    scale /= 10.0;
                    value += (character - '0') * scale;
                }
                else
                {
                    value = value * 10.0 + (character - '0');
                }
            }
            return value;
        }

        bool
        HasPrefix(std::string_view text, std::string_view prefix)
        {
            return text.substr(0, prefix.size()) == prefix;
        }

        Status
        ApplyRunnerTag(TestEntry &entry, const std::string &tag)
        {
            const auto where = [&] {
                return "test '" + entry.name + "' at " + entry.file + ":"
                       + std::to_string(entry.line);
            };
            if (tag == "!serial")
            {
                entry.serial = true;
            }
            else if (tag == "!isolated")
            {
                entry.isolated = true;
            }
            else if (tag == "!quarantine")
            {
                entry.quarantined = true;
            }
            else if (HasPrefix(tag, "!resource:"))
            {
                entry.resources.push_back(tag.substr(10));
            }
            else if (HasPrefix(tag, "!depends:"))
            {
                entry.dependsOn.push_back(tag.substr(9));
            }
            else if (HasPrefix(tag, "!timeout:"))
            {
                const auto seconds = ParseSeconds(tag.substr(9));
                if (!seconds || *seconds > 1e7)
                {
                    return Error{ "[" + tag + "] of " + where()
                                  + " needs a number of seconds, such as "
                                    "[!timeout:30]" };
                }
                entry.timeout = std::chrono::milliseconds{
                    static_cast<long long>(std::llround(*seconds * 1000.0))
                };
            }
            else if (HasPrefix(tag, "!retries:"))
            {
                const auto count = ParseSeconds(tag.substr(9));
                if (!count || *count > 100 || std::floor(*count) < *count)
                {
                    return Error{ "[" + tag + "] of " + where()
                                  + " needs a whole number up to 100, such as "
                                    "[!retries:3]" };
                }
                entry.retries = static_cast<unsigned>(*count);
            }
            return Status::Ok();
        }
    } // namespace

    Expected<Catalog>
    Catalog::Load()
    {
        Catalog catalog;
        const auto &handles
            = Catch::getRegistryHub().getTestCaseRegistry().getAllTests();
        catalog.handles_ = handles;
        // Two tests with the same class, name and tags cannot be told apart
        // by any filter, history record or report, so they are refused.
        std::vector<const Catch::TestCaseInfo *> sorted;
        sorted.reserve(handles.size());
        for (const auto &handle : handles)
        {
            sorted.push_back(&handle.getTestCaseInfo());
        }
        std::stable_sort(sorted.begin(),
                         sorted.end(),
                         [](const auto *lhs, const auto *rhs) {
                             return *lhs < *rhs;
                         });
        const auto duplicate
            = std::adjacent_find(sorted.begin(),
                                 sorted.end(),
                                 [](const auto *lhs, const auto *rhs) {
                                     return *lhs == *rhs;
                                 });
        if (duplicate != sorted.end())
        {
            const auto *first = *duplicate;
            const auto *again = *std::next(duplicate);
            return Error{ "test case \"" + again->name + "\", with tags \""
                          + again->tagsAsString() + "\" already defined.\n"
                          + "\tFirst seen at "
                          + std::string{ first->lineInfo.file } + ":"
                          + std::to_string(first->lineInfo.line) + "\n"
                          + "\tRedefined at "
                          + std::string{ again->lineInfo.file } + ":"
                          + std::to_string(again->lineInfo.line) };
        }
        std::map<std::string, std::size_t> nameCounts;
        for (const auto &handle : handles)
        {
            const auto &info = handle.getTestCaseInfo();
            const std::string className{ info.className.data(),
                                         info.className.size() };
            ++nameCounts[className.empty() ? info.name
                                           : className + "::" + info.name];
        }
        Sha256 identity;
        catalog.tests_.reserve(handles.size());
        for (std::size_t index = 0; index < handles.size(); ++index)
        {
            const auto &info = handles[index].getTestCaseInfo();
            TestEntry entry;
            entry.index = index;
            entry.name = info.name;
            entry.className
                = std::string{ info.className.data(), info.className.size() };
            entry.file = std::string{ info.lineInfo.file };
            entry.line = info.lineInfo.line;
            entry.hidden = info.isHidden();
            entry.mayFail = info.okToFail() && !info.expectedToFail();
            entry.shouldFail = info.expectedToFail();
            for (const auto &tag : info.tags)
            {
                entry.tags.emplace_back(tag.original.data(),
                                        tag.original.size());
            }
            entry.benchmark
                = std::find(entry.tags.begin(), entry.tags.end(), "!benchmark")
                  != entry.tags.end();
            for (const auto &tag : entry.tags)
            {
                if (!tag.empty() && tag.front() == '!')
                {
                    if (auto status = ApplyRunnerTag(entry, tag); !status)
                    {
                        return status.GetError();
                    }
                }
            }
            const std::string base = entry.className.empty()
                                         ? entry.name
                                         : entry.className + "::" + entry.name;
            entry.key = base;
            if (nameCounts[base] > 1)
            {
                entry.key += " " + info.tagsAsString();
            }
            identity.Update(entry.key);
            identity.Update("\n");
            catalog.tests_.push_back(std::move(entry));
        }
        catalog.identity_ = identity.HexDigest();
        return catalog;
    }

    Catalog
    Catalog::FromEntries(std::vector<TestEntry> entries)
    {
        Catalog catalog;
        Sha256 identity;
        for (std::size_t index = 0; index < entries.size(); ++index)
        {
            TestEntry &entry = entries[index];
            entry.index = index;
            if (entry.key.empty())
            {
                entry.key = entry.name;
            }
            identity.Update(entry.key);
            identity.Update("\n");
        }
        catalog.tests_ = std::move(entries);
        catalog.identity_ = identity.HexDigest();
        return catalog;
    }

    const TestEntry *
    Catalog::Find(const std::string &keyOrName) const
    {
        for (const auto &entry : tests_)
        {
            if (entry.key == keyOrName)
            {
                return &entry;
            }
        }
        for (const auto &entry : tests_)
        {
            if (entry.name == keyOrName)
            {
                return &entry;
            }
        }
        return nullptr;
    }

    std::vector<std::size_t>
    Catalog::Match(const Catch::IConfig &config,
                   std::vector<std::string> &unmatched) const
    {
        std::vector<std::size_t> selected;
        const auto &spec = config.testSpec();
        if (!spec.hasFilters())
        {
            for (const auto &entry : tests_)
            {
                if (!entry.hidden
                    && Catch::isThrowSafe(handles_[entry.index], config))
                {
                    selected.push_back(entry.index);
                }
            }
            return selected;
        }
        std::vector<bool> chosen(tests_.size(), false);
        for (const auto &match : spec.matchesByFilter(handles_, config))
        {
            if (match.tests.empty())
            {
                unmatched.push_back(match.name);
            }
            for (const auto *handle : match.tests)
            {
                chosen[static_cast<std::size_t>(handle - handles_.data())]
                    = true;
            }
        }
        for (std::size_t index = 0; index < chosen.size(); ++index)
        {
            if (chosen[index])
            {
                selected.push_back(index);
            }
        }
        return selected;
    }
} // namespace Progmasoft::Catch3::Runner
