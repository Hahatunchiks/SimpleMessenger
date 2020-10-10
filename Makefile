all:
	$(CC) server.c -std=c11 -g -O3 -Werror -Wall -Wextra -pthread -pedantic -o server
	$(CC) client.c -std=c11 -g -O3 -Werror -Wall -Wextra -pthread -pedantic -o client

clean:
	rm -f server
	rm -f client
