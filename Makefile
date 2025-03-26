TARGET = ./builds/weakshell
SRC = ./src/main.c ./src/flags.h

build:
	gcc -Wall -Wextra -Werror $(SRC) -o $(TARGET)

daemon:
	$(TARGET) -s -d

server:
	$(TARGET) -s

client:
	$(TARGET) -c
