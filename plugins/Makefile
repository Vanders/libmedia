CXXFLAGS += -I. -I../include -Wall -c

OBJDIR = objs
PLUGINS = file wave
OBJS := $(addprefix $(OBJDIR)/,$(addsuffix .o,$(PLUGINS)))

all: $(OBJDIR) $(PLUGINS)

file: $(OBJDIR)/file.o
	g++ $^ -plugin -Xlinker -Bsymbolic -lsyllable  -L../lib/ -lmedia_ng -o $@

wave: $(OBJDIR)/wave.o
	g++ $^ -plugin -Xlinker -Bsymbolic -lsyllable  -L../lib/ -lmedia_ng -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	-rm $(OBJS)
	-rm $(PLUGINS)

$(OBJDIR)/%.o : %.cpp
	@echo Compiling : $<
	@g++ $(CXXFLAGS) $< -o $@

