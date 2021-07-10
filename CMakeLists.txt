#
# Copyright (c) 2021 jdeokkim
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

# CMake의 최소 버전을 설정한다.
cmake_minimum_required(VERSION 3.10)

# 프로젝트의 이름, 버전과 설명을 정의한다.
project(ferox 
    VERSION 0.0.1 
    DESCRIPTION "A simple 2D physics engine written in C."
)

# 추가적인 CMake 기능을 불러온다.
include(GNUInstallDirs)

# C 표준을 설정한다.
set(CMAKE_C_STANDARD 99)

# 'raylib' 패키지를 찾는다.
find_package(raylib 3.5.0)

# 프로젝트 빌드에 필요한 디렉토리를 추가한다.
add_subdirectory("${CMAKE_SOURCE_DIR}/ferox")
add_subdirectory("${CMAKE_SOURCE_DIR}/examples")
