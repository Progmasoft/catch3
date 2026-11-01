// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0
#pragma once

#include <Progmasoft/Catch3/Internal/Enforce.hpp>
#include <Progmasoft/Catch3/Internal/SourceLineInfo.hpp>
#include <Progmasoft/Catch3/Matchers/Matchers.hpp>
#include <Progmasoft/Catch3/Matchers/Templated.hpp>
#include <Progmasoft/Catch3/Matchers/Vector.hpp>
#include <Progmasoft/Catch3/ToString.hpp>
#include <cstddef>
#include <exception>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace Progmasoft::Catch3
{
    template<typename Signature>
    class MockExpectation;
    template<typename Signature>
    class MockFunction;

    namespace Detail
    {
        inline constexpr std::size_t kUnlimitedCalls
            = std::numeric_limits<std::size_t>::max();

        class MockSequence;

        /// What the checks need to know of an expectation, whatever the
        /// signature of its mock.
        struct MockExpectationState final
        {
            std::string Description;
            Catch::SourceLineInfo Line;
            std::size_t Minimum = 1;
            std::size_t Maximum = 1;
            std::size_t Calls = 0;
            bool Retired = false;
            std::vector<std::shared_ptr<MockSequence>> Sequences;

            MockExpectationState(std::string description,
                                 const Catch::SourceLineInfo &line)
                : Description(std::move(description))
                , Line(line)
            {}

            [[nodiscard]] bool
            Satisfied() const noexcept
            {
                return Calls >= Minimum;
            }

            /// Whether the expectation can take one more call.
            [[nodiscard]] bool
            Available() const noexcept
            {
                return !Retired && Calls < Maximum;
            }
        };

        /// The steps of a sequence. It does not keep them alive: a step whose
        /// mock is gone was verified with that mock.
        class MockSequence final
        {
        public:
            void
            Add(const std::shared_ptr<MockExpectationState> &step);

            /// Moves to `step`, retiring the steps before it. Returns what
            /// was left undone, empty when the call came in order.
            [[nodiscard]] std::string
            Enter(const MockExpectationState &step);

        private:
            std::vector<std::weak_ptr<MockExpectationState>> steps;
            std::size_t current = 0;
        };

        /// All mocks share one lock, since a sequence can span them.
        [[nodiscard]] std::unique_lock<std::recursive_mutex>
        LockMocks();

        /// Fails the running test case at `line` and never throws, since a
        /// mock can stand in for a `noexcept` function or be in a destructor.
        /// Outside of a test case the message goes to standard error.
        void
        ReportMockFailure(const Catch::SourceLineInfo &line,
                          const std::string &mock,
                          std::string message);

        /// Fails the running test case and leaves it, for a call nothing can
        /// return from. In a `noexcept` function that ends the process.
        [[noreturn]] void
        AbortMockCall(const Catch::SourceLineInfo &line,
                      const std::string &mock,
                      std::string message);

        /// Counts a met expectation as a passed assertion.
        void
        ReportMockSuccess(const Catch::SourceLineInfo &line,
                          const std::string &mock,
                          std::string message);

        /// "exactly 2 calls", "at least 1 call", "any number of calls".
        [[nodiscard]] std::string
        DescribeCalls(std::size_t minimum, std::size_t maximum);

        /// "called once" and the like, for a report.
        [[nodiscard]] std::string
        DescribeCount(std::size_t calls);

        template<std::size_t Index, typename Signature>
        struct MockArgumentOf;

        template<std::size_t Index, typename... Arguments>
        struct MockArgumentOf<Index, void(Arguments...)>
        {
            using Type = std::tuple_element_t<Index, std::tuple<Arguments...>>;
        };

        /// The type of a parameter of a mocked method.
        template<std::size_t Index, typename Signature>
        using MockArgument = typename MockArgumentOf<Index, Signature>::Type;

        template<typename T>
        struct IsComposedMatcher: std::false_type
        {
        };
        template<typename T>
        struct IsComposedMatcher<Catch::Matchers::Detail::MatchAllOf<T>>
            : std::true_type
        {
        };
        template<typename T>
        struct IsComposedMatcher<Catch::Matchers::Detail::MatchAnyOf<T>>
            : std::true_type
        {
        };
        template<typename T>
        struct IsComposedMatcher<Catch::Matchers::Detail::MatchNotOf<T>>
            : std::true_type
        {
        };
        template<typename... Ts>
        struct IsComposedMatcher<
            Catch::Matchers::Detail::MatchAllOfGeneric<Ts...>>: std::true_type
        {
        };
        template<typename... Ts>
        struct IsComposedMatcher<
            Catch::Matchers::Detail::MatchAnyOfGeneric<Ts...>>: std::true_type
        {
        };
        template<typename T>
        struct IsComposedMatcher<Catch::Matchers::Detail::MatchNotOfGeneric<T>>
            : std::true_type
        {
        };

        /// Catch's vector matchers refer to what they compare with.
        template<typename T>
        struct HoldsReference: std::false_type
        {
        };
        template<typename T, typename Alloc>
        struct HoldsReference<
            Catch::Matchers::VectorContainsElementMatcher<T, Alloc>>
            : std::true_type
        {
        };
        template<typename T, typename AllocComp, typename AllocMatch>
        struct HoldsReference<
            Catch::Matchers::ContainsMatcher<T, AllocComp, AllocMatch>>
            : std::true_type
        {
        };
        template<typename T, typename AllocComp, typename AllocMatch>
        struct HoldsReference<
            Catch::Matchers::EqualsMatcher<T, AllocComp, AllocMatch>>
            : std::true_type
        {
        };
        template<typename T, typename AllocComp, typename AllocMatch>
        struct HoldsReference<
            Catch::Matchers::ApproxMatcher<T, AllocComp, AllocMatch>>
            : std::true_type
        {
        };
        template<typename T, typename AllocComp, typename AllocMatch>
        struct HoldsReference<
            Catch::Matchers::UnorderedEqualsMatcher<T, AllocComp, AllocMatch>>
            : std::true_type
        {
        };

        /// Whether two integers have the same value, whatever their types.
        template<typename Left, typename Right>
        [[nodiscard]] constexpr bool
        SameInteger(Left left, Right right) noexcept
        {
            if constexpr (std::is_signed_v<Left> == std::is_signed_v<Right>)
            {
                return left == right;
            }
            else if constexpr (std::is_signed_v<Left>)
            {
                return left >= 0
                       && static_cast<std::make_unsigned_t<Left>>(left)
                              == right;
            }
            else
            {
                return right >= 0
                       && left
                              == static_cast<std::make_unsigned_t<Right>>(
                                  right);
            }
        }

        /// How one argument of a call is judged.
        template<typename Argument>
        struct ArgumentMatcher final
        {
            std::function<bool(const Argument &)> Test;
            std::string Description;
        };
    } // namespace Detail

    /// Accepts any value of an argument.
    struct AnyArgument final
    {};
    inline constexpr AnyArgument Any{};

    /// Accepts the values `predicate` accepts; `description` names it in
    /// reports.
    template<typename Predicate>
    struct WhereArgument final
    {
        Predicate Test;
        std::string Description;
    };

    template<typename Predicate>
    [[nodiscard]] WhereArgument<std::decay_t<Predicate>>
    Where(Predicate &&predicate, std::string description)
    {
        return { std::forward<Predicate>(predicate), std::move(description) };
    }

    namespace Detail
    {
        template<typename Argument, typename Expected>
        [[nodiscard]] ArgumentMatcher<Argument>
        MakeArgumentMatcher(Expected &&expected)
        {
            using Stored = std::decay_t<Expected>;
            if constexpr (std::is_same_v<Stored, AnyArgument>)
            {
                return { [](const Argument &) {
                            return true;
                        },
                         "any" };
            }
            else if constexpr (std::is_base_of_v<
                                   Catch::Matchers::MatcherUntypedBase,
                                   Stored>)
            {
                static_assert(!IsComposedMatcher<Stored>::value,
                              "A composed matcher refers to temporaries that "
                              "are gone when the mock is called; use Where.");
                static_assert(!HoldsReference<Stored>::value,
                              "A vector matcher refers to its vector, which "
                              "can be gone when the mock is called; use "
                              "Where.");
                auto matcher = std::make_shared<Stored>(
                    std::forward<Expected>(expected));
                auto description = matcher->toString();
                return { [matcher](const Argument &value) {
                            return matcher->match(value);
                        },
                         std::move(description) };
            }
            else if constexpr (!std::is_null_pointer_v<Stored>
                               && std::is_constructible_v<std::string, Stored>
                               && std::is_constructible_v<std::string,
                                                          Argument>)
            {
                // Strings are compared by value, also as `const char *`.
                if constexpr (std::is_pointer_v<Stored>)
                {
                    if (expected == nullptr)
                    {
                        return { []([[maybe_unused]] const Argument &value) {
                                    if constexpr (std::is_pointer_v<Argument>)
                                    {
                                        return value == nullptr;
                                    }
                                    else
                                    {
                                        return false;
                                    }
                                },
                                 "nullptr" };
                    }
                }
                std::string text{ std::forward<Expected>(expected) };
                auto description = Catch::Detail::stringify(text);
                return { [text = std::move(text)](const Argument &value) {
                            if constexpr (std::is_pointer_v<Argument>)
                            {
                                return value != nullptr && text == value;
                            }
                            else
                            {
                                return text == value;
                            }
                        },
                         std::move(description) };
            }
            else if constexpr (std::is_integral_v<Argument>
                               && std::is_integral_v<Stored>
                               && !std::is_same_v<Argument, bool>
                               && !std::is_same_v<Stored, bool>)
            {
                // A literal is an int, whatever the type of the parameter.
                const Stored value = expected;
                return { [value](const Argument &argument) {
                            return SameInteger(argument, value);
                        },
                         Catch::Detail::stringify(value) };
            }
            else
            {
                auto description = Catch::Detail::stringify(expected);
                return { [value = std::forward<Expected>(expected)](
                             const Argument &argument) {
                            return argument == value;
                        },
                         std::move(description) };
            }
        }

        template<typename Argument, typename Predicate>
        [[nodiscard]] ArgumentMatcher<Argument>
        MakeArgumentMatcher(WhereArgument<Predicate> where)
        {
            return { std::move(where.Test), std::move(where.Description) };
        }
    } // namespace Detail

    /// Calls that have to come in the order their expectations were declared
    /// in, across mocks.
    class Sequence final
    {
    public:
        Sequence()
            : state(std::make_shared<Detail::MockSequence>())
        {}

    private:
        template<typename Signature>
        friend class MockExpectation;

        std::shared_ptr<Detail::MockSequence> state;
    };

    /// One expected kind of call of a mock. It expects one call until told
    /// otherwise.
    template<typename Result, typename... Arguments>
    class MockExpectation<Result(Arguments...)> final
    {
    public:
        MockExpectation &
        Times(std::size_t count)
        {
            return Between(count, count);
        }

        MockExpectation &
        AtLeast(std::size_t count)
        {
            return Between(count, Detail::kUnlimitedCalls);
        }

        MockExpectation &
        AtMost(std::size_t count)
        {
            return Between(0, count);
        }

        MockExpectation &
        AnyTimes()
        {
            return Between(0, Detail::kUnlimitedCalls);
        }

        MockExpectation &
        Never()
        {
            return Between(0, 0);
        }

        MockExpectation &
        Between(std::size_t minimum, std::size_t maximum)
        {
            CATCH_ENFORCE(minimum <= maximum,
                          "A mock expectation needs its minimum at most its "
                          "maximum");
            const auto lock = Detail::LockMocks();
            state->Minimum = minimum;
            state->Maximum = maximum;
            return *this;
        }

        /// Takes this step in `sequence`, after the ones declared before it.
        MockExpectation &
        InSequence(const Sequence &sequence)
        {
            const auto lock = Detail::LockMocks();
            sequence.state->Add(state);
            state->Sequences.push_back(sequence.state);
            return *this;
        }

        /// Returns `value`. A type that cannot be copied is moved out on the
        /// first call; a reference result refers to `value`, which has to
        /// outlive the mock.
        template<typename Value>
        MockExpectation &
        Returns(Value &&value)
        {
            static_assert(!std::is_void_v<Result>,
                          "A mock of a void function returns nothing.");
            if constexpr (std::is_reference_v<Result>)
            {
                static_assert(std::is_lvalue_reference_v<Value>,
                              "A reference result needs an object that "
                              "outlives the mock.");
                auto *target = &value;
                return Does([target](Arguments...) -> Result {
                    return static_cast<Result>(*target);
                });
            }
            else if constexpr (std::is_copy_constructible_v<Result>)
            {
                return Does([held = static_cast<Result>(std::forward<Value>(
                                 value))](Arguments...) -> Result {
                    return held;
                });
            }
            else
            {
                auto held
                    = std::make_shared<Result>(std::forward<Value>(value));
                return Does([held](Arguments...) -> Result {
                    // Calls can come from several threads.
                    const auto lock = Detail::LockMocks();
                    return std::move(*held);
                });
            }
        }

        /// Runs `action` with the arguments of each call; what it returns is
        /// what the call returns.
        template<typename Action>
        MockExpectation &
        Does(Action &&work)
        {
            const auto lock = Detail::LockMocks();
            action = std::make_shared<std::function<Result(Arguments...)>>(
                std::forward<Action>(work));
            return *this;
        }

        /// Throws `exception` from each call.
        template<typename Exception>
        MockExpectation &
        Throws(Exception exception)
        {
            return Does(
                [exception = std::move(exception)](Arguments...) -> Result {
                    Catch::throw_exception(exception);
                });
        }

        /// What the arguments of a call have to match.
        using Matchers
            = std::tuple<Detail::ArgumentMatcher<std::decay_t<Arguments>>...>;

    private:
        template<typename Signature>
        friend class MockFunction;

        MockExpectation(std::string description,
                        const Catch::SourceLineInfo &line,
                        std::unique_ptr<Matchers> accepted)
            : state(std::make_shared<Detail::MockExpectationState>(
                  std::move(description),
                  line))
            , matchers(std::move(accepted))
        {}

        template<std::size_t... Indices>
        [[nodiscard]] bool
        Accepts([[maybe_unused]] const std::tuple<
                    const std::decay_t<Arguments> &...> &values,
                std::index_sequence<Indices...>) const
        {
            return (std::get<Indices>(*matchers).Test(std::get<Indices>(values))
                    && ...);
        }

        [[nodiscard]] bool
        Accepts(
            const std::tuple<const std::decay_t<Arguments> &...> &values) const
        {
            return matchers == nullptr
                   || Accepts(values, std::index_sequence_for<Arguments...>{});
        }

        std::shared_ptr<Detail::MockExpectationState> state;
        // Empty when any arguments will do.
        std::unique_ptr<Matchers> matchers;
        std::shared_ptr<std::function<Result(Arguments...)>> action;
    };

    /// A mock of one function: a callable that checks its calls against the
    /// expectations set on it and fails the running test case when they do
    /// not hold. It is what `CATCH3_MOCK_METHOD` declares, and on its own it
    /// stands in for a free function behind a seam. Its destructor verifies
    /// it.
    template<typename Result, typename... Arguments>
    class MockFunction<Result(Arguments...)> final
    {
        using Expectation = MockExpectation<Result(Arguments...)>;

    public:
        /// Declares an expectation at `line`; called with what the arguments
        /// have to match. See `CATCH3_EXPECT_CALL`.
        class ExpectationStart final
        {
        public:
            template<typename... Expected>
            Expectation &
            operator()(Expected &&...expected)
            {
                constexpr auto arguments = sizeof...(Arguments);
                static_assert(sizeof...(Expected) == 0
                                  || sizeof...(Expected) == arguments,
                              "Match every argument of the mock, or none to "
                              "accept any.");
                if constexpr (sizeof...(Expected) == arguments
                              && arguments != 0)
                {
                    return mock.Add(
                        line,
                        std::make_unique<typename Expectation::Matchers>(
                            Detail::MakeArgumentMatcher<
                                std::decay_t<Arguments>>(
                                std::forward<Expected>(expected))...));
                }
                else
                {
                    return mock.Add(line, nullptr);
                }
            }

        private:
            friend class MockFunction;

            ExpectationStart(MockFunction &function,
                             const Catch::SourceLineInfo &place)
                : mock(function)
                , line(place)
            {}

            MockFunction &mock;
            Catch::SourceLineInfo line;
        };

        /// `place` is where a call no expectation takes is reported when the
        /// mock has no expectations to point at.
        explicit MockFunction(std::string mockName = "mock",
                              const Catch::SourceLineInfo &place
                              = CATCH_INTERNAL_LINEINFO)
            : name(std::move(mockName))
            , declared(place)
            , exceptions(std::uncaught_exceptions())
        {}

        MockFunction(const MockFunction &) = delete;
        MockFunction &
        operator=(const MockFunction &) = delete;

        ~MockFunction()
        {
            // A test case that is being left has failed for its own reason,
            // and what it did not get to call says nothing more.
            if (std::uncaught_exceptions() <= exceptions)
            {
                VerifyAll();
            }
        }

        [[nodiscard]] ExpectationStart
        Expect(const Catch::SourceLineInfo &line)
        {
            return { *this, line };
        }

        /// Checks the expectations now and forgets them, so that a test case
        /// can go on with new ones.
        void
        Verify()
        {
            VerifyAll();
        }

        Result
        operator()(Arguments... arguments)
        {
            const Decision decision = Decide(
                std::tuple<const std::decay_t<Arguments> &...>{ arguments... });
            if (!decision.line)
            {
                return Refuse(decision.message);
            }
            if (!decision.message.empty())
            {
                Detail::ReportMockFailure(*decision.line,
                                          name,
                                          decision.message);
            }
            if (decision.action != nullptr)
            {
                return (*decision.action)(
                    std::forward<Arguments>(arguments)...);
            }
            if constexpr (std::is_void_v<Result>)
            {
                return;
            }
            else if constexpr (std::is_default_constructible_v<Result>)
            {
                return Result{};
            }
            else
            {
                Detail::AbortMockCall(*decision.line,
                                      name,
                                      "nothing to return from " + name
                                          + ": give the expectation Returns "
                                            "or Does");
            }
        }

        /// A function that calls this mock, which has to outlive it.
        [[nodiscard]] std::function<Result(Arguments...)>
        AsFunction()
        {
            return [this](Arguments... arguments) -> Result {
                return (*this)(std::forward<Arguments>(arguments)...);
            };
        }

    private:
        struct Decision final
        {
            // Empty when the call is refused.
            std::optional<Catch::SourceLineInfo> line;
            std::shared_ptr<std::function<Result(Arguments...)>> action;
            std::string message;
        };

        Expectation &
        Add(const Catch::SourceLineInfo &line,
            std::unique_ptr<typename Expectation::Matchers> matchers)
        {
            auto description = Describe(matchers.get());
            const auto lock = Detail::LockMocks();
            expectations.push_back(std::unique_ptr<Expectation>(
                new Expectation(std::move(description),
                                line,
                                std::move(matchers))));
            return *expectations.back();
        }

        template<std::size_t... Indices>
        [[nodiscard]] std::string
        DescribeMatchers(
            [[maybe_unused]] const typename Expectation::Matchers &matchers,
            std::index_sequence<Indices...>) const
        {
            std::string text;
            ((text += (Indices == 0 ? "" : ", ")
                      + std::get<Indices>(matchers).Description),
             ...);
            return text;
        }

        [[nodiscard]] std::string
        Describe(const typename Expectation::Matchers *matchers) const
        {
            if (matchers != nullptr)
            {
                return name + "("
                       + DescribeMatchers(
                           *matchers,
                           std::index_sequence_for<Arguments...>{})
                       + ")";
            }
            return sizeof...(Arguments) == 0 ? name + "()" : name + "(...)";
        }

        template<std::size_t... Indices>
        [[nodiscard]] std::string
        DescribeCall([[maybe_unused]] const std::tuple<
                         const std::decay_t<Arguments> &...> &values,
                     std::index_sequence<Indices...>) const
        {
            std::string text = name + "(";
            ((text += (Indices == 0 ? "" : ", ")
                      + Catch::Detail::stringify(std::get<Indices>(values))),
             ...);
            return text + ")";
        }

        /// Picks the first declared expectation that accepts the call and can
        /// take it, and counts the call.
        [[nodiscard]] Decision
        Decide(const std::tuple<const std::decay_t<Arguments> &...> &values)
        {
            const auto lock = Detail::LockMocks();
            const Expectation *used = nullptr;
            for (const auto &expectation : expectations)
            {
                if (!expectation->Accepts(values))
                {
                    continue;
                }
                if (expectation->state->Available())
                {
                    return Take(*expectation);
                }
                used = expectation.get();
            }
            const auto call
                = DescribeCall(values, std::index_sequence_for<Arguments...>{});
            Decision refused;
            refused.message = used != nullptr ? DescribeUsed(call, *used->state)
                                              : DescribeUnexpected(call);
            return refused;
        }

        [[nodiscard]] Decision
        Take(const Expectation &expectation)
        {
            auto &state = *expectation.state;
            ++state.Calls;
            Decision decision;
            decision.line = state.Line;
            decision.action = expectation.action;
            for (const auto &sequence : state.Sequences)
            {
                decision.message += sequence->Enter(state);
            }
            return decision;
        }

        [[nodiscard]] static std::string
        DescribeUsed(const std::string &call,
                     const Detail::MockExpectationState &state)
        {
            return state.Retired ? "call out of order: " + call
                                       + ", its step of the sequence has passed"
                                 : "too many calls: " + call + ", expected "
                                       + Detail::DescribeCalls(state.Minimum,
                                                               state.Maximum)
                                       + " of " + state.Description;
        }

        [[nodiscard]] std::string
        DescribeUnexpected(const std::string &call) const
        {
            std::string text = "unexpected call: " + call;
            if (expectations.empty())
            {
                return text + ", nothing was expected of this mock";
            }
            text += ", expected";
            for (const auto &expectation : expectations)
            {
                text += "\n  " + expectation->state->Description;
            }
            return text;
        }

        /// Reports a call no expectation takes, and returns what the call
        /// can return.
        Result
        Refuse(const std::string &message)
        {
            const auto line = FirstLine();
            if constexpr (std::is_void_v<Result>)
            {
                Detail::ReportMockFailure(line, name, message);
            }
            else if constexpr (std::is_default_constructible_v<Result>)
            {
                Detail::ReportMockFailure(line, name, message);
                return Result{};
            }
            else
            {
                Detail::AbortMockCall(line, name, message);
            }
        }

        /// Where to point at for a call that matches nothing: the first
        /// expectation, or the mock when it has none.
        [[nodiscard]] Catch::SourceLineInfo
        FirstLine() const
        {
            const auto lock = Detail::LockMocks();
            return expectations.empty() ? declared
                                        : expectations.front()->state->Line;
        }

        void
        VerifyAll()
        {
            std::vector<std::unique_ptr<Expectation>> checked;
            {
                const auto lock = Detail::LockMocks();
                checked.swap(expectations);
            }
            for (const auto &expectation : checked)
            {
                const auto &state = *expectation->state;
                if (state.Satisfied())
                {
                    Detail::ReportMockSuccess(state.Line,
                                              name,
                                              state.Description);
                }
                else
                {
                    Detail::ReportMockFailure(
                        state.Line,
                        name,
                        "unmet expectation: " + state.Description + " needs "
                            + Detail::DescribeCalls(state.Minimum,
                                                    state.Maximum)
                            + ", " + Detail::DescribeCount(state.Calls));
                }
            }
        }

        std::string name;
        Catch::SourceLineInfo declared;
        int exceptions;
        std::vector<std::unique_ptr<Expectation>> expectations;
    };

    /// Points a seam, a `std::function` or a function pointer that production
    /// code calls through, at `replacement` until the end of the scope.
    template<typename Seam>
    class ScopedSeam final
    {
    public:
        ScopedSeam(Seam &target, std::enable_if_t<true, Seam> replacement)
            : seam(target)
            , original(std::move(target))
        {
            seam = std::move(replacement);
        }

        ScopedSeam(const ScopedSeam &) = delete;
        ScopedSeam &
        operator=(const ScopedSeam &) = delete;

        ~ScopedSeam()
        {
            seam = std::move(original);
        }

    private:
        Seam &seam;
        Seam original;
    };

} // namespace Progmasoft::Catch3

