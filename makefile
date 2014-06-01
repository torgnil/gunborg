CC=g++
CFLAGS=-D__GXX_EXPERIMENTAL_CXX0X__ -O3 -Wall -std=c++0x -Wl,--no-as-needed
LDFLAGS=-pthread -flto -O3
SOURCES=*cpp
EXECUTABLE=gunborg

all: clean $(EXECUTABLE)
	
$(EXECUTABLE): 
	$(CC) $(CFLAGS) $(SOURCES) -o $@ $(LDFLAGS)

modern:
	$(CC) $(CFLAGS) $(SOURCES) -msse4.2 -o $(EXECUTABLE)_$@ $(LDFLAGS)

w64:
	x86_64-w64-mingw32-g++ $(CFLAGS) $(SOURCES) -o $(EXECUTABLE)_$@.exe -static -static-libstdc++ -lpthread

w64_modern:
	x86_64-w64-mingw32-g++ $(CFLAGS) -msse4.2 $(SOURCES) -o $(EXECUTABLE)_$@.exe -static -static-libstdc++ -lpthread

clean:
	rm -f $(EXECUTABLE)
	rm -f $(EXECUTABLE)_modern
	rm -f $(EXECUTABLE)_w64.exe
	rm -f $(EXECUTABLE)_w64_modern.exe

