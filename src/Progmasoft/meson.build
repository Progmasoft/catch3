# Copyright (c) 2026 Progmasoft.
# SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1

# Compile the non-template C++20 utilities separately while retaining Catch2's
# own C++14 baseline for existing users.
progmasoft_catch3 = static_library(
  'ProgmasoftCatch3',
  ['Results.cpp', 'Snapshot.cpp', 'XmlWriter.cpp'],
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
  install_headers('Catch3/Generator.hpp', subdir: 'Progmasoft/Catch3')
  install_headers('Catch3/Property.hpp', subdir: 'Progmasoft/Catch3')
  install_headers('Catch3/Random.hpp', subdir: 'Progmasoft/Catch3')
  install_headers('Catch3/Results.hpp', subdir: 'Progmasoft/Catch3')
  install_headers('Catch3/Snapshot.hpp', subdir: 'Progmasoft/Catch3')
  install_headers('Catch3/XmlWriter.hpp', subdir: 'Progmasoft/Catch3')
  install_headers('Catch3/Generators/Boolean.hpp', subdir: 'Progmasoft/Catch3/Generators')
  install_headers('Catch3/Generators/Integer.hpp', subdir: 'Progmasoft/Catch3/Generators')
endif
