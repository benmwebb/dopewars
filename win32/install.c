#include <stdio.h>
#include <windows.h>

int main() {
  HRSRC hrsrc;
  HGLOBAL hglobal;
  DWORD ressize,i;
  LPVOID respt;
  HANDLE fout;
  DWORD byteswritten;

  printf("Looking for resource...\n");

  hrsrc = FindResource(NULL,MAKEINTRESOURCE(10),"INSTFILE");
  if (!hrsrc) { printf("Could not find resource!\n"); return 1; }

  ressize=SizeofResource(NULL,hrsrc);
  printf("Resource found; size returned = %ld bytes\n",ressize);

  hglobal = LoadResource(NULL,hrsrc);
  if (!hglobal) { printf("Could not load resource!\n"); return 1; }

  respt = LockResource(hglobal);
  if (!respt) { printf("Could not lock resource!\n"); return 1; }

  fout = CreateFile("windowsout.html",GENERIC_WRITE,0,NULL,CREATE_ALWAYS,
                    0,NULL);
  if (!fout) { printf("Could not open file\n"); return 1; }

  if (!WriteFile(fout,respt,ressize,&byteswritten,NULL)) {
    printf("Write to file failed - %ld of %ld bytes written\n",
           byteswritten,ressize);
  }
  CloseHandle(fout);
  
  return 0;
}
