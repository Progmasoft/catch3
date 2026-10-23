# Copyright (c) 2026 Progmasoft.
# SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1

# The property engine and the other Progmasoft test utilities; their headers
# need C++20 in the code that includes them.
progmasoft_catch3 = static_library(
  'ProgmasoftCatch3',
  [
    'ChoiceShrinker.cpp',
    'Context.cpp',
    'PropertyEngine.cpp',
    'Snapshot.cpp',
  ],
  include_directories: '..',
  dependencies: catch2_dep,
  override_options: ['cpp_std=c++20'],
  install: get_option('install'),
)
progmasoft_catch3_dep = declare_dependency(
  link_with: progmasoft_catch3,
  dependencies: catch2_dep,
  include_directories: include_directories('..'),
)

if get_option('install')
  install_headers('Catch3.hpp', subdir: 'Progmasoft')
  install_headers('Catch3/Assertions.hpp', subdir: 'Progmasoft/Catch3')
  install_headers('Catch3/Detail/InvalidArgument.hpp', subdir: 'Progmasoft/Catch3/Detail')
  install_headers('Catch3/Detail/Sequence.hpp', subdir: 'Progmasoft/Catch3/Detail')
  install_headers('Catch3/Generator.hpp', subdir: 'Progmasoft/Catch3')
  install_headers('Catch3/Property.hpp', subdir: 'Progmasoft/Catch3')
  install_headers('Catch3/Random.hpp', subdir: 'Progmasoft/Catch3')
  install_headers('Catch3/Results.hpp', subdir: 'Progmasoft/Catch3')
  install_headers('Catch3/Runner.hpp', subdir: 'Progmasoft/Catch3')
  install_headers('Catch3/Version.hpp', subdir: 'Progmasoft/Catch3')
  install_headers('Catch3/Snapshot.hpp', subdir: 'Progmasoft/Catch3')
  install_headers('Catch3/XmlWriter.hpp', subdir: 'Progmasoft/Catch3')
  install_headers('Catch3/ForAll.hpp', subdir: 'Progmasoft/Catch3')
  install_headers('Catch3/Relations.hpp', subdir: 'Progmasoft/Catch3')
  install_headers('Catch3/Stateful.hpp', subdir: 'Progmasoft/Catch3')
  install_headers('Catch3/Strategy/ChoiceSource.hpp', subdir: 'Progmasoft/Catch3/Strategy')
  install_headers('Catch3/Strategy/Containers.hpp', subdir: 'Progmasoft/Catch3/Strategy')
  install_headers('Catch3/Strategy/Context.hpp', subdir: 'Progmasoft/Catch3/Strategy')
  install_headers('Catch3/Strategy/Engine.hpp', subdir: 'Progmasoft/Catch3/Strategy')
  install_headers('Catch3/Strategy/Numeric.hpp', subdir: 'Progmasoft/Catch3/Strategy')
  install_headers('Catch3/Strategy/Strategy.hpp', subdir: 'Progmasoft/Catch3/Strategy')
  install_headers('Catch3/Generators/Boolean.hpp', subdir: 'Progmasoft/Catch3/Generators')
  install_headers('Catch3/Generators/Integer.hpp', subdir: 'Progmasoft/Catch3/Generators')
  install_headers('Catch3/Generators/Element.hpp', subdir: 'Progmasoft/Catch3/Generators')
  install_headers('Catch3/Generators/Text.hpp', subdir: 'Progmasoft/Catch3/Generators')
  install_headers('Catch3/Generators/Tuple.hpp', subdir: 'Progmasoft/Catch3/Generators')
  install_headers('Catch3/Generators/Vector.hpp', subdir: 'Progmasoft/Catch3/Generators')
endif
