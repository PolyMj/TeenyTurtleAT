CFLAGS += -I.. -I./include -O3 -Wno-narrowing
CPPFLAGS += $(CFLAGS) --std=c++17
ifeq ($(OS),Windows_NT)
	LDFLAGS += -s -lopengl32 -lgdi32
	RMCMD = del
else
	RMCMD = rm -f
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Darwin)
		LDFLAGS += -framework OpenGL -framework Cocoa
	else ifeq ($(UNAME_S),Linux)
		LDFLAGS += -s -lGLU -lGL -lX11
	endif
endif

all:
	@echo ------ Building ------
	gcc -c tigr.c -o tigr.o $(CFLAGS)
	gcc -c ../teenyat.c -o teenyat.o $(CFLAGS)
	gcc -c draw.cpp -o draw.o $(CFLAGS)
	g++ -c main.cpp -o main.o $(CPPFLAGS)
	g++ tigr.o teenyat.o draw.o main.o -o turtle $(LDFLAGS)
	@echo ------ Cleaning -------
	$(RMCMD) *.o

clean:
	$(RMCMD) turtle *.o
