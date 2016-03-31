all: lib/capture.o lib/filter.o lib/glhelper.o lib/blob.o lib/tracker.o lib/fiducial.o lib/tracker2.o lib/disjointset.o lib/queue.o lib/widget.o lib/tuio.o
	g++ main.cc lib/capture.o lib/filter.o lib/glhelper.o lib/blob.o lib/tracker.o lib/fiducial.o lib/tracker2.o lib/disjointset.o lib/queue.o lib/widget.o lib/tuio.o cvblobslib/libblob.a -o bin/main -L/usr/lib `sdl-config --cflags --libs` -lSDL_ttf -lSDL_image `pkg-config opencv --cflags --libs` -lGL -lGLU -lvlc -lpthread

lib/capture.o: src/capture.h src/capture.cc
	g++ src/capture.cc -c -o lib/capture.o -I/usr/include/opencv/

lib/filter.o: lib/capture.o src/filter.h src/filter.cc
	g++ src/filter.cc  -c -o lib/filter.o

lib/glhelper.o: lib/blob.o src/glhelper.h src/glhelper.cc
	g++ src/glhelper.cc -c -o lib/glhelper.o

lib/blob.o: src/blob.h src/blob.cc
	g++ src/blob.cc -c -o lib/blob.o

lib/tracker.o: lib/blob.o src/tracker.h src/tracker.cc
	g++ src/tracker.cc -c -o lib/tracker.o -I/usr/include/opencv/

lib/fiducial.o: lib/blob.o src/fiducial.h src/fiducial.cc
	g++ src/fiducial.cc -c -o lib/fiducial.o

lib/tracker2.o: lib/blob.o lib/disjointset.o src/tracker2.h src/tracker2.cc
	g++ src/tracker2.cc -c -o lib/tracker2.o

lib/disjointset.o: src/disjointset.h src/disjointset.cc
	g++ src/disjointset.cc -c -o lib/disjointset.o

lib/queue.o: lib/blob.o lib/widget.o src/queue.h src/queue.cc src/functors.h
	g++ src/queue.cc -c -o lib/queue.o -I/usr/include/opencv

lib/widget.o: lib/blob.o lib/glhelper.o lib/tracker.o lib/tracker2.o src/widget.h src/widget.cc src/functors.h
	g++ src/widget.cc -c -o lib/widget.o -I/usr/include/opencv

lib/tuio.o: src/tuio.h src/tuio.cc
	g++ src/tuio.cc -c -o lib/tuio.o

clean:
	@rm -f *~ src/*~ lib/* bin/*
