CC = gcc
CFLAGS = -Wall -Wextra -g -pthread

all: class_server class_client

class_server: class_server.o admin_commands.o client_commands.o
	$(CC) $(CFLAGS) -o class_server class_server.o admin_commands.o client_commands.o

class_server.o: class_server.c admin_commands.h client_commands.h
	$(CC) $(CFLAGS) -c class_server.c

admin_commands.o: admin_commands.c admin_commands.h
	$(CC) $(CFLAGS) -c admin_commands.c

client_commands.o: client_commands.c client_commands.h
	$(CC) $(CFLAGS) -c client_commands.c

class_client: class_client.o
	$(CC) $(CFLAGS) -o class_client class_client.o

class_client.o: class_client.c
	$(CC) $(CFLAGS) -c class_client.c

clean:
	rm -f *.o class_server class_client