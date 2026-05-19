LLVM_CONFIG=/opt/homebrew/opt/llvm/bin/llvm-config
CFLAGS=-Wall -Wextra -O2 -Iinclude $(shell $(LLVM_CONFIG) --cflags)
LDFLAGS=$(shell $(LLVM_CONFIG) --ldflags)
LIBS=$(shell $(LLVM_CONFIG) --libs core interpreter native)
SRCS=src/main.c src/lexer.c src/parser.c src/codegen.c
TARGET=freq

all: $(TARGET)

$(TARGET): $(SRCS)
	gcc $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean