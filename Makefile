UNAME:=$(shell uname)

ifeq ($(UNAME), Darwin) # MAC MAKE
CPP=clang
CFLAGS=-framework Carbon -framework OpenGL -framework GLUT -Wno-deprecated
else
CPP=g++
CFLAGS=-g -lGL -lGLU -lglut -lGLEW `sdl2-config --cflags` `sdl2-config --libs`
endif
a2make: sine_wave.cpp shaders.c
	$(CPP) -o a2 sine_wave.cpp shaders.c $(CFLAGS)
