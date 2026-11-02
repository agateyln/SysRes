CC      = gcc
CFLAGS  = -Wall -Wextra -g -MMD -MP
LDFLAGS =
LDLIBS  =

TARGET  = client
SRC     = client.c sem.c
OBJ     = $(SRC:.c=.o)
DEPS    = $(OBJ:.o=.d)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(DEPS)

fclean: clean
	rm -f $(TARGET)

re: fclean all

-include $(DEPS)

.PHONY: all clean fclean re