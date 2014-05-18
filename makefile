CC=g++
CFLAGS=-D__GXX_EXPERIMENTAL_CXX0X__ -O3 -Wall -std=c++11 -Wl,--no-as-needed
LDFLAGS=-pthread
SOURCES=*cpp
EXECUTABLE=gunborg

all: clean $(EXECUTABLE)
	
$(EXECUTABLE): 
	$(CC) $(CFLAGS) $(SOURCES) -o $@ $(LDFLAGS)

win64:
	x86_64-w64-mingw32-c++ $(CFLAGS) $(SOURCES) -o $(EXECUTABLE)_w64.exe $(LDFLAGS)


win32:
	i686-w64-mingw32-c++ $(CFLAGS) $(SOURCES) -o $(EXECUTABLE)_w32.exe $(LDFLAGS)

clean:
	rm -f $(EXECUTABLE)
	rm -f $(EXECUTABLE)_w64.exe
	rm -f $(EXECUTABLE)_w32.exe

