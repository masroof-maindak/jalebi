CC = gcc

CFLAGS = -Wall -Wextra -pedantic
RELEASE_FLAGS = -march=native -O3
DEBUG_FLAGS = -g3 -O0
# LDFLAGS = 

SRCDIR = src
BUILDDIR = build
TARGETS = jalebi namak-paare

PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

SRCS = $(wildcard ${SRCDIR}/*.c)
OBJS = ${SRCS:${SRCDIR}/%.c=${BUILDDIR}/%.o}
SERVER_OBJS = $(filter-out $(BUILDDIR)/client.o,$(OBJS))
CLIENT_OBJS = $(filter-out $(BUILDDIR)/server.o,$(OBJS))
