CC = gcc
CFLAGS = -g -Wall -Wextra -pedantic -Winvalid-pch -std=c23 
#-fsanitize=address 
LDFLAGS = -lX11 -lGL -lrunara -lfreetype -lharfbuzz -lm
SRCS = editor.c rope.c memento.c cursor.c
OBJS = $(SRCS:.c=.o)
TARGET = editor.out

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJS) $(TARGET)

.PHONY: all clean
