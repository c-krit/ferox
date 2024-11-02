#
# Copyright (c) 2021-2024 Jaedeok Kim <jdeokkim@protonmail.com>
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

# ============================================================================>

.POSIX:

# ============================================================================>

.PHONY: all clean rebuild

# ============================================================================>

_COLOR_BEGIN = \033[1;38;5;045m
_COLOR_END = \033[m

# ============================================================================>

PROJECT_NAME = ferox

LOG_PREFIX = ${_COLOR_BEGIN} ~>${_COLOR_END}

# ============================================================================>

BINARY_PATH = bin
INCLUDE_PATH = include
LIBRARY_PATH = ../lib
SOURCE_PATH = src

OBJECTS = \
	${SOURCE_PATH}/broad_phase.o  \
	${SOURCE_PATH}/collision.o    \
	${SOURCE_PATH}/ferox_tests.o  \
	${SOURCE_PATH}/geometry.o     \
	${SOURCE_PATH}/rigid_body.o   \
	${SOURCE_PATH}/utils.o        \
	${SOURCE_PATH}/world.o

TARGET_SUFFIX = out

TARGETS = ${BINARY_PATH}/ferox_tests.${TARGET_SUFFIX}

# ============================================================================>

CC = cc
CFLAGS = -D_DEFAULT_SOURCE -g -I${INCLUDE_PATH} -I../${INCLUDE_PATH} \
	-I../${SOURCE_PATH}/external -O2 -std=gnu99
LDFLAGS = -L${LIBRARY_PATH}
LDLIBS = -lferox -lm

# ============================================================================>

all: pre-build build post-build

pre-build:
	@printf "${LOG_PREFIX} CC = ${CC}, MAKE = ${MAKE}\n"

build: ${TARGETS}

.c.o:
	@printf "${LOG_PREFIX} Compiling: $@ (from $<)\n"
	@${CC} -c $< -o $@ ${CFLAGS} ${LDFLAGS} ${LDLIBS}

${TARGETS}: ${OBJECTS}
	@mkdir -p ${BINARY_PATH}
	@printf "${LOG_PREFIX} Linking: ${TARGETS}\n"
	@${CC} ${OBJECTS} -o ${TARGETS} ${LDFLAGS} ${LDLIBS}

post-build:
	@printf "${LOG_PREFIX} Build complete.\n"

# ============================================================================>

rebuild: clean all

# ============================================================================>

clean:
	@printf "${LOG_PREFIX} Cleaning up.\n"
	@rm -f ${BINARY_PATH}/*.${TARGET_SUFFIX} ${SOURCE_PATH}/*.o

# ============================================================================>
