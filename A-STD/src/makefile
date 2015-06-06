#CC=g++
CC=clang++
#CFLAGS=-c -Wall -g -std=c++11	 
CFLAGS=-c -Wall -O3 -std=c++11 
LDFLAGS= 
SOURCES=Template.cpp DynamicOracle.cpp Maps.cpp State.cpp Train.cpp Eval.cpp Sentence.cpp Parsing.cpp IO.cpp FeatureCollector.cpp DepTree.cpp Config.cpp util.cpp Pool.cpp SRParser.cpp main.cpp # Scorer.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=Parser

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm *.o Parser
