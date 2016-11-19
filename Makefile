TARGET = diary

default: $(TARGET)

$(TARGET):
	gcc diary.c -o $(TARGET) -lncurses

clean:
	rm -f $(TARGET)
