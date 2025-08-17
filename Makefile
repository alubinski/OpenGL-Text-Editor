app: editor.c data_structure.o data_structure.h 
	$(CC) editor.c data_structure.o -o editor.out -g -Wall -Wextra -pedantic -std=c99 -I/usr/include/freetype2 -lX11 -lGL -lrunara -lfreetype -lharfbuzz -lm 
