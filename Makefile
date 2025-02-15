CFLAGS=-O3 -Wall -Wextra -lX11 -lpci
PREFIX=$(HOME)/.local
CACHE=$(shell if [ "$$XDG_CACHE_HOME" ]; then echo "$$XDG_CACHE_HOME"; else echo "$$HOME"/.cache; fi)

.PHONY: clean install uninstall
all: paleofetch

clean:
	rm -f paleofetch $(CACHE)/paleofetch

paleofetch: paleofetch.c paleofetch.h config.h functions.c helper.c
	$(eval battery_path := $(shell ./config_scripts/battery_config.sh))
	$(CC) paleofetch.c -o paleofetch $(CFLAGS) -D $(battery_path)
	strip paleofetch
	./paleofetch -r

debug:
	$(eval battery_path := $(shell ./config_scripts/battery_config.sh))
	$(CC) paleofetch.c -o paleofetch $(CFLAGS) -D $(battery_path) -g

install: paleofetch
	mkdir -p $(PREFIX)/bin
	install ./paleofetch $(PREFIX)/bin/paleofetch

uninstall:
	rm $(PREFIX)/bin/paleofetch
