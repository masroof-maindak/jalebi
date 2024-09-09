CC = gcc

CFLAGS = -Wall -Wextra -pedantic
RELEASE_FLAGS = -march=native -O3
DEBUG_FLAGS = -g -O0
# LDFLAGS = 

SRCDIR = src
BUILDDIR = build
TARGETS = jalebi namak-pare

PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

SRCS = $(wildcard ${SRCDIR}/*.c)
OBJS = ${SRCS:${SRCDIR}/%.c=${BUILDDIR}/%.o}