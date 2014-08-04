## PPCA 2014 Final project
## Makefile of the Texas Hold'em Server
## You might want to add -O2 to CFLAGS after debug.
## Change BOOST_PATH to the location where boost is located

TARGET = server
LIBS = -lpthread -lboost_system
CXX = g++
#CFLAGS = -std=c++11 -Wall -Wextra -g		## use this for debug
CFLAGS = -std=c++11 -Wall -Wextra -O2 -DNDEBUG

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.cpp, %.o, $(wildcard *.cpp))
HEADERS = $(wildcard *.h)

%.o: %.cpp $(HEADERS)
	$(CXX) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS) $(SUNDOWN)
	$(CXX) $(OBJECTS) -Wall $(LIBS) -o $@ 

clean:
	-rm *.o
	-rm ${TARGET}
