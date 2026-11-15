#              Copyright Catch2 Authors
# Distributed under the Boost Software License, Version 1.0.
#   (See accompanying file LICENSE.txt or copy at
#        https://www.boost.org/LICENSE_1_0.txt)

# SPDX-License-Identifier: BSL-1.0

project(
  'catch3',
  'cpp',
  version: '0.4.0',
  license: ['BSL-1.0', 'LGPL-3.0-or-later'],
  # Meson 1.1 added clang-cl support for the C++ modes used by Progmasoft tests.
  meson_version: '>=1.1.0',
)

# yyjson and pugixml, compiled into the library; nothing of them is installed.
subdir('third_party/yyjson')
subdir('third_party/pugixml')
subdir('src/Progmasoft/Catch3')
subdir('src/Progmasoft')
if get_option('tests')
  subdir('tests')
endif
