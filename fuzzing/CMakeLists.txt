# License: Boost 1.0
# By Paul Dreik 2020

# add a library that brings in the main() function from libfuzzer
# and has all the dependencies, so the individual fuzzers can be
# added one line each.
add_library(fuzzhelper NullOStream.h NullOStream.cpp)
target_link_libraries(fuzzhelper PUBLIC Catch3::Catch3)

# The runner fuzzers include its headers, which need C++17.
target_compile_features(fuzzhelper PUBLIC cxx_std_17)

# This should be possible to set from the outside to be oss-fuzz compatible,
# fix later. For now, target libFuzzer only.
target_link_options(fuzzhelper PUBLIC "-fsanitize=fuzzer")

foreach(fuzzer TestSpecParser)
  add_executable(fuzz_${fuzzer} fuzz_${fuzzer}.cpp)
  target_link_libraries(fuzz_${fuzzer} PRIVATE fuzzhelper)
endforeach()

# Progmasoft's fuzzers live apart, under their own license header.
foreach(fuzzer ProjectConfig RunnerJson RunnerOptions)
  add_executable(fuzz_${fuzzer} Progmasoft/fuzz_${fuzzer}.cpp)
  target_link_libraries(fuzz_${fuzzer} PRIVATE fuzzhelper)
endforeach()