// How many types a parenthesized list holds, up to 8, including none. A type
// with a comma of its own needs an alias.
#define CATCH3_INTERNAL_MOCK_EXPAND(...) __VA_ARGS__
#define CATCH3_INTERNAL_MOCK_CONCAT_IMPL(a, b) a##b
#define CATCH3_INTERNAL_MOCK_CONCAT(a, b) CATCH3_INTERNAL_MOCK_CONCAT_IMPL(a, b)
#define CATCH3_INTERNAL_MOCK_FIRST(first, ...) first
#define CATCH3_INTERNAL_MOCK_SECOND_OF(first, second, ...) second
#define CATCH3_INTERNAL_MOCK_SECOND(...)                                       \
    CATCH3_INTERNAL_MOCK_EXPAND(CATCH3_INTERNAL_MOCK_SECOND_OF(__VA_ARGS__))
#define CATCH3_INTERNAL_MOCK_PROBE(...) ~, 1
#define CATCH3_INTERNAL_MOCK_IS_EMPTY(argument)                                \
    CATCH3_INTERNAL_MOCK_EXPAND(                                               \
        CATCH3_INTERNAL_MOCK_SECOND(CATCH3_INTERNAL_MOCK_PROBE argument(),     \
                                    0, ))
#define CATCH3_INTERNAL_MOCK_SELECT(a1,                                        \
                                    a2,                                        \
                                    a3,                                        \
                                    a4,                                        \
                                    a5,                                        \
                                    a6,                                        \
                                    a7,                                        \
                                    a8,                                        \
                                    a9,                                        \
                                    count,                                     \
                                    ...)                                       \
    count
