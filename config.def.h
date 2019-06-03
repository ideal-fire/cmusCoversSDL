#define WINDOWTITLE "cmus art"
// Minimum amount of time to pass before the program checks for song change
#define RECHECKMILLI 1000
//
#define PIXELPADDING 5
// 1 if you will allow a cover to a song to be the same filename as the song, just with an image extension. For example, bla.jpg as a cover for bla.wav.
#define SAMEASSONGNAMECOVER 1
// Possible filenames for cover files
// Should all end in a dot and be lowercase
#define NUMCOVERFILENAMES 2
char* const coverFilenames[NUMCOVERFILENAMES] = {
	"cover.",
	"folder.",
};
// Extensions that covers can have
// Should be written lowercase and without a dot
#define NUMEXTENSIONS 2
char* const loadableExtensions[NUMEXTENSIONS] = {
	"jpg",
	"png",
};
// Program arguments to get info from cmus-remote
// You may need to change the path to /usr/bin/local if you installed cmus yourself
char* const _programArgs[] = {
	"/usr/bin/cmus-remote",
	"-Q",
	NULL,
};
// If the cover isn't found in the audio file's folder, try going up a directory. Can repeat MAXDIRUP times
#define MAXDIRUP 1
#define DIRSEPARATORCHAR '/'