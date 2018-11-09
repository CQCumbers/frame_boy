# Frame Boy
> Simple C++ Game Boy Emulator for WebAssembly

This project replicates the behavior of early revision Game Boy (DMG) hardware in C++. It was intended as a learning experience, and does not intend to set a standard for accuracy, performance, or features. Nevertheless, it supports stereo sound, passes all of blargg's cpu instruction and instruction timing tests, and boots most MBC1 cartridges.

## Resources
- [Gekkio's Docs](https://gekkio.fi/files/gb-docs/gbctr.pdf) & [notes](https://github.com/Gekkio/mooneye-gb/blob/master/docs/accuracy.markdown) where possible
- [AntonioND's Docs](https://github.com/AntonioND/giibiiadvance/blob/master/docs/TCAGBD.pdf) & [Pandocs](http://gbdev.gg8.se/wiki/articles/Pan_Docs) otherwise
- [Gameboy sound hardware](http://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware) for APU info
- [Blargg's test roms](https://github.com/retrio/gb-test-roms) for debugging cpu instructions
- [Sameboy](https://github.com/LIJI32/SameBoy) for comparing memory and VRAM
- [awesome-gbdev](https://github.com/gbdev/awesome-gbdev) includes many other resources

Note that AntonioND's MBC1 description is incorrect (follow Gekkio), and Gameboy sound hardware's length counters are wrong (follow Pandocs).

Also many thanks to *izik1*, *xiphias*, *Thief*, *Mask of Destiny* and others on the Emulation Development Discord for their help.
