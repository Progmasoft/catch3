# Copyright (c) 2026 Progmasoft.
# SPDX-License-Identifier: MPL-2.0 WITH AdditionRef-Progmasoft-Exception-1.1

# Expose the header-only C++20 layer without raising Catch2's C++14 baseline.
progmasoft_catch3_dep = declare_dependency(
  dependencies: catch2_dep,
  include_directories: include_directories('..'),
)

if get_option('install')
  install_headers('Catch3.hpp', subdir: 'Progmasoft')
  install_headers('Catch3/Assertions.hpp', subdir: 'Progmasoft/Catch3')
  install_headers('Catch3/Generator.hpp', subdir: 'Progmasoft/Catch3')
  install_headers('Catch3/Property.hpp', subdir: 'Progmasoft/Catch3')
  install_headers('Catch3/Random.hpp', subdir: 'Progmasoft/Catch3')
  install_headers('Catch3/Generators/Boolean.hpp', subdir: 'Progmasoft/Catch3/Generators')
  install_headers('Catch3/Generators/Integer.hpp', subdir: 'Progmasoft/Catch3/Generators')
endif
