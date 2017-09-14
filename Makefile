CPP=g++
CFLAGS=-g -lGL -lGLU -lglut -lGLEW `sdl2-config --cflags` `sdl2-config --libs`

a2make: sine_wave.cpp shaders.c
	$(CPP) -o a2 sine_wave.cpp shaders.c $(CFLAGS)
