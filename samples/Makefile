all: polyline segment samples

polyline: polyline.cpp png_readwrite.cpp test1_base.cpp
	fltk-config --use-gl --use-images --compile polyline.cpp

segment: segment.cpp test1_base.cpp
	fltk-config --use-gl --compile segment.cpp

samples: samples.cpp test1_base.cpp
	fltk-config --use-gl --compile samples.cpp
