#include <windows.h>

typedef struct _bstr {
  char *text;
  unsigned length; /* Length of current text, NOT including terminating null */
  unsigned bufsiz; /* Size of the allocated memory buffer */
} bstr;

typedef struct _InstFiles {
  char *filename;
  DWORD filesize;
  struct _InstFiles *next;
} InstFiles;

typedef struct _InstLink {
  char *linkfile;
  char *origfile;
  char *args;
  struct _InstLink *next;
} InstLink;

typedef struct _InstData {
  char *product;
  char *installdir,*startmenudir;
  DWORD totalsize;
  InstFiles *instfiles;
  InstFiles *extrafiles;
  InstLink *startmenu;
  InstLink *desktop;
} InstData;

extern const char *UninstallKey;
extern const char *UninstallEXE;

void *bmalloc(UINT numbytes);
void bfree(void *pt);
void *brealloc(void *pt,UINT numbytes);
char *bstrdup(char *str);

bstr *bstr_new(void);
void bstr_free(bstr *str,BOOL free_text);
void bstr_expandby(bstr *str,unsigned extralength);
void bstr_setlength(bstr *str,unsigned length);
void bstr_assign(bstr *str,const char *text);
void bstr_append(bstr *str,const char *text);
void bstr_appendpath(bstr *str,const char *text);
void bstr_append_c(bstr *str,const char ch);
void bstr_append_long(bstr *str,const long val);
void bstr_append_windir(bstr *str);
void bstr_append_curdir(bstr *str);
void bstr_assign_windir(bstr *str);
void bstr_assign_curdir(bstr *str);

void DisplayError(const char *errtext,BOOL addsyserr,BOOL fatal);
void AddInstFiles(char *filename,DWORD filesize,InstFiles **lastpt,
                  InstFiles **firstpt);
void AddInstLink(char *linkfile,char *origfile,char *args,InstLink **lastpt,
                 InstLink **firstpt);
void FreeLinkList(InstLink *linklist,BOOL freepts);
void FreeFileList(InstFiles *filelist,BOOL freepts);
void FreeInstData(InstData *idata,BOOL freepts);
void WriteLinkList(HANDLE fout,InstLink *listpt);
void WriteFileList(HANDLE fout,InstFiles *listpt);
char *GetStartMenuTopDir(void);
char *GetStartMenuDir(InstData *idata);
char *GetDesktopDir(void);
BOOL CreateWholeDirectory(char *path);
BOOL RemoveWholeDirectory(char *path);
