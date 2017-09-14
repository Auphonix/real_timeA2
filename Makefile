CPP=clang
CFLAGS=-framework Carbon -framework OpenGL -framework GLUT -Wno-deprecated

a2make: sine_wave.cpp shaders.c
	$(CPP) -o a2 sine_wave.cpp shaders.c $(CFLAGS)
