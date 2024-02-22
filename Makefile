RGB_LIBDIR=./lib
RGB_LIBRARY_NAME=rgbmatrix
RGB_LIBRARY=$(RGB_LIBDIR)/lib$(RGB_LIBRARY_NAME).a

# Some language bindings.
CXXFLAGS += -std=c++11
all : $(RGB_LIBRARY)

$(RGB_LIBRARY): FORCE
	$(MAKE) -C $(RGB_LIBDIR)

clean:
	$(MAKE) -C lib clean

FORCE:
.PHONY: FORCE
