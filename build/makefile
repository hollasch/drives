
LIBS = kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib \
       advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib \
       odbccp32.lib

OUT = ..\out\x64\Debug
SRC = ..\src
SRCFromOut = ..\..\..\src

all: $(OUT)\netuse.exe

$(OUT)\netuse.exe: $(SRC)\netuse.cpp
    -mkdir 2>nul $(OUT)
    cd $(OUT) & cl -nologo /Zi $(SRCFromOut)\netuse.cpp

clean clobber:
	-cd $(OUT) & del 2>nul netuse.* vc120.pdb

fresh: clobber all
