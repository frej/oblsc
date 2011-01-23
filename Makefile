# Tools
CC		= gcc

BISON		= bison
FLEX		= flex

# Installation path
PREFIX		?= /usr/local
MANPAGE_DIR	?= $(PREFIX)/share/man/man1/
BIN_DIR		?= $(PREFIX)/bin/

# Compilation flags
VERSION		= 110123.1
PKG_MODULES	= glib-2.0
OPTIMIZE	= -O2
DEFS		= -D_GNU_SOURCE -DVERSION_STRING="\"$(VERSION)\""
LIBS		=
INCLUDES	= $(shell pkg-config --cflags $(PKG_MODULES))
CFLAGS 		= $(INCLUDES) -Wall -pedantic --std=gnu99 $(OPTIMIZE) \
			$(DEFS) -g
LDFLAGS		= $(shell pkg-config --libs $(PKG_MODULES))
BISON_FLAGS	= -Wall -d -v
FLEX_FLAGS	=

# Files
LOAD_MODULE	= oblsc
MAN_PAGES	= oblsc.1
C_FILES         = main.c serial.c cmdline.c sump.c state.c vcd.c	\
		  trigger_parse.c trigger_lex.c trigger.c trigger_type.c
OBJS		= $(C_FILES:.c=.o)

# Helpers
BEAMS		= $(ERLS:.erl=.beam)
ALL_SOURCE	= $(C_FILES)

all: $(LOAD_MODULE) $(MAN_PAGES)

$(LOAD_MODULE): $(OBJS)
	@echo "L " $@
	@$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -rf $(OBJS) *~ $(LOAD_MODULE) $(LOAD_MODULE).elf		\
		$(DEPFILES) trigger_parse.c trigger_parse.h		\
		trigger_parse.output trigger_lex.c trigger_lex.h

oblsc.1: oblsc.1.txt

oblsc.1.html: oblsc.1.txt

distfile:
	git archive --format=tar v$(VERSION) --prefix=oblsc-$(VERSION)/ | \
		gzip > oblsc-$(VERSION).tar.gz

install: $(LOAD_MODULE) $(MAN_PAGES)
	install -d $(MANPAGE_DIR)
	install -d $(BIN_DIR)
	install -m 444 -t $(MANPAGE_DIR) $(MAN_PAGES)
	install -m 555 -t $(BIN_DIR) $(LOAD_MODULE)

%.1: %.1.txt
	a2x -f manpage oblsc.1.txt

%.1.html: %.1.txt
	a2x -f xhtml oblsc.1.txt

%.o : %.c
	@echo "Cc" $<
	@$(CC) $(CFLAGS) -c $< -o $@

%.c: %.y
	@echo "Y " $<
	@$(BISON) $(BISON_FLAGS) $< -o $@

%.c: %.lex
	@echo "Lex " $<
	@$(FLEX) $(FLEX_FLAGS) $<

# Rules for building dependencies

.%.c.dep : %.c
	@echo "Dc" $<
	@$(CC) $(CFLAGS) -M $< > $@

.trigger_lex.c.dep : trigger_lex.c trigger_parse.h
.trigger.c.dep : trigger_lex.h trigger_parse.h

DEPFILES = $(join $(dir $(ALL_SOURCE)), $(addsuffix .dep, $(addprefix	\
                        ., $(notdir $(ALL_SOURCE)))))

-include $(DEPFILES)
