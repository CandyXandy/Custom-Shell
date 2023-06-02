nothing:
	@echo "Doing nothing!"

clean:
	rm smsh1 smsh2 smsh3 smsh4

smsh1: execute.c splitline.c smsh1.c
	gcc -o smsh1 execute.c splitline.c smsh1.c

part1: execute.c splitline.c smsh2.c
	gcc -o smsh2 execute.c splitline.c smsh2.c

part2: execute.c splitline.c smsh3.c
	gcc -o smsh3 execute.c splitline.c smsh3.c

part3: execute.c splitline.c smsh4.c
	gcc -o smsh4 execute.c splitline.c smsh4.c

