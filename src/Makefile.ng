all: main

CPPFLAGS = -Wall -Wextra  `pkg-config --cflags glib-2.0`
LDFLAGS = `pkg-config --libs glib-2.0`
CXX = gcc $(GCCOPTS) -fno-rtti -fno-exceptions -Wabi -Wpadded -Wcast-align

main: main.cc *.h
	$(CXX) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $<

clean:
	rm -f main main.o
