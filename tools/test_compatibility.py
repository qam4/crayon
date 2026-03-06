#!/usr/bin/env python3
"""
Crayon MO5 ROM Compatibility Test Tool

Computes CRC32 checksums, populates the compatibility database,
validates it, and automatically tests K7/ROM files in headless mode.

Usage:
    python tools/test_compatibility.py scan <path>
    python tools/test_compatibility.py populate [--roms-dir roms] [--output doc/compatibility.csv]
    python tools/test_compatibility.py validate [--database doc/compatibility.csv] [--roms-dir roms]
    python tools/test_compatibility.py test [--database doc/compatibility.csv] [--roms-dir roms] [--limit N]
"""

import argparse
import csv
import os
import re
import subprocess
import sys
import zlib
from dataclasses import dataclass, field
from pathlib import Path


# ---------------------------------------------------------------------------
# Data models
# ---------------------------------------------------------------------------

@dataclass
class RomInfo:
    filename: str
    crc32: str
    size: int
    file_type: str      # "k7" or "cartridge"
    load_type: str      # "LOAD", "LOADM", "cartridge", "unknown"


@dataclass
class CsvRow:
    crc32: str
    name: str
    filename: str
    size: int
    type: str           # "k7" or "cartridge"
    load_type: str
    status: str = "not_tested"
    notes: str = ""


@dataclass
class ValidationReport:
    crc_mismatches: list = field(default_factory=list)
    missing_roms: list = field(default_factory=list)
    orphan_roms: list = field(default_factory=list)
    status_counts: dict = field(default_factory=dict)


# ---------------------------------------------------------------------------
# CRC32 computation
# ---------------------------------------------------------------------------

