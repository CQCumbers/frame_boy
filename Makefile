# Compile the main executable
main.exe: main.cpp memory.cpp cpu.cpp ppu.cpp timer.cpp
	g++ -Wall -Werror -g --std=c++11 main.cpp memory.cpp cpu.cpp ppu.cpp timer.cpp -o main.exe -O3

# Remove automatically generated files
clean:
	rm -rvf *.exe *~ *.out *.dSYM *.stackdump
