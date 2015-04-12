
LIBS = kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib

OUT = ..\..\out\win
SRC = ..\..\src

all: $(OUT)\netuse.exe

$(OUT)\netuse.exe: $(SRC)\netuse.cpp
    -md 2>nul $(OUT)
    cd $(OUT) & cl -nologo -Ox ..\..\src\netuse.cpp

clean clobber:
	-cd $(OUT) & del 2>nul *.obj *.exe *.pdb *.ilk

fresh: clobber all
