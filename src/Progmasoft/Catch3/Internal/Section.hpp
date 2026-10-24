
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0
#pragma once

#include <Progmasoft/Catch3/Internal/CompilerCapabilities.hpp>
#include <Progmasoft/Catch3/Internal/ConfigStaticAnalysisSupport.hpp>
#include <Progmasoft/Catch3/Internal/NonCopyable.hpp>
#include <Progmasoft/Catch3/SectionInfo.hpp>
#include <Progmasoft/Catch3/Timer.hpp>
#include <Progmasoft/Catch3/Totals.hpp>
#include <Progmasoft/Catch3/Internal/UniqueName.hpp>

namespace Catch {

    class Section : Detail::NonCopyable {
    public:
        Section( SectionInfo&& info );
        Section( SourceLineInfo const& _lineInfo,
                 StringRef _name,
                 const char* const = nullptr );
        ~Section();

        // This indicates whether the section should be executed or not
        explicit operator bool() const;

    private:
        SectionInfo m_info;

        Counts m_assertions;
        bool m_sectionIncluded;
        Timer m_timer;
    };

} // end namespace Catch

#if !defined(CATCH_CONFIG_EXPERIMENTAL_STATIC_ANALYSIS_SUPPORT)
#    define INTERNAL_CATCH_SECTION( ... )                                 \
        CATCH_INTERNAL_START_WARNINGS_SUPPRESSION                         \
        CATCH_INTERNAL_SUPPRESS_UNUSED_VARIABLE_WARNINGS                  \
        if ( Catch::Section const& INTERNAL_CATCH_UNIQUE_NAME(            \
                 catch_internal_Section ) =                               \
                 Catch::Section( CATCH_INTERNAL_LINEINFO, __VA_ARGS__ ) ) \
        CATCH_INTERNAL_STOP_WARNINGS_SUPPRESSION

#    define INTERNAL_CATCH_DYNAMIC_SECTION( ... )                     \
        CATCH_INTERNAL_START_WARNINGS_SUPPRESSION                     \
        CATCH_INTERNAL_SUPPRESS_UNUSED_VARIABLE_WARNINGS              \
        if ( Catch::Section const& INTERNAL_CATCH_UNIQUE_NAME(        \
                 catch_internal_Section ) =                           \
                 Catch::SectionInfo(                                  \
                     CATCH_INTERNAL_LINEINFO,                         \
                     ( Catch::ReusableStringStream() << __VA_ARGS__ ) \
                         .str() ) )                                   \
        CATCH_INTERNAL_STOP_WARNINGS_SUPPRESSION

#else

// These section definitions imply that at most one section at one level
// will be entered (because only one section's __LINE__ can be equal to
// the dummy `catchInternalSectionHint` variable from `TEST_CASE`).

namespace Catch {
    namespace Detail {
        // Intentionally without linkage, as it should only be used as a dummy
        // symbol for static analysis.
        // The arguments are used as a dummy for checking warnings in the passed
        // expressions.
        int GetNewSectionHint( StringRef, const char* const = nullptr );
    } // namespace Detail
} // namespace Catch


#    define INTERNAL_CATCH_SECTION( ... )                                   \
        CATCH_INTERNAL_START_WARNINGS_SUPPRESSION                           \
        CATCH_INTERNAL_SUPPRESS_UNUSED_VARIABLE_WARNINGS                    \
        CATCH_INTERNAL_SUPPRESS_SHADOW_WARNINGS                             \
        if ( [[maybe_unused]] const int catchInternalPreviousSectionHint =  \
                 catchInternalSectionHint,                                  \
             catchInternalSectionHint =                                     \
                 Catch::Detail::GetNewSectionHint(__VA_ARGS__);             \
             catchInternalPreviousSectionHint == __LINE__ )                 \
        CATCH_INTERNAL_STOP_WARNINGS_SUPPRESSION

#    define INTERNAL_CATCH_DYNAMIC_SECTION( ... )                           \
        CATCH_INTERNAL_START_WARNINGS_SUPPRESSION                           \
        CATCH_INTERNAL_SUPPRESS_UNUSED_VARIABLE_WARNINGS                    \
        CATCH_INTERNAL_SUPPRESS_SHADOW_WARNINGS                             \
        if ( [[maybe_unused]] const int catchInternalPreviousSectionHint =  \
                 catchInternalSectionHint,                                  \
             catchInternalSectionHint = Catch::Detail::GetNewSectionHint(   \
                ( Catch::ReusableStringStream() << __VA_ARGS__ ).str());    \
             catchInternalPreviousSectionHint == __LINE__ )                 \
        CATCH_INTERNAL_STOP_WARNINGS_SUPPRESSION

#endif


