CXXFLAGS = -I. -g -O2 -Wformat=0
LDFLAGS = -lboost_context

all: test_counter test_divide

test_counter: sim.o test_counter.o
	g++ -o test_counter $^ $(LDFLAGS)

test_divide: sim.o test_divide.o
	g++ -o test_divide $^ $(LDFLAGS)

%.o:	%.c
	g++ $(CFLAGS) -c $^ -o $

clean:
	rm -f test_counter test_divide *.o