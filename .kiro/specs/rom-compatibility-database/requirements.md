# Requirements Document

## Introduction

A ROM compatibility database for the Crayon MO5 emulator that catalogs Thomson MO5 K7 cassette files and cartridge ROMs with their names, CRC32 checksums, and emulator support status. The database enables tracking which games work correctly in the emulator, which have issues, and which remain untested. A companion CRC32 tool automates checksum computation from K7/ROM files, enabling the database to be populated and verified from the user's existing collection.

The collection exists in `roms/` containing loose `.k7` cassette files and `.mo5`/`.rom`/`.bin` cartridge files, named with descriptive TOSEC-style filenames (e.g., `Yeti (1984) (Loriciels).k7`).

## Glossary

- **Database**: A structured CSV file stored in the repository that contains ROM entries with metadata and compatibility status
- **ROM_Entry**: A single record in the Database representing one K7 or cartridge file, including its name, CRC32 checksum, and compatibility status
- **CRC32_Tool**: A command-line Python script that computes CRC32 checksums from K7/ROM files
- **CRC32_Checksum**: A 32-bit cyclic redundancy check value represented as an 8-character uppercase hexadecimal string (e.g., "A1B2C3D4"), used to uniquely identify a file's binary content
- **K7_File**: A Thomson cassette tape image file with `.k7` extension, containing BASIC or machine-code programs
- **Cartridge_ROM**: A Thomson cartridge image file with `.rom`, `.bin`, or `.mo5` extension
- **Compatibility_Status**: One of the defined status categories:
  - `perfect` — Runs identically to real hardware. No known issues.
  - `playable` — Fully playable. Minor cosmetic issues may exist.
  - `in_game` — Gets into gameplay but has issues affecting playability.
  - `menu` — Shows title/menu screen but cannot progress into gameplay.
  - `boots` — Shows something on screen but doesn't reach a usable state.
  - `nothing` — Black screen, crash, or immediate hang.
  - `not_tested` — No one has tried this file yet.
- **Load_Type**: How the game is loaded: `LOAD` (BASIC program), `LOADM` (machine code), `cartridge`, or `unknown`

## Requirements

### Requirement 1: Database File Structure

**User Story:** As a developer, I want a structured data file that lists all known K7/ROM files with their metadata, so that I can track emulator compatibility at a glance.

#### Acceptance Criteria

1. THE Database SHALL store each ROM_Entry with: game name, filename, CRC32_Checksum, file size in bytes, Load_Type, Compatibility_Status, and optional notes
2. THE Database SHALL use CSV format with columns: `crc32`, `name`, `filename`, `size`, `type`, `load_type`, `status`, `notes`
3. THE Database SHALL be stored at `doc/compatibility.csv` in the repository
4. THE Database SHALL include entries for all K7 and cartridge files found in the roms directory
5. THE Database SHALL sort entries alphabetically by game name

### Requirement 2: Compatibility Status Tracking

**User Story:** As a developer, I want to record the emulator's support status for each game, so that I can identify which games need work.

#### Acceptance Criteria

1. THE Database SHALL support the Compatibility_Status values: `perfect`, `playable`, `in_game`, `menu`, `boots`, `nothing`, `not_tested`
2. WHEN a new ROM_Entry is added, THE Database SHALL assign `not_tested` as the default status
3. THE Database SHALL allow a free-text notes field per entry to describe specific issues
4. THE Database SHALL track Load_Type as one of: `LOAD`, `LOADM`, `cartridge`, `unknown`

### Requirement 3: CRC32 Computation Tool

**User Story:** As a developer, I want a tool that computes CRC32 checksums from my K7/ROM files, so that I can populate and verify the database.

#### Acceptance Criteria

1. WHEN given a path to a K7 or ROM file, THE CRC32_Tool SHALL compute and output the CRC32_Checksum as an 8-character uppercase hexadecimal string
2. WHEN given a path to a directory, THE CRC32_Tool SHALL compute CRC32 checksums for all `.k7`, `.rom`, `.bin`, and `.mo5` files recursively
3. THE CRC32_Tool SHALL output results in a format that can update the Database
4. IF a file cannot be read, THE CRC32_Tool SHALL print an error and continue processing remaining files

### Requirement 4: Database Population from Collection

**User Story:** As a developer, I want to populate the database from my existing collection, so that I have a complete starting point.

#### Acceptance Criteria

1. WHEN run in populate mode, THE CRC32_Tool SHALL scan the roms directory for all K7 and cartridge files
2. THE CRC32_Tool SHALL derive game names from filenames by stripping the extension and parsing TOSEC-style naming (extracting title, year, publisher)
3. THE CRC32_Tool SHALL detect file type from extension: `.k7` → K7, `.rom`/`.bin`/`.mo5` → cartridge
4. THE CRC32_Tool SHALL detect Load_Type for K7 files by parsing the K7 header block (file_type field: 0x00=BASIC→LOAD, 0x02=machine code→LOADM)
5. THE CRC32_Tool SHALL deduplicate by CRC32 when the same content appears under different filenames

### Requirement 5: Database Validation

**User Story:** As a developer, I want to verify database consistency, so that I can trust the data.

#### Acceptance Criteria

1. WHEN run in validation mode, THE CRC32_Tool SHALL report files whose CRC32 does not match the Database entry
2. THE CRC32_Tool SHALL report Database entries with no corresponding file
3. THE CRC32_Tool SHALL report a summary count by Compatibility_Status
