#              Copyright Catch2 Authors
# Distributed under the Boost Software License, Version 1.0.
#   (See accompanying file LICENSE.txt or copy at
#        https://www.boost.org/LICENSE_1_0.txt)

# SPDX-License-Identifier: BSL-1.0

project(
  'catch2',
  'cpp',
  version: '3.16.0', # CML version placeholder, don't delete
  license: 'BSL-1.0',
  # Meson 1.1 added clang-cl support for the C++20 mode used by Progmasoft tests.
  meson_version: '>=1.1.0',
)

subdir('src/catch2')
subdir('src/Progmasoft')
if get_option('tests')
  subdir('tests')
endif
