TARGET = weakshell
BLD = ./builds/
SRC = ./src/
OBJ = ./obj/

pre-build:
	gcc -c $(SRC)utils.c -o $(OBJ)utils.o ;
	gcc -c $(SRC)server.c -o $(OBJ)server.o ;
	gcc -c $(SRC)client.c -o $(OBJ)client.o

build:
	make pre-build ;
	gcc $(SRC)/main.c -o $(BLD)$(TARGET) $(OBJ)utils.o $(OBJ)server.o $(OBJ)client.o

# pre-build:
# 	gcc -Wall -Wextra -Werror -c $(SRC)utils.c -o $(OBJ)utils.o ;
# 	gcc -Wall -Wextra -Werror -c $(SRC)server.c -o $(OBJ)server.o ;
# 	gcc -Wall -Wextra -Werror -c $(SRC)client.c -o $(OBJ)client.o

# build:
# 	make pre-build ;
# 	gcc -Wall -Wextra -Werror $(SRC)/main.c -o $(BLD)$(TARGET) $(OBJ)utils.o $(OBJ)server.o $(OBJ)client.o
