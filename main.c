#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#if _has_libavformat
#include <libavformat/avformat.h>
#endif
#include "config.h"

volatile sig_atomic_t shouldRecheck=1;
SDL_Renderer* mainWindowRenderer;

// Catch SIGUSR1
static void refreshcatch(const int signo){
	(void)signo;
	shouldRecheck=1;
}
// string should not include dot
char isLoadableExtension(const char* _passedFilename){
	int i;
	for (i=0;i<NUMEXTENSIONS;++i){
		if (strncasecmp(_passedFilename,loadableExtensions[i],strlen(loadableExtensions[i]))==0){
			return 1;
		}
	}
	return 0;
}
const char* getExtensionStart(const char* _passedFilename){
	int i;
	for (i=strlen(_passedFilename)-1;i>=0;--i){
		if (_passedFilename[i]=='.'){
			return &(_passedFilename[i+1]);
		}
	}
	return NULL;
}
char isLoadableFilename(const char* _passedFilename){
	const char* _possibleExtension = getExtensionStart(_passedFilename);
	if (_possibleExtension!=NULL){
		return isLoadableExtension(_possibleExtension);
	}
	return 0;
}
char filenamesMatch(const char* _passedCandidate, const char* _passedAcceptable, char _filenameCaseSensitive, char _canAsterisk){
	int _matcherLen = strlen(_passedAcceptable);
	if (_canAsterisk && _passedAcceptable[_matcherLen-1]=='*'){
		--_matcherLen;
	}
	if (_filenameCaseSensitive){
		return (strncmp(_passedCandidate,_passedAcceptable,_matcherLen)==0);
	}else{
		return (strncasecmp(_passedCandidate,_passedAcceptable,_matcherLen)==0);
	}
}
FILE* goodpopen(char* const _args[]){
	int _crossFiles[2]; // File descriptors that work for both processes
	if (pipe(_crossFiles)!=0){
		return NULL;
	}
	pid_t _newProcess = fork();
	if (_newProcess==-1){
		close(_crossFiles[0]);
		return NULL;
	}else if (_newProcess==0){ // 0 is returned to the new process
		close(_crossFiles[0]); // Child does not ned to read
		dup2(_crossFiles[1], STDOUT_FILENO); // make _crossFiles[1] be the same as STDOUT_FILENO
		// First arg is the path of the file again
		execv(_args[0],_args); // This will do this program and then end the child process
		exit(1); // This means execv failed
	}

	close(_crossFiles[1]); // Parent doesn't need to write
	FILE* _ret = fdopen(_crossFiles[0],"r");
	waitpid(_newProcess,NULL,0);
	return _ret;
}
void seekPast(FILE* fp, unsigned char _target){
	while (1){
		int _lastRead=fgetc(fp);
		if (_lastRead==_target || _lastRead==EOF){
			break;
		}
	}
}
void seekNextLine(FILE* fp){
	seekPast(fp,0x0A);
}
SDL_Texture* memToTexture(void* _buff, size_t _len){
	SDL_Texture* _ret=NULL;
	SDL_RWops* _readableBuffer = SDL_RWFromMem(_buff,_len);
	if (_readableBuffer!=NULL){
		SDL_Surface* _tempSurface = IMG_Load_RW(_readableBuffer,1); // closes SDL_RWops for us
		if (_tempSurface!=NULL){
			if ((_ret = SDL_CreateTextureFromSurface(mainWindowRenderer,_tempSurface))==NULL){
				fprintf(stderr,"SDL_CreateTextureFromSurface failed\n");
			}
			SDL_FreeSurface(_tempSurface);
		}else{
			fprintf(stderr,"Failed to load buffer as image.\n");
		}
	}else{
		fprintf(stderr,"Failed SDL_RWFromMem %s\n",SDL_GetError());
	}
	return _ret;
}
SDL_Texture* getEmbeddedCover(char* _currentFilename){
	#if _has_libavformat
	SDL_Texture* _ret=NULL;
	AVFormatContext *pFormatCtx = avformat_alloc_context();
	if (avformat_open_input(&pFormatCtx,_currentFilename,NULL,NULL)==0){
		for (int i=0;i<pFormatCtx->nb_streams;++i){
			if (pFormatCtx->streams[i]->disposition & AV_DISPOSITION_ATTACHED_PIC) {
				AVPacket pkt = pFormatCtx->streams[i]->attached_pic;
				_ret=memToTexture(pkt.data,pkt.size);
				break;
			}
		}
		avformat_close_input(&pFormatCtx);
	}
	return _ret;
	#else
	return NULL;
	#endif
}
SDL_Texture* getCoverByStandaloneImage(char* _currentFilename){
	// NOTE - If I give up the ability to have a cover file be the music filename just with image extension, I can store the last song as the last folder and not rescan the same folder again. Useful for those who play songs in order.
	SDL_Texture* _ret=NULL;
	// Find the folder of the song file by finding the last slash
	char* _pathSongFolder=strdup(_currentFilename); // Once it contains a folder path, that folder path will end with DIRSEPARATORCHAR
	int _currentSubDirUp=0;
	char* _gottenCoverFilename=NULL;
	for (_currentSubDirUp=0;_currentSubDirUp<=MAXDIRUP;++_currentSubDirUp){
		int i;
		for (i=strlen(_pathSongFolder)-2;i>=0;--i){ // Skip the ending char. This goes over either one char of the filename or the end DIRSEPARATORCHAR from the last path
			if (_pathSongFolder[i]==DIRSEPARATORCHAR){
				_pathSongFolder[i+1]='\0';
				break;
			}
		}
		if (i>=0){ // If we actually found a new _pathSongFolder
			#if SAMEASSONGNAMECOVER
			int _musicNoExtensionLen=-1;
			int _cachedDirLength;
			char _oldChar;
			const char* _extensionPos = getExtensionStart(_currentFilename);
			if (_extensionPos!=NULL){
				_cachedDirLength = strlen(_pathSongFolder);
				_musicNoExtensionLen = _extensionPos-_currentFilename-1;
				// Temporarily trim the extension off the filename
				_oldChar = _currentFilename[_musicNoExtensionLen];
				_currentFilename[_musicNoExtensionLen]='\0';
			}else{
				fprintf(stderr,"No extension for %s\n",_currentFilename);
			}										
			#endif
			// Find cover image in folder
			DIR* _songFolder=opendir(_pathSongFolder);
			if (_songFolder!=NULL){
				#if FALLBACKRANDOM
				char* _backupCoverFilename=NULL; // For images that can be loaded, but don't match a known cover filename. Load this if _gottenCoverFilename is NULL
				#endif
				errno=0;
				struct dirent* _currentEntry;
				while((COVERFILENAMEPRIORITYENABLED || _gottenCoverFilename==NULL) && (_currentEntry=readdir(_songFolder))){
					if (isLoadableFilename(_currentEntry->d_name)){
						#if SAMEASSONGNAMECOVER
						// I chose to compare every filename to the music's filename.
						// I could've checked if files exist using the music's filename and all valid extensions, but I feel that checking file existance would be worse than string comparison.
						if (_musicNoExtensionLen!=-1 && filenamesMatch(_currentEntry->d_name,&(_currentFilename[_cachedDirLength]),1,0)){
							free(_gottenCoverFilename);
							_gottenCoverFilename = strdup(_currentEntry->d_name);
							continue;
						}
						#endif
						// If this is one of the known cover filenames
						for (i=0;i<NUMCOVERFILENAMES;++i){
							if (filenamesMatch(_currentEntry->d_name,coverFilenames[i],0,1)){
								free(_gottenCoverFilename);
								_gottenCoverFilename=strdup(_currentEntry->d_name);
								break;
							}
						}
						#if FALLBACKRANDOM
						// If it's not one of the known filenames, still keep it for later if we need to fall back on it
						if (i==NUMCOVERFILENAMES){
							free(_backupCoverFilename);
							_backupCoverFilename = strdup(_currentEntry->d_name);
						}
						#endif
					}
				}
				#if FALLBACKRANDOM
				// Use the backup if needed
				if (_gottenCoverFilename==NULL){
					_gottenCoverFilename=_backupCoverFilename;
				}else{
					free(_backupCoverFilename);
				}
				#endif
				if (errno!=0){
					fprintf(stderr,"Failure when reading directory entries, errno %d\n",errno);
				}
				if (closedir(_songFolder)==-1){
					fprintf(stderr,"Failed to close dir\n");
				}
			}else{
				fprintf(stderr,"Failed to open folder %s\n",_pathSongFolder);
			}
			#if SAMEASSONGNAMECOVER
			if (_musicNoExtensionLen!=-1){
				// Restore the old filename
				_currentFilename[_musicNoExtensionLen]=_oldChar;
			}
			#endif
			// If we've got a cover image, exit early
			if (_gottenCoverFilename!=NULL){
				// We must exit right now because the _gottenCoverFilename we have right now relies on the _pathSongFolder that we also have right now. If we continue, the _pathSongFolder will change.
				break;
			}
		}else{
			fprintf(stderr,"Failed to get folder from path\n");
		}
	}
	// Load the cover image if found
	if (_gottenCoverFilename!=NULL){
		// Expand to a full file path
		char* _fullFilename = malloc(strlen(_pathSongFolder)+strlen(_gottenCoverFilename)+1);
		strcpy(_fullFilename,_pathSongFolder);
		strcat(_fullFilename,_gottenCoverFilename);
		printf("Got art %s\n",_fullFilename);
		//
		SDL_Surface* _tempSurface=IMG_Load(_fullFilename);
		if (_tempSurface!=NULL){
			if ((_ret = SDL_CreateTextureFromSurface(mainWindowRenderer,_tempSurface))==NULL){
				fprintf(stderr,"SDL_CreateTextureFromSurface failed\n");
			}
			SDL_FreeSurface(_tempSurface);
		}else{
			fprintf(stderr,"Failed to load %s with error %s\n",_fullFilename,IMG_GetError());
		}
		free(_fullFilename);
		free(_gottenCoverFilename);
	}
	free(_pathSongFolder);
	return _ret;
}
int stripCueData(char* _currentFilename){ // returns 1 if something was stripped, 0 if not
	// playing a .cue file puts "cue:///path/to/blah.cue/track-number" in cmus-remote -Q
	// this function will strip this and just leave the file path
	#define CUE_PREFIX "cue://"
	if (_currentFilename == NULL) return 0;
	int l = strlen(_currentFilename), cl = strlen(CUE_PREFIX), i;
	if (l < cl || memcmp(_currentFilename, CUE_PREFIX, cl) != 0) return 0; // check if CUE file
	for (i = 0; i < l-cl; ++i) _currentFilename[i] = _currentFilename[i + cl]; // get rid of CUE_PREFIX
	char* c;
	for (c = _currentFilename + l - cl - 1; c >= _currentFilename; --c) {
		if (*c == '/') {
			*c = '\0';
			break;
		}
	}
	return 1;
}
int main(int argc, char const *argv[]){
	// Catch SIGUSR1. Use it as a signal to refresh
	struct sigaction refreshsig;
	memset(&refreshsig, 0, sizeof(refreshsig));
	refreshsig.sa_handler = refreshcatch;
	sigaction(SIGUSR1, &refreshsig, NULL);

	struct timespec _eventCheckTime;
	_eventCheckTime.tv_sec=0;
	_eventCheckTime.tv_nsec = EVENTPOLLTIME*1000000L;

	SDL_Window* mainWindow;
	SDL_Init(SDL_INIT_VIDEO);
	IMG_Init( IMG_INIT_PNG );
	IMG_Init( IMG_INIT_JPG );
	mainWindow = SDL_CreateWindow(WINDOWTITLE,SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,640,480,SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN );
	if (!mainWindow){
		fprintf(stderr,"SDL_CreateWindow failed\n");
		return 1;
	}
	mainWindowRenderer = SDL_CreateRenderer( mainWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!mainWindowRenderer){
		fprintf(stderr,"SDL_CreateRenderer failed.\n");
		return 1;
	}
	unsigned int lastRefresh=0;
	char* _currentFilename=NULL;
	SDL_Texture* _currentImage=NULL;
	char running=1;
	while(running){
		SDL_Event e;
		while(SDL_PollEvent(&e)!=0){
			if(e.type==SDL_QUIT){
				running=0;
			}else if(e.type==SDL_KEYDOWN){
				if (e.key.keysym.sym==SDLK_q){
					running=0;
				}
			}
		}

		unsigned int _curTime=0;
		#if RECHECKMILLI != -1
			_curTime=SDL_GetTicks()+RECHECKMILLI; // Offset all time values by RECHECKMILLI so when the program starts it'll check if RECHECKMILLI > 0 
			if (_curTime>=lastRefresh+RECHECKMILLI){
				shouldRecheck=1;
			}
		#endif
		if (shouldRecheck){
			shouldRecheck=0;
			lastRefresh=_curTime;
			// Use cmus-remote to see if we've got a new song playing
			FILE* _cmusRes = goodpopen(programArgs);
			char _foundFile=0;
			while (!feof(_cmusRes)){
				// Find the 'file' line
				if (fgetc(_cmusRes)!='f'){
					seekNextLine(_cmusRes);
				}else{
					_foundFile=1;
					seekPast(_cmusRes,' '); // Seek to the end of 'file '
					char* _readFile=NULL;
					size_t _readLen=0;
					if (getline(&_readFile,&_readLen,_cmusRes)!=-1){
						// Strip newline left on the string by getline
						int _cachedStrlen=strlen(_readFile);
						if (_readFile[_cachedStrlen-1]=='\n'){
							_readFile[--_cachedStrlen]='\0';
						}

						// Strip cue data if enabled
						#ifdef STRIPCUE
						stripCueData(_readFile);
						#endif

						// If the filename of the current song is different from the last one
						if (_currentFilename==NULL || strcmp(_readFile,_currentFilename)!=0){
							printf("Looking for art\n");
							if (_currentImage){ // No matter what, we don't want the old art anymore
								SDL_DestroyTexture(_currentImage);
								_currentImage=NULL;
							}
							free(_currentFilename);
							_currentFilename=_readFile;

							// Try and get cover image using various ways
							int i;
							for (i=0;i<NUMCOVERGETTERS && ((_currentImage=coverGetters[i](_currentFilename))==NULL);++i);
						}
					}
					if (_currentFilename!=_readFile){
						free(_readFile);
					}
				}
			}
			if (!_foundFile){ // If cmus isn't running you wouldn't find a file
				if (_currentImage){
					SDL_DestroyTexture(_currentImage);
					_currentImage=NULL;
				}
				free(_currentFilename);
				_currentFilename=NULL;
			}
			fclose(_cmusRes);
		}
		// SDL says you must redraw everything every time you use SDL_RenderPresent
		SDL_SetRenderDrawColor(mainWindowRenderer,BACKGROUNDCOLOR);
		SDL_RenderClear(mainWindowRenderer);
		if (_currentImage!=NULL){
			SDL_Rect _srcRect;
			_srcRect.x=0;
			_srcRect.y=0;
			SDL_QueryTexture(_currentImage, NULL, NULL, &(_srcRect.w), &(_srcRect.h));

			SDL_Rect _destRect;
			int _windowWidth;
			int _windowHeight;
			SDL_GetWindowSize(mainWindow,&_windowWidth,&_windowHeight);
			_windowWidth-=PIXELPADDING*2;
			_windowHeight-=PIXELPADDING*2;

			int _imgW;
			int _imgH;
			SDL_QueryTexture(_currentImage,NULL,NULL,&_imgW,&_imgH);

			double _scaleFactor;
			if (_windowWidth/(double)_imgW<_windowHeight/(double)_imgH){
				_scaleFactor=_windowWidth/(double)_imgW;
			}else{
				_scaleFactor=_windowHeight/(double)_imgH;
			}
			_destRect.w=_srcRect.w*_scaleFactor;
			_destRect.h=_srcRect.h*_scaleFactor;
			_destRect.x=PIXELPADDING+(_windowWidth-_destRect.w)/2;
			_destRect.y=PIXELPADDING+(_windowHeight-_destRect.h)/2;

			SDL_RenderCopy(mainWindowRenderer, _currentImage, &_srcRect, &_destRect );
		}
		SDL_RenderPresent(mainWindowRenderer);
		int _nextCmusCheckTime;
		#if RECHECKMILLI != -1
			_nextCmusCheckTime = (lastRefresh+RECHECKMILLI+10)-_curTime;
		#else
			_nextCmusCheckTime = INT32_MAX;
		#endif
		#if WAITMODE == 0
			// Based on SDL_WaitEventTimeout
			unsigned int _maxTime = SDL_GetTicks()+_nextCmusCheckTime;
			while(1){
				SDL_PumpEvents();
				int _numNewEvents = SDL_PeepEvents(NULL,1,SDL_GETEVENT,SDL_FIRSTEVENT,SDL_LASTEVENT);
				if (_numNewEvents!=0){ // Accounts for errors too
					break;
				}else{
					if (shouldRecheck || SDL_GetTicks()>=_maxTime){
						shouldRecheck=1;
						break;
					}else{
						nanosleep(&_eventCheckTime,NULL);
					}
				}
			}
		#elif WAITMODE == 1
			struct timespec _waitTime;
			_waitTime.tv_sec=0;
			_waitTime.tv_nsec = ALTWAITMODEREDRAWTIME < _nextCmusCheckTime*1000000L ? ALTWAITMODEREDRAWTIME : _nextCmusCheckTime*1000000L;
			nanosleep(&_waitTime,NULL);
		#endif
	}
	free(_currentFilename);
	SDL_DestroyWindow(mainWindow);
	return 0;
}
