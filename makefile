
SRCDIR = src
OBJDIR = obj
BINDIR = bin

SRC = $(shell find $(SRCDIR) | grep .c)
OBJ = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRC))
PROGS = $(patsubst $(SRCDIR)/%.c, $(BINDIR)/%, $(SRC))

.PHONEY: all clean init

all: clean init $(PROGS)
clean:
	rm -rfd $(OBJDIR) $(BINDIR)

init:
	mkdir $(OBJDIR) $(BINDIR)
	$(foreach DIR, $(patsubst $(SRCDIR)/%, $(OBJDIR)/%, $(shell find $(SRCDIR)/* -type d)), mkdir $(DIR))

$(APPNAME): init $(OBJ)
	gcc -o $(BINDIR)/$(APPNAME) $(OBJ)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	gcc -c $< -o $@

$(BINDIR)/%: $(OBJDIR)/%.o
	gcc -o $@ $<