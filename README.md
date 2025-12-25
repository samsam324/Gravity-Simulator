# Gravity Simulator

A deterministic n-body gravity simulator with a Barnes–Hut quadtree (≈ O(N log N) force approximation), multi-threaded acceleration evaluation, and GPU bloom + trails for high-impact visuals.

## Controls
- Mouse Wheel: Zoom
- Middle Mouse Drag: Pan
- Space: Pause/Resume
- R: Reset (deterministic)
- Up / Down: Increase / Decrease Barnes–Hut theta
- 1 / 2 / 3: Visual quality preset (bloom/trails only)

## Build
```bash
cmake -S . -B build
cmake --build build -j
```

## Threads
By default the simulator uses (hardware threads - 1). Override with:
```bash
GRAVITY_THREADS=16 ./build/gravity_sim
```

### Windows (PowerShell)
```powershell
$env:GRAVITY_THREADS="16"
.\build\gravity_sim.exe
```

