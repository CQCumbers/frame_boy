# cross-platform dependencies
SOURCES = $(filter-out $(wildcard main*.cpp), $(wildcard *.cpp))

# Compile the main executable
frame_boy: $(SOURCES) blip_buf.c main_sdl2.cpp
	g++ -std=c++11 -Wall -Wextra -O3 -fno-rtti -fno-exceptions \
	$(SOURCES) blip_buf.c main_sdl2.cpp -o frame_boy \
	-I/Library/Frameworks/SDL2.framework/Headers -F/Library/Frameworks -framework SDL2

# Compile main executable to wasm
index.html: $(SOURCES) blip_buf.c main_wasm.cpp
	emcc -Wall -Wextra -O3 -fno-rtti -fno-exceptions \
	$(SOURCES) blip_buf.c main_wasm.cpp -o docs/index.html --llvm-lto 1 \
	--emrun --shell-file base.html --pre-js script.js \
	-s ENVIRONMENT='web' -s EXPORTED_FUNCTIONS='["_load", "_save", "_main"]' \
	-s FORCE_FILESYSTEM=1 -s ALLOW_MEMORY_GROWTH=1 -s DISABLE_EXCEPTION_CATCHING=1

# serve wasm executable
serve: index.html
	workbox generateSW workbox-config.js && \
	emrun --no_browser --port 8080 docs/index.html

# Remove automatically generated files
clean:
	rm -rvf frame_boy *~ *.out *.dSYM *.stackdump dist/*

# Run cppcheck static analyzer
check:
	cppcheck --enable=all --inconclusive --std=c++11 .

# Run clang-format
format:
	clang-format -i *.cpp *.h
