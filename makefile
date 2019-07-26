all : 
	g++ -o router *.cpp

.phony : clean

clean :
	rm -f router
