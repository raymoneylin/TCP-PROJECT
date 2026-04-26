CC=gcc
#Conventional variable name for C compiler
CFLAGS=-Wall -Wextra
# -Wall -enable all common warnings. Catches: unused variables, implicit function
#declarations, missing return statements, comparisons b/w signed/unsigned
# -Wextra - Enable extra warnings beyond -Wall. Catches: unused parameters, missing field initializers,etc
#These don't change the compiled output - they just make the compiler tell you about potential problems

all: server client
# all - is the default target. When you type 'make' with no arguments, make runs
#this target. It depends on server and client, so both get built. It has no recipe
# of its own - just dependencies.

server: server.c
	$(CC) $(CFLAGS) server.c -o server

client: client.c
	$(CC) $(CFLAGS) client.c -o client

clean:
	rm -f server client