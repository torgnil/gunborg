CC=g++
CFLAGS=-D__GXX_EXPERIMENTAL_CXX0X__ -O3 -Wall -std=c++11 -Wl,--no-as-needed -march=native
LDFLAGS=-pthread -flto -O3
SOURCES=*cpp
EXECUTABLE=gunborg

all: clean $(EXECUTABLE)
	
$(EXECUTABLE): 
	$(CC) $(CFLAGS) $(SOURCES) -o $@ $(LDFLAGS)

win64:
	x86_64-w64-mingw32-g++ $(CFLAGS) $(SOURCES) -o $(EXECUTABLE)_w64.exe -static -static-libstdc++ -lpthread

clean:
	rm -f $(EXECUTABLE)
	rm -f $(EXECUTABLE)_w64.exe


