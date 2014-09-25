
all: CDStacker

CDStacker: CDStacker.cpp
	$(CXX) -O2 -o $@ $^ -lstdc++ -lboost_program_options -lboost_regex

clean:
	-rm -f CDStacker




