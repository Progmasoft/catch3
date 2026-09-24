// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#include <Progmasoft/Catch3/Results.hpp>
#include <Progmasoft/Catch3/XmlWriter.hpp>

#include <cmath>
#include <iomanip>
#include <locale>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace Progmasoft::Catch3 {

    namespace {

        struct DecodedCharacter final {
            char32_t Value = 0;
            std::size_t Length = 0;
        };

        DecodedCharacter DecodeUtf8( std::string_view text,
                                     std::size_t offset ) noexcept {
            const auto first = static_cast<unsigned char>( text[offset] );
            if ( first < 0x80 ) { return { first, 1 }; }

            std::size_t length = 0;
            char32_t value = 0;
            char32_t minimum = 0;
            if ( first >= 0xC2 && first <= 0xDF ) {
                length = 2;
                value = first & 0x1F;
                minimum = 0x80;
            } else if ( first >= 0xE0 && first <= 0xEF ) {
                length = 3;
                value = first & 0x0F;
                minimum = 0x800;
            } else if ( first >= 0xF0 && first <= 0xF4 ) {
                length = 4;
                value = first & 0x07;
                minimum = 0x10000;
            } else {
                return { 0xFFFD, 0 };
            }

            if ( offset + length > text.size() ) { return { 0xFFFD, 0 }; }
            for ( std::size_t index = 1; index < length; ++index ) {
                const auto continuation =
                    static_cast<unsigned char>( text[offset + index] );
                if ( ( continuation & 0xC0 ) != 0x80 ) { return { 0xFFFD, 0 }; }
                value = static_cast<char32_t>( ( value << 6 ) |
                                               ( continuation & 0x3F ) );
            }
            if ( value < minimum || value > 0x10FFFF ||
                 ( value >= 0xD800 && value <= 0xDFFF ) ) {
                return { 0xFFFD, 0 };
            }
            return { value, length };
        }

        bool IsXmlCharacter( char32_t value ) noexcept {
            return value == 0x09 || value == 0x0A || value == 0x0D ||
                   ( value >= 0x20 && value <= 0xD7FF ) ||
                   ( value >= 0xE000 && value <= 0xFFFD ) ||
                   ( value >= 0x10000 && value <= 0x10FFFF );
        }

        void WriteEscaped( std::string_view value,
                           std::ostream& output,
                           bool attribute ) {
            for ( std::size_t index = 0; index < value.size(); ) {
                const DecodedCharacter character = DecodeUtf8( value, index );
                if ( character.Length == 0 ||
                     !IsXmlCharacter( character.Value ) ) {
                    // Preserve parseability without silently dropping input.
                    output << "&#xFFFD;";
                    ++index;
                    continue;
                }

                if ( character.Value < 0x80 ) {
                    switch ( static_cast<char>( character.Value ) ) {
                    case '&':
                        output << "&amp;";
                        break;
                    case '<':
                        output << "&lt;";
                        break;
                    case '>':
                        output << "&gt;";
                        break;
                    case '"':
                        if ( attribute ) {
                            output << "&quot;";
                        } else {
                            output.put( '"' );
                        }
                        break;
                    case '\'':
                        if ( attribute ) {
                            output << "&apos;";
                        } else {
                            output.put( '\'' );
                        }
                        break;
                    default:
                        output.put( static_cast<char>( character.Value ) );
                    }
                } else {
                    output.write(
                        value.data() + index,
                        static_cast<std::streamsize>( character.Length ) );
                }
                index += character.Length;
            }
        }

        void WriteAttribute( std::ostream& output,
                             std::string_view name,
                             std::string_view value ) {
            output.put( ' ' );
            output.write( name.data(),
                          static_cast<std::streamsize>( name.size() ) );
            output << "=\"";
            WriteEscaped( value, output, true );
            output.put( '"' );
        }

        std::string FormatSeconds( double seconds ) {
            std::ostringstream formatted;
            formatted.imbue( std::locale::classic() );
            formatted << std::fixed << std::setprecision( 6 ) << seconds;
            return formatted.str();
        }

        void WriteIndent( std::ostream& output, unsigned int level ) {
            for ( unsigned int index = 0; index < level; ++index ) {
                output << "  ";
            }
        }

        void WriteTextElement( std::ostream& output,
                               unsigned int indent,
                               std::string_view name,
                               std::string_view value ) {
            WriteIndent( output, indent );
            output.put( '<' );
            output.write( name.data(),
                          static_cast<std::streamsize>( name.size() ) );
            output.put( '>' );
            WriteEscaped( value, output, false );
            output << "</";
            output.write( name.data(),
                          static_cast<std::streamsize>( name.size() ) );
            output << ">\n";
        }

        TestRunSummary SummarizeSuite( const TestSuiteResult& suite ) {
            return SummarizeTestSuite( suite );
        }

        void WriteTestCase( const TestCaseResult& testCase,
                            std::string_view suiteName,
                            std::ostream& output ) {
            const bool hasBody = testCase.Status != TestCaseStatus::kPassed ||
                                 !testCase.StandardOutput.empty() ||
                                 !testCase.StandardError.empty();
            WriteIndent( output, 2 );
            output << "<testcase";
            WriteAttribute( output,
                            "classname",
                            testCase.ClassName.empty() ? suiteName
                                                       : testCase.ClassName );
            WriteAttribute( output, "name", testCase.Name );
            WriteAttribute(
                output, "time", FormatSeconds( testCase.DurationSeconds ) );
            if ( !testCase.SourceFile.empty() ) {
                WriteAttribute( output, "file", testCase.SourceFile );
            }
            if ( !hasBody ) {
                output << "/>\n";
                return;
            }

            output << ">\n";
            if ( testCase.Status == TestCaseStatus::kFailed ||
                 testCase.Status == TestCaseStatus::kErrored ) {
                const bool isError =
                    testCase.Status == TestCaseStatus::kErrored;
                WriteIndent( output, 3 );
                output << ( isError ? "<error" : "<failure" );
                WriteAttribute( output, "message", testCase.Message );
                WriteAttribute( output,
                                "type",
                                isError ? "TestError" : "AssertionFailure" );
                output.put( '>' );
                WriteEscaped( testCase.Details.empty() ? testCase.Message
                                                       : testCase.Details,
                              output,
                              false );
                output << ( isError ? "</error>\n" : "</failure>\n" );
            } else if ( testCase.Status == TestCaseStatus::kSkipped ) {
                WriteIndent( output, 3 );
                output << "<skipped";
                WriteAttribute( output, "message", testCase.Message );
                output << "/>\n";
            }
            if ( !testCase.StandardOutput.empty() ) {
                WriteTextElement(
                    output, 3, "system-out", testCase.StandardOutput );
            }
            if ( !testCase.StandardError.empty() ) {
                WriteTextElement(
                    output, 3, "system-err", testCase.StandardError );
            }
            WriteIndent( output, 2 );
            output << "</testcase>\n";
        }

    } // namespace

    void JUnitXmlWriter::Write( const TestRunResult& result,
                                std::ostream& output ) {
        const TestRunSummary runSummary = SummarizeTestRun( result );
        output << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<testsuites";
        WriteAttribute( output, "name", result.Name );
        WriteAttribute( output, "tests", std::to_string( runSummary.Tests ) );
        WriteAttribute(
            output, "failures", std::to_string( runSummary.Failures ) );
        WriteAttribute( output, "errors", std::to_string( runSummary.Errors ) );
        WriteAttribute(
            output, "skipped", std::to_string( runSummary.Skipped ) );
        WriteAttribute(
            output, "time", FormatSeconds( runSummary.DurationSeconds ) );
        output << ">\n";

        for ( const TestSuiteResult& suite : result.TestSuites ) {
            const TestRunSummary suiteSummary = SummarizeSuite( suite );
            WriteIndent( output, 1 );
            output << "<testsuite";
            WriteAttribute( output, "name", suite.Name );
            WriteAttribute(
                output, "tests", std::to_string( suiteSummary.Tests ) );
            WriteAttribute(
                output, "failures", std::to_string( suiteSummary.Failures ) );
            WriteAttribute(
                output, "errors", std::to_string( suiteSummary.Errors ) );
            WriteAttribute(
                output, "skipped", std::to_string( suiteSummary.Skipped ) );
            WriteAttribute(
                output, "time", FormatSeconds( suiteSummary.DurationSeconds ) );
            output << ">\n";

            for ( const TestCaseResult& testCase : suite.TestCases ) {
                WriteTestCase( testCase, suite.Name, output );
            }
            WriteIndent( output, 1 );
            output << "</testsuite>\n";
        }
        output << "</testsuites>\n";

        if ( !output ) {
            throw std::ios_base::failure( "could not write JUnit XML results" );
        }
    }

    std::string JUnitXmlWriter::ToString( const TestRunResult& result ) {
        std::ostringstream output;
        Write( result, output );
        return output.str();
    }

} // namespace Progmasoft::Catch3
