#
# Copyright (c) 2021-2023 Jaedeok Kim <jdeokkim@protonmail.com>
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

.PHONY: all clean
.SUFFIXES: .c .exe .html .out

_COLOR_BEGIN = \033[1;36m
_COLOR_END = \033[m

PROJECT_NAME = ferox
PROJECT_FULL_NAME = c-krit/ferox

PROJECT_PREFIX = ${_COLOR_BEGIN}${PROJECT_FULL_NAME}:${_COLOR_END}

INCLUDE_PATH = include
LIBRARY_PATH = ../lib
SOURCE_PATH = src

TARGETS ?= \
	${SOURCE_PATH}/basic.out   \
	${SOURCE_PATH}/bricks.out  \
	${SOURCE_PATH}/raycast.out \
	${SOURCE_PATH}/raylib.out

CC ?= gcc
CFLAGS = -D_DEFAULT_SOURCE -g -I${INCLUDE_PATH} -I../${INCLUDE_PATH} \
	-I${RAYLIB_INCLUDE_PATH} -O2 -std=gnu99
LDFLAGS = -L${LIBRARY_PATH} -L${RAYLIB_LIBRARY_PATH}
LDLIBS = -lferox -lraylib -ldl -lGL -lm -lpthread -lrt -lX11

all: pre-build build post-build

pre-build:
	@printf "${PROJECT_PREFIX} Using: '${CC}' to build all examples.\n"

build: ${TARGETS}

.c.exe .c.out:
	@printf "${PROJECT_PREFIX} Compiling: $@ (from $<)\n"
	@${CC} $< -o $@ ${CFLAGS} ${LDFLAGS} ${LDLIBS}

.c.html:
	@printf "${PROJECT_PREFIX} Compiling: $@ (from $<)\n"
	@${CC} $< -o $@ ${CFLAGS} ${LDFLAGS} ${LDLIBS} ${WEBFLAGS}

post-build:
	@printf "${PROJECT_PREFIX} Build complete.\n"

clean:
	@printf "${PROJECT_PREFIX} Cleaning up.\n"
	@rm -f ${SOURCE_PATH}/*.data ${SOURCE_PATH}/*.exe ${SOURCE_PATH}/*.js \
	${SOURCE_PATH}/*.html ${SOURCE_PATH}/*.out ${SOURCE_PATH}/*.wasm
