objects = littlec.o parser1.o lclib.o

littlec : $(objects)
	cc -o littlec $(objects)
$(objects) :
clean:
	rm $(objects) littlec