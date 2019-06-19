src = $(wildcard *.c)
obj = $(src:.c=.o)

# If you don't have id3v2lib, delete the last two entries of this line
CFLAGS = -g -lSDL2 -lSDL2_image -D_has_id3v2lib -lid3v2
OUTNAME = cmusCoverViewer

all: config.h $(OUTNAME)

config.h:
	cp config.def.h config.h

$(OUTNAME): $(obj)
	$(CC) -o $(OUTNAME) $^ $(CFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) $(OUTNAME)

install: $(OUTNAME)
	cp -f $(OUTNAME) "/usr/local/bin"
	chmod 755 "/usr/local/bin/$(OUTNAME)"
