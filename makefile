src = $(wildcard *.c)
obj = $(src:.c=.o)

# If you don't want to build with ffmpeg's libavformat, remove the last two lines.
CFLAGS = -g -lSDL2 -lSDL2_image -D_has_libavformat -lavformat
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

main.o: config.h