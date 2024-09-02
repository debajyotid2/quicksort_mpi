CC=mpicxx
CFLAGS=-Wall -g -Werror

SRC_DIR=src
SRC=$(wildcard $(SRC_DIR)/*.c)

OBJ_DIR=obj
OBJ=$(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC))

BIN=run

all: make_dir $(BIN)

make_dir:
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(SRC_DIR)/%.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN): main.c $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf $(BIN) $(OBJ)
