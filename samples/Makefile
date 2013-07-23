CPP = polyline.cpp segment.cpp samples.cpp
BIN = $(CPP:.cpp=)

all: $(BIN)

polyline: polyline.cpp png_readwrite.cpp test1_base.cpp
	fltk-config --use-gl --use-images --compile polyline.cpp

segment: segment.cpp test1_base.cpp
	fltk-config --use-gl --compile segment.cpp

samples: samples.cpp test1_base.cpp
	fltk-config --use-gl --compile samples.cpp

win32: $(CPP)
	$(foreach var,$(BIN),g++ -I/usr/local/include -I/usr/local/include/FL/images -mwindows -DWIN32 -DUSE_OPENGL32 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -o $(var) $(var).cpp -mwindows /usr/local/lib/libfltk_gl.a -lglu32 -lopengl32 /usr/local/lib/libfltk.a -lole32 -luuid -lcomctl32 -static-libgcc -static-libstdc++ & )
