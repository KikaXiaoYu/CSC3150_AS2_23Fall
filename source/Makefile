CFILES:= $(shell ls|grep .cpp)
PROGS:=$(patsubst %.cpp,%,$(CFILES))

all: $(PROGS)

%:%.cpp
	g++ $< -o $@

clean:$(PROGS)
	rm $(PROGS)
