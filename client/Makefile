## PPCA 2014 Final project
## Makefile of the Texas Hold'em Client
## You might want to add -O2 to CFLAGS after debug.
## Change BOOST_PATH to the location where boost is located

AI = example

TARGET = client
LIBS = -lpthread -lboost_system
CXX = g++
#CFLAGS = -std=c++11 -Wall -Wextra -g		## use this for debug
CFLAGS = -std=c++11 -Wall -Wextra -O2 -DNDEBUG

HEADERS = client.h common.h IO.h player.h pot.h query.h
OBJECTS = common.o client.o ${AI}.o main.o

.PHONY: all clean
.PRECIOUS: $(TARGET) $(OBJECTS)

all: ${TARGET}

${TARGET}: ${OBJECTS}

client.o: ${HEADERS} client.cpp
	${CXX} ${CFLAGS} -c -o client.o client.cpp

${AI}.o: ${HEADERS} ${AI}.cpp
	${CXX} ${CFLAGS} -c -o ${AI}.o ${AI}.cpp

main.o: client.h main.cpp
	${CXX} ${CFLAGS} -c -o main.o main.cpp

common.o: common.h common.cpp
	${CXX} ${CFLAGS} -c -o common.o common.cpp

${TARGET}: ${OBJECTS}
	${CXX} ${OBJECTS} -Wall ${LIBS} -o ${TARGET} 

clean:
	-rm *.o
	-rm ${TARGET}
