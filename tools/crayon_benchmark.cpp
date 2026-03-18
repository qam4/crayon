// Crayon MO5 Emulator — Benchmark Harness
// Usage: crayon_benchmark --basic <rom> --monitor <rom> [options]
//   --frames N       Number of frames to run (default: 1000)
//   --warmup N       Warmup frames before timing (default: 100)
//   --k7 <file>      Load K7 cassette before benchmarking
//   --baseline-fps N Expected FPS for pass/fail check (default: 0 = no check)
//   --check          Exit with error if below baseline
//   --csv            Output CSV line instead of human-readable

#include "emulator_core.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static void usage(const char* prog) {
    std::fprintf(stderr,
        "Usage: %s --basic <rom> --monitor <rom> [options]\n"
        "  --frames N        Frames to benchmark (default: 1000)\n"
        "  --warmup N        Warmup frames (default: 100)\n"
        "  --k7 <file>       Load K7 cassette\n"
        "  --baseline-fps N  Expected FPS for --check\n"
        "  --check           Exit 1 if below baseline\n"
        "  --csv             CSV output\n", prog);
}

int main(int argc, char* argv[]) {
    std::string basic_rom, monitor_rom, k7_path;
    int frames = 1000, warmup = 100;
    double baseline_fps = 0;
    bool check = false, csv = false;

    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--basic") && i + 1 < argc) basic_rom = argv[++i];
        else if (!std::strcmp(argv[i], "--monitor") && i + 1 < argc) monitor_rom = argv[++i];
        else if (!std::strcmp(argv[i], "--k7") && i + 1 < argc) k7_path = argv[++i];
        else if (!std::strcmp(argv[i], "--frames") && i + 1 < argc) frames = std::atoi(argv[++i]);
        else if (!std::strcmp(argv[i], "--warmup") && i + 1 < argc) warmup = std::atoi(argv[++i]);
        else if (!std::strcmp(argv[i], "--baseline-fps") && i + 1 < argc) baseline_fps = std::atof(argv[++i]);
        else if (!std::strcmp(argv[i], "--check")) check = true;
        else if (!std::strcmp(argv[i], "--csv")) csv = true;
        else { usage(argv[0]); return 1; }
    }

    if (basic_rom.empty() || monitor_rom.empty()) { usage(argv[0]); return 1; }

    crayon::Configuration config;
    crayon::EmulatorCore emu(config);

    auto r1 = emu.load_basic_rom(basic_rom);
    if (r1.is_err()) { std::fprintf(stderr, "Error: %s\n", r1.error.c_str()); return 1; }
    auto r2 = emu.load_monitor_rom(monitor_rom);
    if (r2.is_err()) { std::fprintf(stderr, "Error: %s\n", r2.error.c_str()); return 1; }

    if (!k7_path.empty()) {
        auto r3 = emu.get_cassette().load_k7(k7_path);
        if (r3.is_err()) { std::fprintf(stderr, "K7 error: %s\n", r3.error.c_str()); return 1; }
        emu.play_cassette();
    }

    emu.reset();

    // Warmup
    for (int i = 0; i < warmup; ++i)
        emu.run_frame();

    // Benchmark
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < frames; ++i)
        emu.run_frame();
    auto t1 = std::chrono::high_resolution_clock::now();

    double elapsed_s = std::chrono::duration<double>(t1 - t0).count();
    double fps = frames / elapsed_s;
    double ms_per_frame = (elapsed_s / frames) * 1000.0;
    double speedup = fps / 50.0;  // MO5 runs at 50 Hz

    if (csv) {
        std::printf("frames,warmup,elapsed_s,fps,ms_per_frame,speedup_x\n");
        std::printf("%d,%d,%.4f,%.1f,%.3f,%.1fx\n", frames, warmup, elapsed_s, fps, ms_per_frame, speedup);
    } else {
        std::printf("Crayon MO5 Benchmark\n");
        std::printf("  Frames:       %d (warmup: %d)\n", frames, warmup);
        std::printf("  Elapsed:      %.3f s\n", elapsed_s);
        std::printf("  FPS:          %.1f\n", fps);
        std::printf("  ms/frame:     %.3f\n", ms_per_frame);
        std::printf("  Speedup:      %.1fx real-time\n", speedup);
        if (baseline_fps > 0)
            std::printf("  Baseline:     %.0f FPS (%s)\n", baseline_fps,
                        fps >= baseline_fps ? "PASS" : "FAIL");
    }

    if (check && baseline_fps > 0 && fps < baseline_fps) {
        std::fprintf(stderr, "FAIL: %.1f FPS < %.0f baseline\n", fps, baseline_fps);
        return 1;
    }
    return 0;
}
