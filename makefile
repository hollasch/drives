
LIBS = kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib

all: drives.exe netuse.exe

drives.exe: drives.cpp
	cl -Zi -nologo drives.cpp MPR.lib

netuse.exe: netuse.cpp
	cl -nologo -Ox netuse.cpp

$(BINDIR)\drives.exe: drives.exe
    if defined BINDIR copy /y $? %%BINDIR%%

install: $(BINDIR)\drives.exe

clean clobber:
	-del 2>nul *.obj *.exe *.pdb *.ilk

fresh: clobber all
