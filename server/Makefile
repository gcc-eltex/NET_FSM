SRC=$(wildcard src/*.c)
OBJ=$(patsubst src/%.c,%.o,$(SRC))

all: $(OBJ)
	gcc -o ./bin/server $(OBJ) -lpthread -lrt

$(OBJ): $(SRC)
	gcc $(SRC) -c

run:
	./bin/server

clean:
	rm -rf *.o bin/*
