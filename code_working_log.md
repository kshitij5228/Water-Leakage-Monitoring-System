# Code Working Log

## Current Focus

- Main firmware file: `water_monitor/water_monitor.ino`
- Project is GitHub-ready with safe secret handling
- Documentation is polished for clean project presentation

## Current Working Behavior

### `water_monitor/water_monitor.ino`

- Reads inlet and outlet flow in real time.
- Controls motor using relay command from Blynk (`V5`).
- Turns motor OFF when leak is detected and keeps safety latch active until reset (`V8`).
- Turns motor OFF when water level sensor indicates tank full.
- Sends flow, TDS, leak status, motor state, and status message to Blynk.
- Runs buzzer alert patterns for leak and water-level events.

## Security Notes

- WiFi and Blynk credentials are stored in `water_monitor/secrets.h`.
- `water_monitor/secrets.h` is git-ignored.
- Public template is available in `water_monitor/secrets.example.h`.

## Documentation Notes

- `README.md` has project overview, setup steps, and GitHub push commands.
- `CONNECTION_STEPS.md` has complete wiring map, safety rules, and test checklist.

## Next Suggested Additions

- Add circuit photo with pin labels.
- Add Blynk dashboard screenshot.
- Add short demo video link.
