ObjPath = obj/
ExePath = bin/
Mode=debug
Inc = 	-I../../iTRLib/3rdparty/alglib/ \
		-I../../iTRLib/itralgorithm/ \
		-I../../iTRLib/itrbase/ \
		-I../../iTRLib/itrvision/ \
		-I../../iTRLib/itrdevice/\
		-I../../iTRLib/itrsystem/\
		-I../../iTRLib/ix264/

LibPaths= -L/usr/local/lib \
		-L../../iTRLib/itralgorithm/bin/$(Mode) \
		-L../../iTRLib/itrbase/bin/$(Mode) \
		-L../../iTRLib/itrvision/bin/$(Mode) \
		-L../../iTRLib/itrdevice/bin/$(Mode) \
		-L../../iTRLib/itrsystem/bin/$(Mode) \
		-L../../iTRLib/ix264/bin/$(Mode)

Libs=   -litrvision  -litrsystem -litrbase -litrdevice -litralgorithm -lix264\
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
	$(CXX) -o $(ExePath)$@ $^ $(LibPaths) $(Libs) $(Inc)
	
release:	$(OBJS_SERVER)
	Mode=release
	mkdir -p $(ExePath)
	$(CXX) -o $(ExePath)$@ $^ $(LibPaths) $(Libs) $(Inc)

.PHONY : clean
clean: bin/ obj/
	-rm bin/*
	-rm obj/*