# PHPOS Compiler Makefile

RM = rm -f
CC = gcc
MKENUM = ../MakeEnum

OBJ  = main.o ast.o data.o helpers.o symbol.o
OBJ += parser/token.o parser/expr.o
OBJ += opt/common.o opt/pass1.o opt/pass2.o
OBJ += output/common.o output/arch/x86.o
DEPFILES  = $(OBJ:%.o=%.d)
BIN = ../cc

CPPFLAGS = -I./include
CFLAGS	= -Wall -Werror $(CPPFLAGS) -g
LDFLAGS = -g

.PHONY:	all

all:	$(BIN)

clean:
	$(RM) $(BIN) $(OBJ) $(DEPFILES)

$(BIN): $(OBJ)
	$(CC) $(LDFLAGS) -o $(BIN) $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
	@$(CC) -M $(CPPFLAGS) -MT $*.o -o $*.d $<

%.enum.h: %.enum
	$(MKENUM) $< > $@

-include $(DEPFILES)
