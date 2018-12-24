# LittleC
A little C interpreter

  What is contained herein is an implementation of a C interpreter 
that was written by Herb Shildt in his 1989 book Born to Code in C.
I have attempted to keep the actual code as close as possible to
his original specification, but certain compromises have to be made
considering the passage of time.

  Having originally conceived to work on DOS, it was originally 
written with portability in mind, and it is a great testament to that
ideal that the code only required a few modifications to work on 
linux. One of those modifications was the result of tedious 
instrumentation and debugging that left the program in a less than
functional state. It has been included as parser.o for posterity.

  There are a few bugs left, of my own, I'm sure and plan to hunt 
them down in time.

If you wish to try one of the demo files, unpack the files to your
choice of directory, execute,

make

at the prompt, and then execute

./littlec \<filename\>

where \<filename\> is one of the 3 available demo files. You can try
writing your own, but the keyword set is pretty limited. Examine 
parser1.c for more information.

Cheers!
-Dank
