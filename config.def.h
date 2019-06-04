#define WINDOWTITLE "Cover"
// Minimum amount of time to pass before the program checks for song change
// Set this to -1 to disable automatic refresh, but then the program won't be able to detect when cmus closes.
#define RECHECKMILLI 1000
//
#define PIXELPADDING 5
// 1 if you will allow a cover to a song to be the same filename as the song, just with an image extension. For example, bla.jpg as a cover for bla.wav.
#define SAMEASSONGNAMECOVER 1
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
// Program arguments to get info from cmus-remote
// You may need to change the path to /usr/bin/local if you installed cmus yourself
char* const programArgs[] = {
	"/usr/bin/cmus-remote",
	"-Q",
	NULL,
};
// If the cover isn't found in the audio file's folder, try going up a directory. Can repeat MAXDIRUP times
#define MAXDIRUP 1
// Change this if you're using Windows
#define DIRSEPARATORCHAR '/'
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