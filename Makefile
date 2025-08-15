app: app.c
	$(CC) app.c -o app.out -g -Wall -Wextra -pedantic -std=c99 -lX11
