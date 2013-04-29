BINS := hqz

HQZ_OBJS := \
	src/zrender.o \
	src/histogramimage.o \
	src/spectrum.o \
	src/main.o \
	src/lodepng.o

CDEPS := src/*.h

CCFLAGS := -Isrc
LIBS := -lstdc++ -lm

CCFLAGS += -Wall -Wno-gnu -Werror -g
CCFLAGS += -O3 -ffast-math -fno-exceptions

all: $(BINS)

hqz: $(HQZ_OBJS)
	$(CC) -o $@ $(HQZ_OBJS) $(LIBS)

%.o: %.cpp $(CDEPS)
	$(CC) -c -o $@ $< $(CCFLAGS)

# Simple benchmarking target
time: hqz
	time ./hqz example.json example.png
	open example.png

.PHONY: clean time

clean:
	rm -f $(BINS) $(HQZ_OBJS)
