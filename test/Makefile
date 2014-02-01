CXXFLAGS += -I. -I../include -Wall -c

EXE = test

OBJDIR = objs
OBJS = test
OBJS := $(addprefix $(OBJDIR)/,$(addsuffix .o,$(OBJS)))

all: $(OBJDIR) $(OBJS)
	g++ $(OBJS) -lsyllable  -L../lib/ -lmedia_ng  -o $(EXE)

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	-rm $(OBJS)
	-rm $(EXE)

$(OBJDIR)/%.o : %.cpp
	@echo Compiling : $<
	@g++ $(CXXFLAGS) $< -o $@

