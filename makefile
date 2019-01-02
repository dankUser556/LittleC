objects = littlec.o parser1.o lclib.o
sources = littlec.c parser1.c lclib.c

littlec : $(objects)
	cc -o littlec $(objects)
	
$(objects) :

.PHONY : clean
clean:
	rm $(objects) littlec dlittlec
	
debug : $(sources)
	cc -DDEBUG -o dlittlec littlec.c parser1.c lclib.c