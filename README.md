# GB-Emu

This project replicates the behavior of Game Boy (DMG) hardware in C++.

## Debugging Notes
- If cpu-instrs 2 fails, make sure tma is correct register
- If dr-mario shows blank screen, make sure ly is being incremented during V-BLANK
- If dr-mario demo sprites not shown, make sure DMA triggers twice when same value is written twice
- If sprites render incorrectly, make sure tile-x increments correctly & skip condition is correct