#define CATCH3_INTERNAL_MOCK_COUNT_0(...)                                      \
    CATCH3_INTERNAL_MOCK_EXPAND(CATCH3_INTERNAL_MOCK_SELECT(__VA_ARGS__,       \
                                                            9,                 \
                                                            8,                 \
                                                            7,                 \
                                                            6,                 \
                                                            5,                 \
                                                            4,                 \
                                                            3,                 \
                                                            2,                 \
                                                            1,                 \
                                                            0))
#define CATCH3_INTERNAL_MOCK_COUNT_1(...) 0
#define CATCH3_INTERNAL_MOCK_COUNT(...)                                        \
    CATCH3_INTERNAL_MOCK_EXPAND(CATCH3_INTERNAL_MOCK_CONCAT(                   \
        CATCH3_INTERNAL_MOCK_COUNT_,                                           \
        CATCH3_INTERNAL_MOCK_IS_EMPTY(CATCH3_INTERNAL_MOCK_EXPAND(             \
            CATCH3_INTERNAL_MOCK_FIRST(__VA_ARGS__, ))))(__VA_ARGS__))

#define CATCH3_INTERNAL_MOCK_ARGUMENT(signature, index)                        \
    ::Progmasoft::Catch3::Detail::MockArgument<index, signature>
#define CATCH3_INTERNAL_MOCK_PARAMETER(signature, index)                       \
    CATCH3_INTERNAL_MOCK_ARGUMENT(signature, index) argument##index
