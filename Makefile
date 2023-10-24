
CC	= g++

LINK= g++

CINCPATHFLAGS = -Iincludes\
				-Imain\
				-Isrc\
                -I/root/workspace/ArenaSDK_Linux_x64/include/Arena\
                -I/root/workspace/ArenaSDK_Linux_x64/include/Save\
				-I/root/workspace/ArenaSDK_Linux_x64/GenICam/library/CPP/include\

VPATH		=   src:\
	            includes:\
	            main:

LBITS := $(shell getconf LONG_BIT)
ifeq ($(LBITS),64)
	LDFLAGS = -L$(ARENA_ROOT)/GenICam/library/lib/Linux64_x64\
			  -L$(ARENA_ROOT)/lib64\
			  -Wl,--allow-shlib-undefined,--as-needed
	LIBS := -lGCBase_gcc54_v3_3_LUCID -lGenApi_gcc54_v3_3_LUCID -larena -lsave -lgentl
	CFLAGS	= -O2 -D_FORTIFY_SOURCE=0 -g3 -m64 $(CINCPATHFLAGS)
endif

SKIP = %main.cpp 
SRCC = $(wildcard main/*.cpp src/*.cpp)
SRC = $(filter-out $(SKIP), $(SRCC)) 
OBJS = $(SRC:.cpp=.o)

HEADERS = $(wildcard includes/*.h)

EXE =	gps-sdr

all: $(EXE)
	@echo ---- Build Complete ----

gps-sdr: main.o $(OBJS) $(HEADERS)
	 $(LINK) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o:%.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@ 

clean: distclean

minclean: oclean

oclean:
	@rm -rvf `find . \( -name "*.o" \) -print` 	
	
distclean:
	@rm -rvf `find . \( -name "*.o" -o -name "*.dis" -o -name "*.dat" -o -name "*.klm" -o -name "*.m~" -o -name "*.tlm" -o -name "*.log" -o -name "gps-*" \) -print`
	
