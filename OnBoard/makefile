ObjPath = obj/
ExePath = bin/
Inc = 	-I../../iTRLib/3rdparty/alglib/ \
		-I../../iTRLib/itralgorithm/ \
		-I../../iTRLib/itrbase/ \
		-I../../iTRLib/itrvision/ \
		-I../../iTRLib/itrdevice/\
		-I../../iTRLib/itrsystem/\
		-I../../iTRLib/ix264/

Libs= -L/usr/local/lib \
		-L../../iTRLib/itralgorithm/bin/release \
		-L../../iTRLib/itrbase/bin/release \
		-L../../iTRLib/itrvision/bin/release \
		-L../../iTRLib/itrdevice/bin/release \
		-L../../iTRLib/itrsystem/bin/release \
		-L../../iTRLib/ix264/bin/release \
		-litrvision  -litrsystem -litrbase -litrdevice -litralgorithm -lix264\
		-lavcodec -lswscale -lavutil -lx264 -lpthread -lv4l2

OBJS_SERVER=	$(ObjPath)server.o \
				$(ObjPath)colortrack.o $(ObjPath)yuv2hsl.o $(ObjPath)gimbal.o $(ObjPath)lktracking.o


CXXFLAGS=	-c -g -O0 -fPIC -L/usr/local/lib

all: track

$(ObjPath)%.o: %.cpp
	mkdir -p $(ObjPath)
	$(CXX) $(Inc) $(CXXFLAGS)  $< -o $@


track: $(OBJS_SERVER)
	mkdir -p $(ExePath)
	$(CXX) -o $(ExePath)$@ $^ $(Libs) $(Inc)
	
	

.PHONY : clean
clean: bin/ obj/
	-rm bin/*
	-rm obj/*