#define CATCH3_INTERNAL_MOCK_FORWARD(signature, index)                         \
    static_cast<CATCH3_INTERNAL_MOCK_ARGUMENT(signature, index) &&>(           \
        argument##index)

#define CATCH3_INTERNAL_MOCK_LIST_0(each, s)
#define CATCH3_INTERNAL_MOCK_LIST_1(each, s) each(s, 0)
#define CATCH3_INTERNAL_MOCK_LIST_2(each, s)                                   \
    CATCH3_INTERNAL_MOCK_LIST_1(each, s), each(s, 1)
#define CATCH3_INTERNAL_MOCK_LIST_3(each, s)                                   \
    CATCH3_INTERNAL_MOCK_LIST_2(each, s), each(s, 2)
#define CATCH3_INTERNAL_MOCK_LIST_4(each, s)                                   \
    CATCH3_INTERNAL_MOCK_LIST_3(each, s), each(s, 3)
#define CATCH3_INTERNAL_MOCK_LIST_5(each, s)                                   \
    CATCH3_INTERNAL_MOCK_LIST_4(each, s), each(s, 4)
#define CATCH3_INTERNAL_MOCK_LIST_6(each, s)                                   \
    CATCH3_INTERNAL_MOCK_LIST_5(each, s), each(s, 5)
#define CATCH3_INTERNAL_MOCK_LIST_7(each, s)                                   \
    CATCH3_INTERNAL_MOCK_LIST_6(each, s), each(s, 6)
#define CATCH3_INTERNAL_MOCK_LIST_8(each, s)                                   \
    CATCH3_INTERNAL_MOCK_LIST_7(each, s), each(s, 7)
#define CATCH3_INTERNAL_MOCK_LIST(count, each, signature)                      \
    CATCH3_INTERNAL_MOCK_CONCAT(CATCH3_INTERNAL_MOCK_LIST_, count)             \
    (each, signature)

#define CATCH3_INTERNAL_MOCK_DECLARE(result,                                   \
                                     name,                                     \
                                     field,                                    \
                                     arguments,                                \
                                     count,                                    \
                                     specifiers)                               \
    result name(CATCH3_INTERNAL_MOCK_LIST(count,                               \
                                          CATCH3_INTERNAL_MOCK_PARAMETER,      \
                                          void arguments)) specifiers          \
    {                                                                          \
        return field(CATCH3_INTERNAL_MOCK_LIST(count,                          \
                                               CATCH3_INTERNAL_MOCK_FORWARD,   \
                                               void arguments));               \
    }                                                                          \
    mutable ::Progmasoft::Catch3::MockFunction<result arguments> field         \
    {                                                                          \
        #name, CATCH_INTERNAL_LINEINFO                                         \
    }
