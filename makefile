CC=g++
CFLAGS=-D__GXX_EXPERIMENTAL_CXX0X__ -O3 -Wall -std=c++11 -Wl,--no-as-needed
LDFLAGS=-pthread
SOURCES=*cpp
EXECUTABLE=gunborg

all: clean $(EXECUTABLE)
	
$(EXECUTABLE): 
	$(CC) $(CFLAGS) $(SOURCES) -o $@ $(LDFLAGS)

clean:
	rm -f $(EXECUTABLE)

