typedef SDL_Texture*(*getCoverFunction)(char*);
SDL_Texture* getCoverByStandaloneImage(char* _currentFilename);
SDL_Texture* getEmbeddedCover(char* _currentFilename);
