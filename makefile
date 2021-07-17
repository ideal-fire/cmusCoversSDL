src = $(wildcard *.c)
obj = $(src:.c=.o)

# If you don't want to build with ffmpeg's libavformat, remove everything on the line after (and including) "-D_has_libavformat"
CFLAGS = -g `pkg-config --cflags --libs sdl2 SDL2_image` -D_has_libavformat `pkg-config --libs --cflags libavformat`
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
	cp -f cmusv "/usr/local/bin"

uninstall: $(OUTNAME)
	rm -f "/usr/local/bin/$(OUTNAME)"
	rm -f "/usr/local/bin/cmusv"

main.o: config.h
