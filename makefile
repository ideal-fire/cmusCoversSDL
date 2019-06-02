src = $(wildcard *.c)
obj = $(src:.c=.o)

LDFLAGS = -lSDL2 -lSDL2_image
CFLAGS = -g
OUTNAME = cmusCoverViewer

all: config.h $(OUTNAME)

config.h:
	cp config.def.h config.h

$(OUTNAME): $(obj)
	$(CC) -o $(OUTNAME) $^ $(CFLAGS) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) $(OUTNAME)

install: $(OUTNAME)
	cp -f $(OUTNAME) "/usr/local/bin"
	chmod 755 "/usr/local/bin/$(OUTNAME)"
