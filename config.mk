CC = gcc

CFLAGS = -Wall -Wextra -pedantic
RELEASE_FLAGS = -march=native -O3
DEBUG_FLAGS = -g3 -O0
CLIENT_LDFLAGS = -lreadline -lbsd
SERVER_LDFLAGS = -lsqlite3 -lcrypto -lpthread -luuid

SRCDIR = src
BUILDDIR = build
TARGETS = jalebi namak-paare

PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

COMMON_FILES = utils
CLIENT_FILES = $(COMMON_FILES) client
SERVER_FILES = $(COMMON_FILES) server auth queue threadpool prodcons hashmap task

CLIENT_OBJS = $(addprefix $(BUILDDIR)/,$(CLIENT_FILES:=.o))
SERVER_OBJS = $(addprefix $(BUILDDIR)/,$(SERVER_FILES:=.o))

