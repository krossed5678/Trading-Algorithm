# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Users/koanr/Desktop/C++/Trading-Algorithm/out/build/x64-Debug/_deps/imgui-src"
  "C:/Users/koanr/Desktop/C++/Trading-Algorithm/out/build/x64-Debug/_deps/imgui-build"
  "C:/Users/koanr/Desktop/C++/Trading-Algorithm/out/build/x64-Debug/_deps/imgui-subbuild/imgui-populate-prefix"
  "C:/Users/koanr/Desktop/C++/Trading-Algorithm/out/build/x64-Debug/_deps/imgui-subbuild/imgui-populate-prefix/tmp"
  "C:/Users/koanr/Desktop/C++/Trading-Algorithm/out/build/x64-Debug/_deps/imgui-subbuild/imgui-populate-prefix/src/imgui-populate-stamp"
  "C:/Users/koanr/Desktop/C++/Trading-Algorithm/out/build/x64-Debug/_deps/imgui-subbuild/imgui-populate-prefix/src"
  "C:/Users/koanr/Desktop/C++/Trading-Algorithm/out/build/x64-Debug/_deps/imgui-subbuild/imgui-populate-prefix/src/imgui-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/koanr/Desktop/C++/Trading-Algorithm/out/build/x64-Debug/_deps/imgui-subbuild/imgui-populate-prefix/src/imgui-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/koanr/Desktop/C++/Trading-Algorithm/out/build/x64-Debug/_deps/imgui-subbuild/imgui-populate-prefix/src/imgui-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
