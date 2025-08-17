app: editor.c
	$(CC) editor.c -o editor.out -g -Wall -Wextra -pedantic -std=c99 -I/usr/include/freetype2 -lX11 -lGL -lrunara -lfreetype -lharfbuzz -lm 
