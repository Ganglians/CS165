COMPILER	= g++
FLAGS	 	= 
LIBRARIES	= -l ssl -l crypto

all: server.c client.c
	$(COMPILER) $(FLAGS) -o server server.c $(LIBRARIES)
	$(COMPILER) $(FLAGS) -o client client.c $(LIBRARIES)
clean:
	rm -f client server
