
LIBS = kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib

all: ..\out\netuse.exe

..\out\netuse.exe: netuse.cpp
    -md 2>nul ..\out
    cd ..\out & cl -nologo -Ox ..\src\netuse.cpp

clean clobber:
	-del 2>nul ..\out\*.obj ..\out\*.exe ..\out\*.pdb ..\out\*.ilk

fresh: clobber all
