
GCCFLAGS = -Wall  -ansi -pedantic 
LINKERFLAGS = -lpthread


all:	main.exe 
	
stappConSelect.o: stappConSelect.c
	gcc $(GCCFLAGS) -c  stappConSelect.c 

main.o: main.c stappConSelect.c
	gcc $(GCCFLAGS) -c  main.c 

mezzoCondiviso.o: mezzoCondiviso.c
	gcc $(GCCFLAGS) -c  mezzoCondiviso.c

main.exe: main.o stappConSelect.o mezzoCondiviso.o
	gcc  -o main.exe  main.o stappConSelect.o mezzoCondiviso.o ${LINKERFLAGS}




clean: 
	rm  main.o
	rm  stappConSelect.o
	rm  main.exe
	rm mezzoCondiviso.o

