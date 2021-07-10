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

# 소스 파일 경로를 변수에 저장한다.
set(EXAMPLE_SOURCE_FILES
    "src/example_collision.c"
    "src/example_dynamics.c"
    "src/example_geometry.c"
)

if(raylib_FOUND)
    foreach(example_source_file ${EXAMPLE_SOURCE_FILES})
        # 파일 경로에서 파일 이름을 추출한다.
        get_filename_component(example_name ${example_source_file} NAME_WE)

        # 빌드 결과물로 생성할 실행 파일을 추가한다.
        add_executable(${example_name} ${example_source_file})
        
        # 헤더 파일의 경로를 설정한다.
        # target_include_directories(${example_name} PUBLIC "${CMAKE_SOURCE_DIR}/examples/src/external")

        # 외부 라이브러리를 링크한다.
        target_link_libraries(${example_name} PRIVATE ferox raylib dl GL m pthread rt X11)
    endforeach()
endif()