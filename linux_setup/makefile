
CC = gcc

TARGET = linux_setup

OBJECTS = fileutils.o writeos.o main.o 

#if 0

#elif 0

#else

#endif

CFLAGS += -g -lstdc++

CPPFLAGS = CFLAGS


all: $(OBJECTS)
	$(CC)  $(CFLAGS) $(OBJECTS) -o $(TARGET)
	-rm -rf $(OBJECTS)

$(TARGET):$(OBJECTS)
	$(CC)  $(CFLAGS) $(OBJECTS) -o $(TARGET)
	-rm -rf $(OBJECTS)

$(OBJECTS) : %.o : %.cpp
	$(CC) -c $< -o $@


.PHONY clean:
	-rm -rf $(OBJECTS) $(TARGET)
