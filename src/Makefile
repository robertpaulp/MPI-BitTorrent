# Compiler and flags
CC = mpicxx
CFLAGS = -pthread -Wall

SRCS = mpitorrent.cpp peer.cpp tracker.cpp

OBJS = $(SRCS:.cpp=.o)

EXEC = mpitorrent

build: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJS) $(EXEC)