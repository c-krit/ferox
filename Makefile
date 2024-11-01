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

.PHONY: all clean install rebuild uninstall

# ============================================================================>

_COLOR_BEGIN = \033[1;38;5;045m
_COLOR_END = \033[m

# ============================================================================>

PROJECT_NAME = ferox

LOG_PREFIX = ${_COLOR_BEGIN} ~>${_COLOR_END}

# ============================================================================>

INCLUDE_PATH = include
LIBRARY_PATH = lib
SOURCE_PATH = src

OBJECTS = \
	${SOURCE_PATH}/broad_phase.o  \
	${SOURCE_PATH}/collision.o    \
	${SOURCE_PATH}/geometry.o     \
	${SOURCE_PATH}/rigid_body.o   \
	${SOURCE_PATH}/timer.o        \
	${SOURCE_PATH}/utils.o        \
	${SOURCE_PATH}/world.o

TARGETS = ${LIBRARY_PATH}/lib${PROJECT_NAME}.a

PREFIX = ${DESTDIR}/usr/local

# ============================================================================>

CC = cc
AR = ar
CFLAGS = -D_DEFAULT_SOURCE -g -I${INCLUDE_PATH} -O2 -std=gnu99

CFLAGS += -Wall -Wpedantic -Wno-unused-but-set-variable -Wno-unused-value \
	-Wno-unused-variable

# ============================================================================>

all: pre-build build post-build

pre-build:
	@printf "${LOG_PREFIX} CC = ${CC}, AR = ${AR}, MAKE = ${MAKE}\n"

build: ${TARGETS}

.c.o:
	@printf "${LOG_PREFIX} Compiling: $@ (from $<)\n"
	@${CC} -c $< -o $@ ${CFLAGS}

${TARGETS}: ${OBJECTS}
	@mkdir -p ${LIBRARY_PATH}
	@printf "${LOG_PREFIX} Linking: ${TARGETS}\n"
	@${AR} rcs ${TARGETS} ${OBJECTS}

post-build:
	@printf "${LOG_PREFIX} Build complete.\n"

# ============================================================================>

rebuild: clean all

# ============================================================================>

superuser:
	@if [ "$(shell id -u)" -ne 0 ]; then                   \
		printf "${LOG_PREFIX} You must be superuser to ";  \
		printf "install or uninstall this library.\n";     \
														   \
		exit 1;                                            \
	fi

install: superuser
	@printf "${LOG_PREFIX} Installing: "
	@printf "${PREFIX}/${INCLUDE_PATH}/${PROJECT_NAME}.h\n"
	@install -m755 ${INCLUDE_PATH}/${PROJECT_NAME}.h \
		${PREFIX}/${INCLUDE_PATH}
	@printf "${LOG_PREFIX} Installing: ${PREFIX}/${TARGETS}\n"
	@install -m755 ${TARGETS} ${PREFIX}/${TARGETS}

uninstall: superuser
	@printf "${LOG_PREFIX} Uninstalling: "
	@printf "${PREFIX}/${INCLUDE_PATH}/${PROJECT_NAME}.h\n"
	@rm -f ${PREFIX}/${INCLUDE_PATH}/${PROJECT_NAME}.h
	@printf "${LOG_PREFIX} Uninstalling: ${PREFIX}/${TARGETS}\n"
	@rm -f ${PREFIX}/${TARGETS}

# ============================================================================>

clean:
	@printf "${LOG_PREFIX} Cleaning up.\n"
	@rm -f ${LIBRARY_PATH}/*.a
	@rm -f ${SOURCE_PATH}/*.o

# ============================================================================>
