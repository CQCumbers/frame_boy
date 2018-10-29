# GB-Emu

This project replicates the behavior of Game Boy (DMG) hardware in C++.

## Resources
- [Gekkio's Docs](https://gekkio.fi/files/gb-docs/gbctr.pdf) & [notes](https://github.com/Gekkio/mooneye-gb/blob/master/docs/accuracy.markdown) where possible
- [AntonioND's Docs](https://github.com/AntonioND/giibiiadvance/blob/master/docs/TCAGBD.pdf) & [Pandocs](http://gbdev.gg8.se/wiki/articles/Pan_Docs) otherwise
- [Blargg's test roms](https://github.com/retrio/gb-test-roms) for debugging cpu instructions
- [Sameboy](https://github.com/LIJI32/SameBoy) for comparing memory and VRAM
- [awesome-gbdev](https://github.com/gbdev/awesome-gbdev) includes many other resources

## Debugging Notes
- If cpu-instrs 2 fails, make sure tma is correct register
- If dr-mario shows blank screen, make sure ly is being incremented during V-BLANK
- If dr-mario demo sprites not shown, make sure DMA triggers twice when same value is written twice
- If sprites render incorrectly, make sure tile-x increments correctly & skip condition is correct
