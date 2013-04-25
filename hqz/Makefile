BINS := hqz

HQZ_OBJS := \
	src/zrender.o \
	src/main.o \
	src/lodepng.o

CDEPS := src/*.h

CCFLAGS := -Isrc
LIBS := -lstdc++ -lm

CCFLAGS += -Wall -Werror -g
CCFLAGS += -O3 -ffast-math -fno-exceptions

all: $(BINS)

hqz: $(HQZ_OBJS)
	$(CC) -o $@ $(HQZ_OBJS) $(LIBS)

%.o: %.cpp $(CDEPS)
	$(CC) -c -o $@ $< $(CCFLAGS)

.PHONY: clean

clean:
	rm -f $(BINS) $(HQZ_OBJS)
