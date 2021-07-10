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

# 헤더 파일 경로를 변수에 저장한다.
set(INCLUDE_FILES "include/ferox.h")

# 소스 파일 경로를 변수에 저장한다.
set(SOURCE_FILES
    "src/collision.c"
    "src/debug.c"
    "src/dynamics.c"
    "src/geometry.c"
    "src/quadtree.c"
    "src/timer.c"
    "src/utils.c"
    "src/vector.c"
    "src/world.c"
)

# 프로젝트에 라이브러리 'ferox'를 추가한다.
add_library(ferox STATIC ${INCLUDE_FILES} ${SOURCE_FILES})

# 빌드할 때 사용할 속성을 설정한다.
set_target_properties(ferox PROPERTIES PUBLIC_HEADER ${INCLUDE_FILES})

# 'raylib' 패키지가 존재할 경우, 컴파일 매크로를 정의한다.
if(raylib_FOUND)
    target_compile_definitions(ferox PUBLIC HAVE_RAYLIB)
endif()

# 헤더 파일의 경로를 설정한다.
target_include_directories(ferox PUBLIC "${CMAKE_SOURCE_DIR}/ferox/include")
target_include_directories(ferox PUBLIC "${CMAKE_SOURCE_DIR}/ferox/src/external")

# 외부 라이브러리를 링크한다.
target_link_libraries(ferox PUBLIC raylib dl GL m pthread rt X11)

# 라이브러리가 설치될 경로를 정의한다.
install(
    TARGETS ferox
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)