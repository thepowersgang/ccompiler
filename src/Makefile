# Acess C Compiler Makefile

RM = rm -f
CC = gcc
MKENUM = ../MakeEnum

OBJ  = main.o ast.o data.o helpers.o symbol.o types.o
OBJ += parser/token.o parser/expr.o parser/errors.o
OBJ += opt/common.o opt/pass1.o opt/pass2.o
OBJ += compile.o irm.o
OBJ += output/common.o output/arch/x86.o
# output/arch/vm16cisc.o
OBJ := $(OBJ:%=obj/%)
DEPFILES  = $(OBJ:%=%.d)
BIN = ../cc

CPPFLAGS = -I./include
CFLAGS	= -Wall -Werror $(CPPFLAGS) -g -std=gnu99
LDFLAGS = -g

.PHONY:	all

all:	$(BIN)

clean:
	$(RM) $(BIN) $(OBJ) $(DEPFILES)

$(BIN): $(OBJ)
	@echo [LINK] $@
	@$(CC) $(LDFLAGS) -o $(BIN) $(OBJ)

obj/%.o: %.c
	@mkdir -p $(dir $@)
	@echo [CC] -o $@
	@$(CC) $(CFLAGS) -c $< -o $@ -MMD -MF $@.d

%.enum.h: %.enum
	$(MKENUM) $< > $@

-include $(DEPFILES)
