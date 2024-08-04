CC = gcc
CFLAGS = -Wall -fPIC -D_GNU_SOURCE
LDFLAGS = -shared
LIBS = -ldl

TARGET = toralize.so
SRCS = toralize.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)