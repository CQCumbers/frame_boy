# Compile the main executable
main.exe: memory.cpp cpu.cpp ppu.cpp timer.cpp joypad.cpp gameboy.cpp main.cpp
	g++ -Wall -Werror -Wextra -flto -g --std=c++11 memory.cpp cpu.cpp ppu.cpp timer.cpp joypad.cpp gameboy.cpp main.cpp -o main.exe -O3

main.html: memory.cpp cpu.cpp ppu.cpp timer.cpp joypad.cpp gameboy.cpp main.cpp
	emcc -Wall -Werror -Wextra -flto -g --std=c++11 memory.cpp cpu.cpp ppu.cpp timer.cpp joypad.cpp gameboy.cpp main.cpp -o dist/main.html -O3 --preload-file roms -s WASM=1 -s USE_SDL=2 --emrun

serve: main.html
	emrun --no_browser --port 8080 dist/main.html

# Remove automatically generated files
clean:
	rm -rvf *.exe *~ *.out *.dSYM *.stackdump dist/*
