#WARNINGS=-Wall -Wextra -pedantic -pedantic-errors -Wcast-align -Wcast-qual \
         -Wchar-subscripts -Wcomment -Wconversion -Wdisabled-optimization \
         -Wfloat-equal -Wformat -Wformat=2 -Wformat-nonliteral \
         -Wformat-security -Wformat-y2k -Wimport -Winit-self -Winline \
         -Winvalid-pch -Wlong-long -Wmissing-braces -Wmissing-format-attribute \
         -Wmissing-noreturn -Wpacked -Wparentheses -Wpointer-arith \
         -Wredundant-decls -Wreturn-type -Wshadow -Wsign-compare \
         -Wstrict-aliasing -Wstrict-aliasing=2 -Wswitch -Wswitch-default \
         -Wswitch-enum -Wtrigraphs -Wuninitialized -Wunknown-pragmas \
         -Wunreachable-code -Wunused -Wunused-function -Wunused-label \
         -Wunused-parameter -Wunused-value -Wunused-variable -Wno-write-strings
WARNINGS=-Wno-write-strings -Werror
FLAGS=-g -I/usr/include/freetype3 -march=x86-64 -mtune=generic -pipe -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_THREAD_SAFE -D_REENTRANT -fPIE -fPIC
CPPFLAGS=$(FLAGS) $(WARNINGS)
LDFLAGS=$(FLAGS) -lfltk -lcurl
CPP=g++
OBJS=$(patsubst %.cpp,%.o,$(wildcard *.cpp))
TARGET=dj

all: $(TARGET)

$(OBJS) : $(wildcard *.h) Makefile

$(TARGET): $(OBJS)
	$(CPP) $(OBJS) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET) $(OBJS)
