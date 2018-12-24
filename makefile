littlec : littlec.o parser.o lclib.o
	gcc -o littlec littlec.o parser1.o lclib.o
littlec.o : littlec.c
	gcc -o littlec.o -c littlec.c 
parser.o : parser1.c
	gcc -o parser1.o -c parser1.c
lclib.o : lclib.c
	gcc -o lclib.o -c lclib.c
