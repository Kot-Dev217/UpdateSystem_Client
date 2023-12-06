CC = gcc
CFLAGS = -Wall -Wextra
LIBS = -lcurl -lzip
NAME = patcher
FILES = patcher.c

$(NAME): $(FILES)
	$(CC) $(CFLAGS) -o $(NAME) $(FILES) $(LIBS)

clean:
	rm -f $(NAME)

