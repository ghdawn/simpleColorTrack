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
		-L../../iTRLib/itralgorithm/bin/debug \
		-L../../iTRLib/itrbase/bin/debug \
		-L../../iTRLib/itrvision/bin/debug \
		-L../../iTRLib/itrdevice/bin/debug \
		-L../../iTRLib/itrsystem/bin/debug \
		-L../../iTRLib/ix264/bin/debug \
		-litrvision  -litrsystem -litrbase -litrdevice -lix264\
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