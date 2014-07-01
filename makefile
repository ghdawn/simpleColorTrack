ObjPath = obj/
ExePath = bin/
CFLAGS = -g  -fPIC  \
		-I../iTRLib/3rdparty/alglib/ \
		-I../iTRLib/itralgorithm/ \
		-I../iTRLib/itrbase/ \
		-I../iTRLib/itrvision/ \
		-I../iTRLib/itrdevice/

Libs= -L/usr/local/lib \
		-L../iTRLib/itralgorithm/bin/Debug \
		-L../iTRLib/itrbase/bin/Debug \
		-L../iTRLib/itrvision/bin/Debug \
		-L../iTRLib/itrdevice/bin/Debug \
		-litrbase -litrvision -litralgorithm -litrdevice


all: main.cpp $(ObjPath)colortrack.o
	mkdir $(ExePath)
	g++ -o $(ExePath)main $(CFLAGS) $(ObjPath)colortrack.o main.cpp 

$(ObjPath)colortrack.o: colortrack.h colortrack.cpp
	mkdir $(ObjPath)
	g++ -c colortrack.cpp $(CFLAGS) -o $(ObjPath)colortrack.o

clean: bin/ obj/
	rm bin/*
	rm obj/*