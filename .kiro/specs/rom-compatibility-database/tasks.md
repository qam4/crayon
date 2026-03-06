# Implementation Plan: ROM Compatibility Database

## Overview

Implement a CSV-based ROM compatibility database and Python CLI tool (`tools/crc32_tool.py`) with scan, populate, and validate modes for the Crayon MO5 emulator. Uses only Python stdlib.

## Tasks

- [x] 1. Create data models and CRC32 computation
  - [x] 1.1 Create `tools/crc32_tool.py` with data models and CRC32 functions
    - Define `RomInfo`, `CsvRow`, `ValidationReport` dataclasses
    - Implement `compute_crc32(filepath)` using `zlib.crc32`
    - Implement `compute_crc32_from_bytes(data)`
    - _Requirements: 1.1, 1.2, 3.1_

- [x] 2. Implement scan mode
  - [x] 2.1 Implement `scan_directory(dirpath)` function
    - Recursively scan for `.k7`, `.rom`, `.bin`, `.mo5` files
    - Compute CRC32 for each, return list of `RomInfo`
    - Handle errors: log warnings, skip bad files, continue
    - _Requirements: 3.2, 3.4_

- [x] 3. Implement K7 header parser and name parser
  - [x] 3.1 Implement `detect_load_type(filepath)` function
    - Parse K7 header block: find leader (0x01), sync (0x3C), block type 0x00
    - Read file_type byte: 0x00→LOAD, 0x02→LOADM, else unknown
    - Return "cartridge" for non-K7 files
    - _Requirements: 4.4_
  - [x] 3.2 Implement `parse_game_name(filename)` function
    - Strip extension, take text before first `(`
    - Trim whitespace
    - _Requirements: 4.2_

- [x] 4. Implement populate mode
  - [x] 4.1 Implement populate: scan → parse → deduplicate → write CSV
    - Scan roms directory for all K7 and cartridge files
    - Parse game names from filenames
    - Detect load type for K7 files
    - Deduplicate by CRC32
    - Sort alphabetically by name
    - Write `doc/compatibility.csv`
    - _Requirements: 1.4, 1.5, 2.2, 4.1, 4.2, 4.3, 4.5_

- [x] 5. Implement CSV I/O
  - [x] 5.1 Implement `write_csv(rows, path)` and `load_csv(path)` functions
    - CSV header: `crc32,name,filename,size,type,load_type,status,notes`
    - Handle quoting for commas/quotes in notes
    - _Requirements: 1.2, 1.3_

- [x] 6. Implement validate mode
  - [x] 6.1 Implement `validate(database, scanned)` function
    - Report CRC mismatches, missing files, orphan files
    - Print status summary counts
    - _Requirements: 5.1, 5.2, 5.3_

- [x] 7. Wire up CLI with argparse
  - [x] 7.1 Implement argparse with scan, populate, validate subcommands
    - Exit codes: 0 success, 1 fatal, 2 warnings
    - _Requirements: 3.1, 3.2_

- [x] 8. Run populate on the actual roms collection
  - [x] 8.1 Run `python tools/crc32_tool.py populate` and verify output
    - Check doc/compatibility.csv is generated with all K7/ROM files
    - Verify game names are parsed correctly
    - Verify load types are detected for K7 files
    - Commit the CSV to the repository

- [x] 9. Final checkpoint
  - Ensure tool works end-to-end, ask the user if questions arise.

## Notes

- Uses Python stdlib only (zlib, csv, argparse, dataclasses, pathlib)
- K7 header parsing reuses the same block format as CassetteInterface::parse_k7()
- The roms/ directory contains ~300 K7 files and a handful of cartridge ROMs
