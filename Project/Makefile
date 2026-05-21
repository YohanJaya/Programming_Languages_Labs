# Makefile for the RPAL interpreter (CS 3513 Programming Project)
# Builds an executable named rpal20.
#
# Usage:
#   make            -> build ./rpal20
#   make clean      -> remove build artifacts

CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra
TARGET   = rpal20

SRCDIR   = src
SOURCES  = rpal20.cpp \
           $(SRCDIR)/lexer.cpp \
           $(SRCDIR)/node.cpp \
           $(SRCDIR)/parser.cpp \
           $(SRCDIR)/standardizer.cpp \
           $(SRCDIR)/cse_machine.cpp

OBJECTS  = $(SOURCES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -I$(SRCDIR) -o $(TARGET) $(OBJECTS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -I$(SRCDIR) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean
