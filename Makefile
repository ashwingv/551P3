CC=g++
CFLAGS= -Wall -c -g
INCLUDE=-I/home/scf-22/csci551b/openssl/include
LIB=-L/home/scf-22/csci551b/openssl/lib
OPT=-lcrypto -lsocket -lnsl -lresolv 
LFLAGS= $(OPT) 

all:functions.o ini_parser.o sv_node

sv_node:sv_node.o ini_parser.o functions.o
	$(CC) -o sv_node sv_node.o ini_parser.o functions.o $(LFLAGS)
sv_node.o:sv_node.cc sv_node.h ini_parser.o extern.h common.h
	$(CC) $(CFLAGS) sv_node.cc $(INCLUDE)
functions.o:functions.cc sv_node.h extern.h common.h
	$(CC) $(CFLAGS) functions.cc $(INCLUDE)
ini_parser.o:ini_parser.cc sv_node.h extern.h common.h
	$(CC) $(CFLAGS) ini_parser.cc $(INCLUDE)

clean:
	rm -rf *~ sv_node *.o

