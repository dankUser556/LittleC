# LittleC
A little C interpreter

  What is contained herein is an implementation of a C interpreter 
that was written by Herb Shildt and listed in his 1989 book,
Born to Code in C. I have attempted to keep the actual code as close 
as possible to his original specification, but certain compromises 
have had to be made considering the passage of time and the 
intention of running this code on a linux system.

  Having originally conceived to work on DOS, it was originally 
written with portability in mind, and it is a great testament to that
ideal that the code only required a few modifications (mostly to the
ascii encoding in order to work on linux. One of those modifications 
was the result of tedious instrumentation and amateurish debugging 
that left the program in a somewhat less than functional state. It 
has been included in this repository as parser.o for posterity.

  There are a few bugs left, of my own, I'm sure and I plan to hunt 
them down in time, once I have accumulated the experience and 
familiarity with C necessary to understand the intricacies of the 
underlying behavior.

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
