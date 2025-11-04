TARGET = weakshell
BLD = ./builds/
SRC = ./src/
OBJ = ./obj/

pre-build:
	gcc -c $(SRC)utils.c -o $(OBJ)utils.o ;
	gcc -c $(SRC)server.c -o $(OBJ)server.o ;
	gcc -c $(SRC)client.c -o $(OBJ)client.o ;
	gcc -c $(SRC)local.c -o $(OBJ)local.o

build:
	make pre-build ;
	gcc $(SRC)/main.c -o $(BLD)$(TARGET) $(OBJ)utils.o $(OBJ)local.o $(OBJ)server.o $(OBJ)client.o
