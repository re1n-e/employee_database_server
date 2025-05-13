TARGET = bin/dbview
SRC = $(wildcard src/*.c)
OBJ = $(patsubst src/%.c, obj/%.o, $(SRC))

.PHONY: run clean default

run: $(TARGET)
	./$(TARGET) -f ./mynewdb.db -a "Raghavendra Singh Bargali,Haldwani Uttrakhand,120"

default: clean $(TARGET)

new: clean $(TARGET)
	./$(TARGET) -n -f ./mynewdb.db

clean:
	rm -f obj/*.o 
	rm -f bin/*
	rm -f *.db

$(TARGET): $(OBJ)
	gcc -o $@ $?

obj/%.o: src/%.c
	gcc -c $< -o $@ -Iinclude
