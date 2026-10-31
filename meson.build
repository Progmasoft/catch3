#              Copyright Catch2 Authors
# Distributed under the Boost Software License, Version 1.0.
#   (See accompanying file LICENSE.txt or copy at
#        https://www.boost.org/LICENSE_1_0.txt)

# SPDX-License-Identifier: BSL-1.0

project(
  'catch3',
  'cpp',
  version: '0.3.0',
  license: ['BSL-1.0', 'MPL-2.0 WITH AdditionRef-Progmasoft-Exception-2.0'],
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
