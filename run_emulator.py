#!/usr/bin/env python3
"""
Crayon - Thomson MO5 Emulator Launcher
Unified cross-platform script for running the emulator in different modes.
"""

import argparse
import os
import platform
import subprocess
import sys
from pathlib import Path


def get_executable_path():
    """Determine the emulator executable path based on platform."""
    system = platform.system()

    if system == "Windows":
        paths = [
            Path("build/dev-mingw/crayon.exe"),
            Path("build/dev-win64/Release/crayon.exe"),
        ]
    else:
        paths = [
            Path("build/crayon"),
            Path("build/Release/crayon"),
        ]

    for path in paths:
        if path.exists():
            return str(path)

    print("Error: Could not find emulator executable. Tried:", file=sys.stderr)
    for path in paths:
        print(f"  - {path}", file=sys.stderr)
    sys.exit(1)


def find_rom(name, search_dir="roms"):
    """Find a ROM file by name in the search directory."""
    path = Path(search_dir) / name
    if path.exists():
        return str(path)
    return None


def setup_screenshots_dir():
    """Create screenshots directory if needed."""
    screenshots_dir = Path("screenshots")
    screenshots_dir.mkdir(exist_ok=True)


def run_sdl_mode(exe_path, args, extra_args):
    """Run emulator in SDL mode."""
    print("Running in SDL mode...")
    print()

    cmd = [exe_path]
    cmd.extend(["--basic", args.basic])
    cmd.extend(["--monitor", args.monitor])
    if args.scale:
        cmd.extend(["--scale", str(args.scale)])
    if args.frames:
        cmd.extend(["--frames", str(args.frames)])
    if args.cartridge:
        cmd.extend(["--cartridge", args.cartridge])
    if args.cassette:
        cmd.extend(["--cassette", args.cassette])
    if args.debugger:
        cmd.append("--debugger")
    cmd.extend(extra_args)

    print("Command:", " ".join(cmd))
    subprocess.run(cmd)


def run_headless_mode(exe_path, args, extra_args):
    """Run emulator in headless mode with optional screenshot."""
    print("Running in HEADLESS mode...")
    print()

    setup_screenshots_dir()

    frames = args.frames or 200

    cmd = [exe_path, "--headless"]
    cmd.extend(["--basic", args.basic])
    cmd.extend(["--monitor", args.monitor])
    cmd.extend(["--frames", str(frames)])
    if args.cartridge:
        cmd.extend(["--cartridge", args.cartridge])
    if args.cassette:
        cmd.extend(["--cassette", args.cassette])

    # Auto-screenshot at end of run
    screenshot_path = args.screenshot or f"screenshots/frame_{frames}.png"
    cmd.extend(["--screenshot", screenshot_path])

    print("Command:", " ".join(cmd))
    print("=" * 60)
    result = subprocess.run(cmd)
    print("=" * 60)
    print(f"Emulator exited with code: {result.returncode}")

    if Path(screenshot_path).exists():
        size = Path(screenshot_path).stat().st_size
        print(f"Screenshot saved: {screenshot_path} ({size} bytes)")


def main():
    parser = argparse.ArgumentParser(
        description="Crayon - Thomson MO5 Emulator Launcher",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s                          # SDL mode, boot to BASIC prompt
  %(prog)s sdl                      # SDL mode
  %(prog)s sdl --debugger           # SDL mode with debugger enabled (F5 to toggle)
  %(prog)s headless                 # Headless mode, 200 frames, auto-screenshot
  %(prog)s headless --frames 500    # Headless mode, 500 frames
  %(prog)s sdl --cassette "roms/Yeti (1984) (Loriciels).k7"  # Load a K7 cassette
        """
    )

    parser.add_argument(
        "mode",
        nargs="?",
        default="sdl",
        choices=["sdl", "headless", "hl"],
        help="Emulator mode: sdl (default), headless/hl"
    )

    parser.add_argument(
        "--mode",
        dest="mode_flag",
        default=None,
        choices=["sdl", "headless", "hl"],
        help="Emulator mode (alternative to positional arg)"
    )

    parser.add_argument(
        "--basic",
        default="roms/basic5.rom",
        help="Path to BASIC ROM (default: roms/basic5.rom)"
    )

    parser.add_argument(
        "--monitor",
        default="roms/mo5.rom",
        help="Path to Monitor ROM (default: roms/mo5.rom)"
    )

    parser.add_argument(
        "--cassette",
        default=None,
        help="Path to K7 cassette file"
    )

    parser.add_argument(
        "--cartridge",
        default=None,
        help="Path to cartridge ROM file"
    )

    parser.add_argument(
        "--frames",
        type=int,
        default=None,
        help="Number of frames to run"
    )

    parser.add_argument(
        "--scale",
        type=int,
        default=None,
        choices=[1, 2, 3, 4],
        help="Display scale (1-4, default 2)"
    )

    parser.add_argument(
        "--screenshot",
        default=None,
        help="Path to save screenshot (headless mode)"
    )

    parser.add_argument(
        "--debugger",
        action="store_true",
        help="Enable debugger (F5 to toggle)"
    )

    args, extra_args = parser.parse_known_args()

    # --mode flag overrides positional mode
    if args.mode_flag:
        args.mode = args.mode_flag

    # Validate ROM files exist
    if not Path(args.basic).exists():
        print(f"Error: BASIC ROM not found: {args.basic}", file=sys.stderr)
        sys.exit(1)
    if not Path(args.monitor).exists():
        print(f"Error: Monitor ROM not found: {args.monitor}", file=sys.stderr)
        sys.exit(1)

    exe_path = get_executable_path()

    mode = args.mode.lower()
    if mode == "hl":
        mode = "headless"

    if mode == "headless":
        run_headless_mode(exe_path, args, extra_args)
    else:
        run_sdl_mode(exe_path, args, extra_args)


if __name__ == "__main__":
    main()
