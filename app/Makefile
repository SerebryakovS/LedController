CXXFLAGS=-std=c++11 -O3 -W -Wall -Wextra -Wno-unused-parameter
OBJECTS=Controller.o Splasher.o Json.o
BINARIES=Controller Splasher

RGB_LIB_DISTRIBUTION=..
RGB_INCDIR=$(RGB_LIB_DISTRIBUTION)/include
RGB_LIBDIR=$(RGB_LIB_DISTRIBUTION)/lib
RGB_LIBRARY_NAME=rgbmatrix
RGB_LIBRARY=$(RGB_LIBDIR)/lib$(RGB_LIBRARY_NAME).a
LDFLAGS+=-L$(RGB_LIBDIR) -l$(RGB_LIBRARY_NAME) -lrt -lm -lpthread -L/usr/local/lib

# Imagemagic and AV flags, only needed if actually compiled.
MAGICK_CXXFLAGS?=$(shell GraphicsMagick++-config --cppflags --cxxflags)
MAGICK_LDFLAGS?=$(shell GraphicsMagick++-config --ldflags --libs)
AV_CXXFLAGS=$(shell pkg-config --cflags  libavcodec libavformat libswscale libavutil)
AV_LDFLAGS=$(shell pkg-config --cflags --libs  libavcodec libavformat libswscale libavutil)

all : $(BINARIES)

$(RGB_LIBRARY): FORCE
	$(MAKE) -C $(RGB_LIBDIR)

Controller: Controller.o Json.o $(RGB_LIBRARY)
	$(CXX) $(CXXFLAGS) Controller.o Json.o -o $@ $(LDFLAGS) -lcjson

SinglePanel: SinglePanel.o $(RGB_LIBRARY)
	$(CXX) $(CXXFLAGS) SinglePanel.o -o $@ $(LDFLAGS)


Splasher: Splasher.o $(RGB_LIBRARY)
	$(CXX) $(CXXFLAGS) Splasher.o -o $@ $(LDFLAGS) $(MAGICK_LDFLAGS)

%.o : %.cc
	$(CXX) -I$(RGB_INCDIR) $(CXXFLAGS) -c -o $@ $<

Splasher.o : Splasher.cc
	$(CXX) -I$(RGB_INCDIR) $(CXXFLAGS) $(MAGICK_CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(BINARIES)

FORCE:
.PHONY: FORCE
