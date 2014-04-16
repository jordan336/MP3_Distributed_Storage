compiler=gcc
flags=-g -Wall

OBJS = networking.o run.o file_io.o
OBJ_DIR = obj
OBJ = $(patsubst %,$(OBJ_DIR)/%,$(OBJS))

all: directories store

store: $(OBJ)
	$(compiler) $^ -o $@ -pthread

$(OBJ_DIR)/%.o : %.c networking.h file_io.h
	$(compiler) $(flags) -c $< -o $@

directories:
	@mkdir -p $(OBJ_DIR)

clean:
	rm -rf store $(OBJ_DIR)/*.o