#define CATCH3_INTERNAL_MOCK_SPLIT(result, name, field, arguments, ...)        \
    CATCH3_INTERNAL_MOCK_EXPAND(CATCH3_INTERNAL_MOCK_DECLARE(                  \
        result,                                                                \
        name,                                                                  \
        field,                                                                 \
        arguments,                                                             \
        CATCH3_INTERNAL_MOCK_EXPAND(CATCH3_INTERNAL_MOCK_COUNT arguments),     \
        CATCH3_INTERNAL_MOCK_EXPAND(                                           \
            CATCH3_INTERNAL_MOCK_FIRST(__VA_ARGS__, ))))

/// Declares the member `name` of a mock class and, beside it, the mock
/// `name##Mock` that `CATCH3_EXPECT_CALL` takes. The parenthesized list of
/// parameter types comes next, and then what goes after the parameters:
/// `CATCH3_MOCK_METHOD(int, Get, (const Key &), const override);`. A type with
/// a comma in it needs an alias.
#define CATCH3_MOCK_METHOD(result, name, ...)                                  \
    CATCH3_MOCK_OVERLOAD(result, name, name##Mock, __VA_ARGS__)

/// `CATCH3_MOCK_METHOD` for one of several overloads, whose mock needs a name
/// of its own: `field`.
#define CATCH3_MOCK_OVERLOAD(result, name, field, ...)                         \
    CATCH3_INTERNAL_MOCK_EXPAND(                                               \
        CATCH3_INTERNAL_MOCK_SPLIT(result, name, field, __VA_ARGS__, ))

/// Declares a `MockFunction` called `name` with a signature:
/// `CATCH3_MOCK_FUNCTION(clock, long());`.
#define CATCH3_MOCK_FUNCTION(name, ...)                                        \
    ::Progmasoft::Catch3::MockFunction<__VA_ARGS__> name                       \
    {                                                                          \
        #name, CATCH_INTERNAL_LINEINFO                                         \
    }

/// Expects a call of `mock`, a `MockFunction`, and takes what its arguments
/// have to match: values, Catch matchers, `Any` or `Where`, or nothing to
/// accept any. Failures of the expectation point at this line.
#define CATCH3_EXPECT_CALL(mock) (mock).Expect(CATCH_INTERNAL_LINEINFO)
