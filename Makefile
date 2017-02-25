all: sample3D

sample3D: Sample_GL3_3D.cpp glad.c
	g++ -o sample3D Sample_GL3_3D.cpp glad.c -lGL -lglfw -ldl

clean:
	rm sample3D
