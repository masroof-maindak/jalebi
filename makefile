include config.mk

all: CFLAGS += $(RELEASE_FLAGS)
all: $(TARGETS)

debug: CFLAGS += $(DEBUG_FLAGS)
debug: clean $(TARGETS)

jalebi: $(BUILDDIR)/server.o
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

namak-pare: $(BUILDDIR)/client.o
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

install: all
	install -d $(BINDIR)
	install -m 755 jalebi $(BINDIR)
	install -m 755 namak-pare $(BINDIR)
	
uninstall:
	rm -f $(BINDIR)/jalebi
	rm -f $(BINDIR)/namak-pare

clean:
	rm -f jalebi namak-pare $(BUILDDIR)/*.o

.PHONY: all clean install uninstall
