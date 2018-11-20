# Compile the main executable
main.exe: blip_buf.c memory.cpp cpu.cpp ppu.cpp apu.cpp timer.cpp joypad.cpp gameboy.cpp main.cpp
	g++ -Wall -Werror -Wextra -O3 -flto blip_buf.c memory.cpp cpu.cpp ppu.cpp apu.cpp timer.cpp joypad.cpp gameboy.cpp main.cpp -o main.exe

# Compile main executable to wasm
index.html: blip_buf.c memory.cpp cpu.cpp ppu.cpp apu.cpp timer.cpp joypad.cpp gameboy.cpp main.cpp
	emcc -Wall -Werror -Wextra -O3 -flto -fno-rtti -fno-exceptions \
	blip_buf.c memory.cpp cpu.cpp ppu.cpp apu.cpp timer.cpp joypad.cpp gameboy.cpp main.cpp \
	-o docs/index.html --llvm-lto 3 --emrun --shell-file base.html -s USE_SDL=2 \
	-s EXPORTED_FUNCTIONS='["_load", "_save", "_main"]' -s EXTRA_EXPORTED_RUNTIME_METHODS='["ccall"]' \
	-s FORCE_FILESYSTEM=1 -s ALLOW_MEMORY_GROWTH=1

# serve wasm executable
serve: index.html
	workbox generateSW workbox-config.js && \
	emrun --no_browser --port 8080 docs/index.html

# Remove automatically generated files
clean:
	rm -rvf *.exe *~ *.out *.dSYM *.stackdump dist/*

# Run cppcheck static analyzer
check:
	cppcheck --enable=all --inconclusive --std=c++11 .

# Run clang-format
format:
	clang-format -i *.cpp *.h
