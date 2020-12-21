#include "main.h" // do not remove
#define WINDOWTITLE "Cover"
// Minimum amount of time to pass before the program checks for song change
// Set this to -1 to disable automatic refresh, but then the program won't be able to detect when cmus closes.
#define RECHECKMILLI 1000
//
#define PIXELPADDING 5
// Program arguments to get info from cmus-remote
// You can also put the full path to cmus-remote if you want
char* const programArgs[] = {
	"cmus-remote",
	"-Q",
	NULL,
};
/*
Changes how the application waits for its next redraw
0 - Wait using SDL_WaitEventTimeout. The application will redraw when the window receives events or when it checks for cmus updates. Recommended mode
1 - Wait using nanosleep. The application will redraw and process events every ALTWAITMODEREDRAWTIME nanoseconds or when it checks for cmus updates. 
*/
#define WAITMODE 0
#if WAITMODE == 0
	// Amount of time between checks for events sent to the window. 
	#define EVENTPOLLTIME 10
#elif WAITMODE == 1
	// In nanoseconds. The amount of time to wait before a redraw if WAITMODE is 1.
	// Better have this less than RECHECKMILLI if you want it to be useful
	#define ALTWAITMODEREDRAWTIME 500L*1000000L
#endif
// Functions that handle getting cover images. Put them in order of preference.
// If you know you don't have any standalone cover images, you could even remove getCoverByStandaloneImage if you want.
#define NUMCOVERGETTERS 2
getCoverFunction coverGetters[NUMCOVERGETTERS] = {
	getEmbeddedCover,
	getCoverByStandaloneImage,
};
// Options for getCoverByStandaloneImage:
//
// Possible filenames for cover files
// Should all end in a dot and be lowercase
// Ones that end in asterisk only match the start, otherwise they should end in a dot
#define NUMCOVERFILENAMES 2
char* const coverFilenames[NUMCOVERFILENAMES] = {
	"cover*",
	"folder*",
};
// Extensions that covers can have
// Should be written lowercase and without a dot
#define NUMEXTENSIONS 2
char* const loadableExtensions[NUMEXTENSIONS] = {
	"jpg",
	"png",
};
// If this is enabled, the program won't stop right after finding a valid cover filename. It'll look through the rest of the files in that folder first. If it finds a file with a filename that higher (lower index) in the coverFilenames array, it'll use that as the cover instead.
// So you can put your preferred filenames in the array first and those will get chosen.
#define COVERFILENAMEPRIORITYENABLED 1
// If the cover isn't found in the audio file's folder, try going up a directory. Can repeat MAXDIRUP times
#define MAXDIRUP 1
// If you want to fallback on a random loadable image in the same folder as the song if a known cover filename isn't found
#define FALLBACKRANDOM 1
// 1 if you will allow a cover to a song to be the same filename as the song, just with an image extension. For example, bla.jpg as a cover for bla.wav.
#define SAMEASSONGNAMECOVER 1
// 1 if if you want the program to turn cue:// files into normal file paths
#define STRIPCUE 1
// Background/Border color
#define BACKGROUNDCOLOR 0,0,0,255
