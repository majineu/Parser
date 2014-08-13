#CC=g++
CC=clang++
#CFLAGS=-c -Wall -g -std=c++11	 
CFLAGS=-c -Wall -O4 -std=c++11 
LDFLAGS= 
SOURCES=Template.cpp State.cpp Train.cpp Eval.cpp Sentence.cpp Parsing.cpp IO.cpp FeatureCollector.cpp DepTree.cpp Config.cpp util.cpp Pool.cpp Scorer.cpp smallTests.cpp SRParser.cpp main.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=Parser

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm *.o Parser
