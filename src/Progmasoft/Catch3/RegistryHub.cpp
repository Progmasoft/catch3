
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0
#include <Progmasoft/Catch3/Interfaces/IRegistryHub.hpp>

#include <Progmasoft/Catch3/Internal/Context.hpp>
#include <Progmasoft/Catch3/Internal/Enforce.hpp>
#include <Progmasoft/Catch3/Internal/TestCaseRegistryImpl.hpp>
#include <Progmasoft/Catch3/Internal/ReporterRegistry.hpp>
#include <Progmasoft/Catch3/Internal/ExceptionTranslatorRegistry.hpp>
#include <Progmasoft/Catch3/Internal/TagAliasRegistry.hpp>
#include <Progmasoft/Catch3/Internal/StartupExceptionRegistry.hpp>
#include <Progmasoft/Catch3/Internal/Singletons.hpp>
#include <Progmasoft/Catch3/TestCaseInfo.hpp>
#include <Progmasoft/Catch3/Internal/NonCopyable.hpp>
#include <Progmasoft/Catch3/Interfaces/EventListenerFactory.hpp>
#include <Progmasoft/Catch3/Internal/MoveAndForward.hpp>

#include <exception>

namespace Catch {

    namespace {

        class RegistryHub : public IRegistryHub,
                            public IMutableRegistryHub,
                            private Detail::NonCopyable {

        public: // IRegistryHub
            RegistryHub() = default;
            ReporterRegistry const& getReporterRegistry() const override {
                return m_reporterRegistry;
            }
            ITestCaseRegistry const& getTestCaseRegistry() const override {
                return m_testCaseRegistry;
            }
            IExceptionTranslatorRegistry const& getExceptionTranslatorRegistry() const override {
                return m_exceptionTranslatorRegistry;
            }
            ITagAliasRegistry const& getTagAliasRegistry() const override {
                return m_tagAliasRegistry;
            }
            StartupExceptionRegistry const& getStartupExceptionRegistry() const override {
                return m_exceptionRegistry;
            }

        public: // IMutableRegistryHub
            void registerListener( Detail::unique_ptr<EventListenerFactory> factory ) override {
                m_reporterRegistry.registerListener( CATCH_MOVE(factory) );
            }
            void registerTest( Detail::unique_ptr<TestCaseInfo>&& testInfo, Detail::unique_ptr<ITestInvoker>&& invoker ) override {
                m_testCaseRegistry.registerTest( CATCH_MOVE(testInfo), CATCH_MOVE(invoker) );
            }
            void registerTranslator( Detail::unique_ptr<IExceptionTranslator>&& translator ) override {
                m_exceptionTranslatorRegistry.registerTranslator( CATCH_MOVE(translator) );
            }
            void registerTagAlias( std::string const& alias, std::string const& tag, SourceLineInfo const& lineInfo ) override {
                m_tagAliasRegistry.add( alias, tag, lineInfo );
            }
            void registerStartupException() noexcept override {
#if !defined(CATCH_CONFIG_DISABLE_EXCEPTIONS)
                m_exceptionRegistry.add(std::current_exception());
#else
                CATCH_INTERNAL_ERROR("Attempted to register active exception under CATCH_CONFIG_DISABLE_EXCEPTIONS!");
#endif
            }

            ITestCaseRegistry& getMutableTestCaseRegistry() override {
                return m_testCaseRegistry;
            }

        private:
            TestRegistry m_testCaseRegistry;
            ReporterRegistry m_reporterRegistry;
            ExceptionTranslatorRegistry m_exceptionTranslatorRegistry;
            TagAliasRegistry m_tagAliasRegistry;
            StartupExceptionRegistry m_exceptionRegistry;
        };
    }

    using RegistryHubSingleton = Singleton<RegistryHub, IRegistryHub, IMutableRegistryHub>;

    IRegistryHub const& getRegistryHub() {
        return RegistryHubSingleton::get();
    }
    IMutableRegistryHub& getMutableRegistryHub() {
        return RegistryHubSingleton::getMutable();
    }
    void cleanUp() {
        cleanupSingletons();
    }
    std::string translateActiveException() {
        return getRegistryHub().getExceptionTranslatorRegistry().translateActiveException();
    }


} // end namespace Catch
