
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0

// 232-Cfg-CustomMain.cpp
// Show how to use custom main and read an option of your own before the
// rest of the command line goes to the Catch3 runner.
//
// The runner starts this binary again for its worker processes, with the
// same `main`: what happens before `run` happens in every worker too, which
// is where global set-up belongs.

#include <Progmasoft/Catch3/Session.hpp>

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv) {
  Catch::Session session; // There must be exactly one instance

  int height = 0; // Some user variable you want to be able to set

  // Take our own option out of the command line; the runner reads the rest.
  // Workers are started with the runner's options only, so a value they
  // need too belongs in the environment, which they inherit.
  std::vector<char const*> arguments;
  for ( int i = 0; i < argc; ++i ) {
      std::string const argument = argv[i];
      if ( argument.rfind( "--height=", 0 ) == 0 ) {
          height = std::atoi( argument.c_str() + 9 );
      } else {
          arguments.push_back( argv[i] );
      }
  }

  std::cout << "height: " << height << '\n';

  return session.run( static_cast<int>( arguments.size() ), arguments.data() );
}
