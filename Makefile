buffer: 
	g++ --std=c++11 main.cpp buffer.cpp -o buffer

debug: 
	g++ -g --std=c++11 main.cpp buffer.cpp -o buffer

clean :
	rm buffer
