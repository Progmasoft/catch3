// Copyright (c) 2026 Progmasoft.
// SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1
#include <Progmasoft/Catch3.hpp>

#include <cstddef>
#include <deque>
#include <string>
#include <vector>

namespace {

    using Progmasoft::Catch3::Action;
    using Progmasoft::Catch3::Command;
    using Progmasoft::Catch3::PropertyOptions;
    using Progmasoft::Catch3::PropertyStatus;
    using Progmasoft::Catch3::SizeRange;
    namespace Gen = Progmasoft::Catch3::Gen;

    // A queue with a bug: past three elements it silently drops the
    // oldest one.
    class SmallQueue {
    public:
        void Push( int value ) {
            if ( values_.size() == 3 ) { values_.pop_front(); }
            values_.push_back( value );
        }
        int Pop() {
            const int value = values_.front();
            values_.pop_front();
            return value;
        }
        [[nodiscard]] std::size_t Size() const { return values_.size(); }

    private:
        std::deque<int> values_;
    };

    using Model = std::deque<int>;

    std::vector<Command<Model, SmallQueue>> QueueCommands() {
        Command<Model, SmallQueue> push{
            "push",
            []( const Model& ) {
                return Gen::Integers<int>( 0, 100 ).Map( []( int value ) {
                    return Action<Model, SmallQueue>{
                        "push(" + std::to_string( value ) + ")",
                        [value]( Model& model, SmallQueue& queue ) {
                            model.push_back( value );
                            queue.Push( value );
                            CATCH3_PROPERTY_ASSERT( queue.Size() ==
                                                    model.size() );
                        } };
                } );
            } };
        Command<Model, SmallQueue> pop{
            "pop",
            []( const Model& ) {
                return Gen::Just( Action<Model, SmallQueue>{
                    "pop", []( Model& model, SmallQueue& queue ) {
                        const int expected = model.front();
                        model.pop_front();
                        CATCH3_PROPERTY_ASSERT( queue.Pop() == expected );
                    } } );
            },
            []( const Model& model ) { return !model.empty(); } };
        return { push, pop };
    }

} // namespace

TEST_CASE( "Stateful tests find and shrink a failing command sequence",
           "[progmasoft][stateful]" ) {
    PropertyOptions options;
    options.Trials = 300;
    options.Seed = 0x57A7E;
    options.MaxShrinkSteps = 5000;
    const auto result = Progmasoft::Catch3::CheckStateful<Model, SmallQueue>(
        options,
        [] { return Model{}; },
        []( const Model& ) { return SmallQueue{}; },
        QueueCommands() );
    REQUIRE( result.Status == PropertyStatus::kFailed );
    // The shortest failure: the fourth push drops an element.
    CHECK( result.Failure->Counterexample ==
           "{ push(0), push(0), push(0), push(0) }" );
    CHECK( result.Failure->Reason.find(
               "CATCH3_PROPERTY_ASSERT( queue.Size() == model.size() )" ) ==
           0 );
}

TEST_CASE( "Stateful tests pass for a correct system and respect preconditions",
           "[progmasoft][stateful]" ) {
    struct Correct {
        std::deque<int> Values;
    };
    Command<Model, Correct> push{
        "push", []( const Model& ) {
            return Gen::Integers<int>( 0, 9 ).Map( []( int value ) {
                return Action<Model, Correct>{
                    "push", [value]( Model& model, Correct& system ) {
                        model.push_back( value );
                        system.Values.push_back( value );
                    } };
            } );
        } };
    Command<Model, Correct> pop{
        "pop",
        []( const Model& ) {
            return Gen::Just( Action<Model, Correct>{
                "pop", []( Model& model, Correct& system ) {
                    // The precondition guarantees an element.
                    CATCH3_PROPERTY_ASSERT( !model.empty() );
                    CATCH3_PROPERTY_ASSERT( system.Values.front() ==
                                            model.front() );
                    model.pop_front();
                    system.Values.pop_front();
                } } );
        },
        []( const Model& model ) { return !model.empty(); } };
    PropertyOptions options;
    options.Trials = 200;
    const auto result = Progmasoft::Catch3::CheckStateful<Model, Correct>(
        options,
        [] { return Model{}; },
        []( const Model& ) { return Correct{}; },
        { push, pop },
        SizeRange{ 0, 50 } );
    CHECK( result.Status == PropertyStatus::kPassed );
}
