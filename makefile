CC=gcc

SRC := j_src
OBJ := j_obj

SOURCES := $(wildcard $(SRC)/*.c)
OBJECTS := $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SOURCES))

jobExecutor: $(OBJECTS)
	$(CC) -g -Wall $^ -o $@

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) -g -I$(SRC) -c $< -o $@

worker: ./w_src/worker.c ./j_src/file.c ./w_src/trie.c ./w_src/postlist.c
	gcc -o worker ./w_src/worker.c ./j_src/file.c ./w_src/trie.c ./w_src/postlist.c

clean:
	@rm -f jobExecutor ./j_obj/*.o core 
	@rm -f worker
	@rm -rf ./log