def compute_crc32(filepath: str) -> str:
    crc = 0
    with open(filepath, "rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            crc = zlib.crc32(chunk, crc)
    return f"{crc & 0xFFFFFFFF:08X}"


def compute_crc32_from_bytes(data: bytes) -> str:
    return f"{zlib.crc32(data) & 0xFFFFFFFF:08X}"


# ---------------------------------------------------------------------------
# Directory scanning
# ---------------------------------------------------------------------------

ROM_EXTENSIONS = {".k7", ".rom", ".bin", ".mo5"}


def scan_directory(dirpath: str) -> list:
    results = []
    dirpath = Path(dirpath)
    if not dirpath.is_dir():
        print(f"Warning: Not a directory: {dirpath}", file=sys.stderr)
        return results
    for root, _dirs, files in os.walk(dirpath):
        for fname in sorted(files):
            ext = Path(fname).suffix.lower()
            if ext not in ROM_EXTENSIONS:
                continue
            fpath = os.path.join(root, fname)
            try:
                crc = compute_crc32(fpath)
                size = os.path.getsize(fpath)
                file_type = "k7" if ext == ".k7" else "cartridge"
                load_type = detect_load_type(fpath) if ext == ".k7" else "cartridge"
                results.append(RomInfo(
                    filename=fname, crc32=crc, size=size,
                    file_type=file_type, load_type=load_type
                ))
            except (OSError, IOError) as e:
                print(f"Warning: Cannot read {fpath}: {e}", file=sys.stderr)
    return results


# ---------------------------------------------------------------------------
# K7 header parser
# ---------------------------------------------------------------------------

def detect_load_type(filepath: str) -> str:
    try:
        with open(filepath, "rb") as f:
            data = f.read()
    except (OSError, IOError):
        return "unknown"

    pos = 0
    size = len(data)
    while pos < size:
        while pos < size and data[pos] == 0x01:
            pos += 1
        if pos >= size:
            break
        if data[pos] != 0x3C:
            pos += 1
            continue
        pos += 1
        if pos < size and data[pos] == 0x5A:
            pos += 1
        if pos + 2 > size:
            break
        block_type = data[pos]; pos += 1
        length = data[pos]; pos += 1
        if block_type == 0x00 and length >= 11 and pos + length <= size:
            ft_byte = data[pos + 8]
            if ft_byte == ord('B') and pos + 10 < size:
                ft_str = chr(data[pos + 8]) + chr(data[pos + 9]) + chr(data[pos + 10])
                if ft_str == "BAS":
                    return "LOAD"
                elif ft_str == "BIN":
                    return "LOADM"
                else:
                    return "LOAD"
            elif ft_byte == 0x00:
                return "LOAD"
            elif ft_byte == 0x02:
                return "LOADM"
            else:
                return "unknown"
        pos += length + 1
        if block_type == 0xFF:
            break
    return "unknown"


# ---------------------------------------------------------------------------
# Name parser
# ---------------------------------------------------------------------------

def parse_game_name(filename: str) -> str:
    stem = Path(filename).stem
    match = re.match(r'^(.*?)\s*\(', stem)
    if match:
        name = match.group(1).strip()
        if name:
            return name
    return stem.strip()


# ---------------------------------------------------------------------------
# CSV I/O
# ---------------------------------------------------------------------------

CSV_HEADER = ["crc32", "name", "filename", "size", "type", "load_type", "status", "notes"]


def write_csv(rows: list, output_path: str) -> None:
    rows.sort(key=lambda r: r.name.lower())
    os.makedirs(os.path.dirname(output_path) or ".", exist_ok=True)
    with open(output_path, "w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(CSV_HEADER)
        for row in rows:
            writer.writerow([
                row.crc32, row.name, row.filename, row.size,
                row.type, row.load_type, row.status, row.notes
            ])


def load_csv(filepath: str) -> list:
    rows = []
    with open(filepath, "r", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        for r in reader:
            try:
                rows.append(CsvRow(
                    crc32=r.get("crc32", ""),
                    name=r.get("name", ""),
                    filename=r.get("filename", ""),
                    size=int(r.get("size", 0)),
                    type=r.get("type", ""),
                    load_type=r.get("load_type", ""),
                    status=r.get("status", "not_tested"),
                    notes=r.get("notes", ""),
                ))
            except (ValueError, KeyError) as e:
                print(f"Warning: Malformed CSV row: {e}", file=sys.stderr)
    return rows


# ---------------------------------------------------------------------------
# Populate mode
# ---------------------------------------------------------------------------

def populate(roms_dir: str, output_path: str) -> int:
    scanned = scan_directory(roms_dir)
    if not scanned:
        print("No ROM files found.", file=sys.stderr)
        return 1

    seen_crc = {}
    unique = []
    for rom in scanned:
        if rom.crc32 not in seen_crc:
            seen_crc[rom.crc32] = rom
            unique.append(rom)

    rows = []
    for rom in unique:
        name = parse_game_name(rom.filename)
        rows.append(CsvRow(
            crc32=rom.crc32, name=name, filename=rom.filename,
            size=rom.size, type=rom.file_type, load_type=rom.load_type,
        ))

    if os.path.exists(output_path):
        existing = load_csv(output_path)
        existing_by_crc = {r.crc32: r for r in existing}
        for row in rows:
            if row.crc32 in existing_by_crc:
                old = existing_by_crc[row.crc32]
                row.status = old.status
                row.notes = old.notes

    write_csv(rows, output_path)
    k7_count = sum(1 for r in rows if r.type == "k7")
    cart_count = sum(1 for r in rows if r.type == "cartridge")
    load_count = sum(1 for r in rows if r.load_type == "LOAD")
    loadm_count = sum(1 for r in rows if r.load_type == "LOADM")
    print(f"Wrote {len(rows)} entries to {output_path}")
    print(f"  K7: {k7_count} (LOAD: {load_count}, LOADM: {loadm_count}), Cartridge: {cart_count}")
    return 0


# ---------------------------------------------------------------------------
# Validate mode
# ---------------------------------------------------------------------------

def validate(database_path: str, roms_dir: str) -> int:
    if not os.path.exists(database_path):
        print(f"Error: Database not found: {database_path}", file=sys.stderr)
        return 1

    db = load_csv(database_path)
    scanned = scan_directory(roms_dir)
    scanned_by_filename = {r.filename: r for r in scanned}
    db_by_filename = {r.filename: r for r in db}
    report = ValidationReport()

    for entry in db:
        if entry.filename in scanned_by_filename:
            actual = scanned_by_filename[entry.filename]
            if entry.crc32 and entry.crc32 != actual.crc32:
                report.crc_mismatches.append((entry.filename, entry.crc32, actual.crc32))
        else:
            report.missing_roms.append(entry.filename)

    for rom in scanned:
        if rom.filename not in db_by_filename:
            report.orphan_roms.append((rom.filename, rom.crc32))

    for entry in db:
        report.status_counts[entry.status] = report.status_counts.get(entry.status, 0) + 1

    warnings = False
    if report.crc_mismatches:
        warnings = True
        print(f"\nCRC32 mismatches ({len(report.crc_mismatches)}):")
        for fname, expected, actual in report.crc_mismatches:
            print(f"  {fname}: expected {expected}, got {actual}")
    if report.missing_roms:
        warnings = True
        print(f"\nMissing ROM files ({len(report.missing_roms)}):")
        for fname in report.missing_roms:
            print(f"  {fname}")
    if report.orphan_roms:
        warnings = True
        print(f"\nOrphan ROMs not in database ({len(report.orphan_roms)}):")
        for fname, crc in report.orphan_roms:
            print(f"  {fname} ({crc})")

    print(f"\nStatus summary ({len(db)} entries):")
    for status in ["perfect", "playable", "in_game", "menu", "boots", "nothing", "not_tested"]:
        count = report.status_counts.get(status, 0)
        if count > 0:
            print(f"  {status}: {count}")
    return 2 if warnings else 0


# ---------------------------------------------------------------------------
# Test mode — automated headless testing
# ---------------------------------------------------------------------------

def get_exe_path() -> str:
    """Find the emulator executable."""
    candidates = [
        "build/dev-mingw/crayon.exe",
        "build/Release/crayon.exe",
        "build/crayon",
    ]
    for p in candidates:
        if os.path.exists(p):
            return p
    print("Error: Cannot find crayon executable", file=sys.stderr)
    sys.exit(1)


def read_screenshot_bytes(path: str) -> bytes:
    """Read a PNG file as raw bytes for comparison."""
    try:
        with open(path, "rb") as f:
            return f.read()
    except (OSError, IOError):
        return b""


def test_single_rom(exe: str, rom_path: str, load_type: str, file_type: str,
                    boot_frames: int = 80, load_frames: int = 400) -> tuple:
    """
    Test a single ROM in headless mode.
    Returns (status, notes) where status is one of the compatibility values.

    Strategy:
    1. Take a "boot" screenshot (BASIC prompt, before loading)
    2. Type LOAD"" or LOADM"",,R, wait for loading + execution
    3. Take a "game" screenshot
    4. Compare: if screenshots differ, the game at least boots
    5. Check exit code for crashes
    """
    os.makedirs("screenshots/compat_test", exist_ok=True)

    # For cartridges, just boot and screenshot — no typing needed
    if file_type == "cartridge":
        screenshot = "screenshots/compat_test/game.png"
        cmd = [
            exe, "--headless",
            "--basic", "roms/basic5.rom", "--monitor", "roms/mo5.rom",
            "--cartridge", rom_path,
            "--frames", str(boot_frames + 200),
            "--screenshot", screenshot,
        ]
        try:
            result = subprocess.run(cmd, capture_output=True, timeout=30)
        except subprocess.TimeoutExpired:
            return ("nothing", "Timeout — emulator hung")
        except Exception as e:
            return ("nothing", f"Error: {e}")

        if result.returncode != 0:
            return ("nothing", f"Exit code {result.returncode}")

        game_bytes = read_screenshot_bytes(screenshot)
        if len(game_bytes) < 100:
            return ("nothing", "No screenshot produced")
        return ("boots", "Cartridge boots (auto-tested)")

    # K7 files: boot, type LOAD/LOADM, take screenshots
    # First, get a reference "BASIC prompt" screenshot
    ref_screenshot = "screenshots/compat_test/ref_basic.png"
    if not os.path.exists(ref_screenshot):
        cmd = [
            exe, "--headless",
            "--basic", "roms/basic5.rom", "--monitor", "roms/mo5.rom",
            "--frames", str(boot_frames + 20),
            "--screenshot", ref_screenshot,
        ]
        subprocess.run(cmd, capture_output=True, timeout=15)

    ref_bytes = read_screenshot_bytes(ref_screenshot)

    # Build type command based on load type
    type_file = "screenshots/compat_test/type_cmd.txt"
    if load_type == "LOADM":
        with open(type_file, "w") as f:
            f.write('LOADM"",,R\n')
    else:
        with open(type_file, "w") as f:
            f.write('LOAD""\n\\w200RUN\n')

    game_screenshot = "screenshots/compat_test/game.png"
    cmd = [
        exe, "--headless",
        "--basic", "roms/basic5.rom", "--monitor", "roms/mo5.rom",
        "--cassette", rom_path,
        "--k7-mode", "fast",
        "--type-file", type_file,
        "--type-delay", str(boot_frames),
        "--frames", str(load_frames),
        "--screenshot", game_screenshot,
    ]

    try:
        result = subprocess.run(cmd, capture_output=True, timeout=30)
    except subprocess.TimeoutExpired:
        return ("nothing", "Timeout — emulator hung")
    except Exception as e:
        return ("nothing", f"Error: {e}")

    if result.returncode != 0:
        return ("nothing", f"Exit code {result.returncode}")

    game_bytes = read_screenshot_bytes(game_screenshot)
    if len(game_bytes) < 100:
        return ("nothing", "No screenshot produced")

    # Compare with BASIC prompt reference
    if ref_bytes and game_bytes != ref_bytes:
        return ("boots", "Screen differs from BASIC prompt (auto-tested)")
    else:
        return ("nothing", "Screen unchanged from BASIC prompt (auto-tested)")


def test_roms(database_path: str, roms_dir: str, limit: int = 0,
              filter_status: str = "not_tested") -> int:
    """Run automated tests on ROMs in the database."""
    if not os.path.exists(database_path):
        print(f"Error: Database not found: {database_path}", file=sys.stderr)
        return 1

    exe = get_exe_path()
    db = load_csv(database_path)

    # Filter to entries matching the requested status
    to_test = [r for r in db if r.status == filter_status]
    if limit > 0:
        to_test = to_test[:limit]

    if not to_test:
        print(f"No entries with status '{filter_status}' to test.")
        return 0

    print(f"Testing {len(to_test)} ROMs (status={filter_status})...")
    print()

    tested = 0
    results = {"boots": 0, "nothing": 0}

    for entry in to_test:
        rom_path = os.path.join(roms_dir, entry.filename)
        if not os.path.exists(rom_path):
            print(f"  SKIP {entry.name} — file not found")
            continue

        status, notes = test_single_rom(exe, rom_path, entry.load_type, entry.type)
        entry.status = status
        entry.notes = notes
        tested += 1
        results[status] = results.get(status, 0) + 1

        icon = "✓" if status == "boots" else "✗"
        print(f"  {icon} {entry.name}: {status}")

    # Save updated database
    # Merge results back into full DB
    tested_by_crc = {r.crc32: r for r in to_test}
    for row in db:
        if row.crc32 in tested_by_crc:
            updated = tested_by_crc[row.crc32]
            row.status = updated.status
            row.notes = updated.notes

    write_csv(db, database_path)

    print(f"\nTested {tested} ROMs:")
    for status, count in sorted(results.items()):
        if count > 0:
            print(f"  {status}: {count}")
    print(f"Database updated: {database_path}")
    return 0


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Crayon MO5 ROM Compatibility Test Tool")
    sub = parser.add_subparsers(dest="command", required=True)

    # scan
    scan_p = sub.add_parser("scan", help="Compute CRC32 for ROM files")
    scan_p.add_argument("path", help="File or directory to scan")

    # populate
    pop_p = sub.add_parser("populate", help="Generate compatibility.csv")
    pop_p.add_argument("--roms-dir", default="roms", help="ROM directory")
    pop_p.add_argument("--output", default="doc/compatibility.csv", help="Output CSV")

    # validate
    val_p = sub.add_parser("validate", help="Validate database against ROMs")
    val_p.add_argument("--database", default="doc/compatibility.csv", help="CSV path")
    val_p.add_argument("--roms-dir", default="roms", help="ROM directory")

    # test
    test_p = sub.add_parser("test", help="Automated headless testing of ROMs")
    test_p.add_argument("--database", default="doc/compatibility.csv", help="CSV path")
    test_p.add_argument("--roms-dir", default="roms", help="ROM directory")
    test_p.add_argument("--limit", type=int, default=0, help="Max ROMs to test (0=all)")
    test_p.add_argument("--status", default="not_tested", help="Only test entries with this status")

    args = parser.parse_args()

    if args.command == "scan":
        path = Path(args.path)
        if path.is_file():
            crc = compute_crc32(str(path))
            print(f"{path.name}\t{crc}\t{path.stat().st_size}")
        elif path.is_dir():
            for rom in scan_directory(str(path)):
                print(f"{rom.filename}\t{rom.crc32}\t{rom.size}\t{rom.file_type}\t{rom.load_type}")
        else:
            print(f"Error: {path} not found", file=sys.stderr)
            sys.exit(1)

    elif args.command == "populate":
        sys.exit(populate(args.roms_dir, args.output))

    elif args.command == "validate":
        sys.exit(validate(args.database, args.roms_dir))

    elif args.command == "test":
        sys.exit(test_roms(args.database, args.roms_dir, args.limit, args.status))


if __name__ == "__main__":
    main()
