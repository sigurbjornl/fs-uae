
bool initvideograb(const TCHAR *filename);
void uninitvideograb(void);
bool getvideograb(long **buffer, int *width, int *height);
void pausevideograb(int pause);
uae_s64 getsetpositionvideograb(uae_s64 framepos);
bool isvideograb(void);
bool getpausevideograb(void);
void setvolumevideograb(int volume);
void isvideograb_status(void);
