CFLAGS += $(shell pkg-config --cflags libnl-genl-3.0)
LDFLAGS += $(shell pkg-config --libs libnl-genl-3.0)

list-80211: list-80211.o
	$(CC) $< $(LDFLAGS) -o $@

list-80211.o: list-80211.c
	$(CC) -c $< $(CFLAGS) -o $@

clean:
	rm -f list-80211.o list-80211

.PHONY: clean
