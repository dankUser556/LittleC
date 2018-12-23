



int i,j;
char ch;
main() {
	int i,j;
	puts("Little C Demo Program");

	print_alpha();
	do {
		puts("enter a number (0 to quit): ");
		i = getnum();
		if(i<0){
			puts("Numbers must be positive, try again");}
		else {
			for(j = 0; j< i; j=j+1){
				print(j);
				print("summed is");
				print(sum(j));
				puts(''); } } } while(i!=0); }
