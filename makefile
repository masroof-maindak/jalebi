include config.mk

# Make a debug build of the server
default: CFLAGS += $(DEBUG_FLAGS)
default: jalebi

all: CFLAGS += $(RELEASE_FLAGS)
all: $(TARGETS)

debug: CFLAGS += $(DEBUG_FLAGS)
debug: clean $(TARGETS)

jalebi: $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(SERVER_LDFLAGS)

namak-paare: $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(CLIENT_LDFLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

install: all
	install -d $(BINDIR)
	install -m 755 jalebi $(BINDIR)
	install -m 755 namak-paare $(BINDIR)
	
uninstall:
	rm -f $(BINDIR)/jalebi
	rm -f $(BINDIR)/namak-paare

clean:
	rm -f $(TARGETS) $(BUILDDIR)/*.o sqlite.db

.PHONY: all clean install uninstall
