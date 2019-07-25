all : 
	gcc *.c -o router

.phony : clean

clean :
	rm -f router