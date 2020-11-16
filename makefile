TARGET=tc ts
LDLIBS+=-pthread

all: $(TARGET)

clean:
	rm -f $(TARGET) *.o
