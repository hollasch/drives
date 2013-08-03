
LIBS = kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib

all: netuse.exe

netuse.exe: netuse.cpp
	cl -nologo -Ox netuse.cpp

clean clobber:
	-del 2>nul *.obj *.exe *.pdb *.ilk

fresh: clobber all
