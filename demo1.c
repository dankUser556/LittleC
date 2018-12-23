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
				puts(""); 
			} 
		} 
	} while(i!=0); 
}

sum(int num)
{
	int running_sum;
	running_sum = 0;
	while(num) {
		running_sum = running_sum + num;
		num = num - 1;
	}
	return running_sum;
}

print_alpha()
{
	for(ch = 'A'; ch<='Z' ; ch = ch + 1) {
		putch(ch);
	}
	puts("");
}
