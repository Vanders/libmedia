CXXFLAGS += -I. -I../include/ -Wall -c

OBJDIR = objs
OBJS = pipeline buffer stage

LIB = media_ng
VERSION = 0
SONAME = lib$(LIB).so.$(VERSION)

OBJS := $(addprefix $(OBJDIR)/,$(addsuffix .o,$(OBJS)))

all: $(OBJDIR) $(OBJS)
	g++ $(OBJS) -shared -Wl,-soname=$(SONAME) -lsyllable -o $(SONAME)

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	-rm $(OBJS)
	-rm $(SONAME)

install:
	cp -f $(SONAME) /system/libs/
	ln -sf /system/libs/$(SONAME) /system/libs/lib$(LIB).so

$(OBJDIR)/%.o : %.cpp
	@echo Compiling : $<
	@g++ $(CXXFLAGS) $< -o $@

