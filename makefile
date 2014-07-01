
CFLAGS = -g  -fPIC -L/usr/local/lib \
		-I../iTRLib/3rdparty/alglib/ \
		-I../iTRLib/itralgorithm/ \
		-I../iTRLib/itrbase/ \
		-I../iTRLib/itrvision/ \
		-I../iTRLib/itrdevice/

all: main.cpp colortrack.o
	g++ -o main $(CFLAGS) colortrack.o main.cpp 

colortrack.o: colortrack.h colortrack.cpp
	g++ -c colortrack.cpp $(CFLAGS) -o colortrack.o
