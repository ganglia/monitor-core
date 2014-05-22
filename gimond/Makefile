CC = gcc
CFLAGS = -pthread -Wall -Wextra -O3
SOURCES = tcpServer.c
OBJECTS = $(SOURCES:.c=.o)
EXEC = tcpServer
PRINT = @echo '$@ done.'
CLEAN = rm -f *.o *~ \#*
CLEANALL = rm -f $(EXEC) *.o *~ \#*

.PHONY: clean cleanall

all: $(SOURCES) $(EXEC)
	$(CLEAN)
	$(PRINT)

# Use make all -e "DFLAGS = -DVERBOSE" to make the server more expressive
$(EXEC): $(OBJECTS)
	$(CC) $(DFLAGS) $(CFLAGS) $(OBJECTS) -o $@
	$(PRINT)
	
clean:
	$(CLEAN)
	$(PRINT)

cleanall:
	$(CLEANALL)
	$(PRINT)

re: cleanall all
	$(PRINT)