"""Generate offline grain-start policies and render them with Drizel's grain voice."""

import argparse
import json
import random
import subprocess
import wave
from pathlib import Path

GRAINS_PER_SECOND = 20
STEP_DIRECTIONS = (1, -1, -1)


def trajectories(length, *, low, high, period, step, step_interval, seed, distance_variation=0, timing_variation=0):
    """Yield normalized positions over a bounded region for each pattern."""
    span = high - low
    cyclic = [low + span * ((index / GRAINS_PER_SECOND / period) % 1)
              for index in range(length)]
    stepped = []
    position = 0.5
    rng = random.Random(seed)
    next_move = step_interval
    move = 0
    for index in range(length):
        if index == next_move:
            direction = STEP_DIRECTIONS[move % len(STEP_DIRECTIONS)]
            distance = step * (1 + rng.uniform(-distance_variation, distance_variation))
            position = (position + direction * distance) % 1
            move += 1
            interval = step_interval * (1 + rng.uniform(-timing_variation, timing_variation))
            next_move += max(1, round(interval))
        stepped.append(low + span * position)

    irregular = []
    rng = random.Random(seed)
    position = 0.5
    remaining = 0
    for _ in range(length):
        if remaining == 0:
            position = (position + rng.uniform(-2 * step, 2 * step)) % 1
            remaining = rng.randint(max(1, step_interval // 2), step_interval * 2)
        irregular.append(low + span * position)
        remaining -= 1
    return {"cyclic": cyclic, "stepped": stepped, "irregular": irregular}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("wav", type=Path, help="Source PCM WAV")
    parser.add_argument("renderer", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--region", type=float, nargs=2, default=(0.05, 0.9),
                        metavar=("LOW", "HIGH"), help="Normalized source bounds")
    parser.add_argument("--pattern", choices=("cyclic", "stepped", "irregular"),
                        help="Render one pattern instead of the comparison")
    parser.add_argument("--seconds", type=int, default=6)
    parser.add_argument("--period", type=float, default=2, help="Cyclic scan period in seconds")
    parser.add_argument("--step", type=float, default=0.18, help="Step as a fraction of the region")
    parser.add_argument("--step-interval", type=int, default=5, help="Grains between stepped moves")
    parser.add_argument("--distance-variation", type=float, default=0)
    parser.add_argument("--timing-variation", type=float, default=0)
    parser.add_argument("--seed", type=int, default=12013)
    args = parser.parse_args()
    low, high = args.region
    if (not 0 <= low < high <= 1 or not 1 <= args.seconds <= 60
            or not 0 < args.period <= 60 or not 0 < args.step <= 0.5
            or not 1 <= args.step_interval <= 1200):
        parser.error("Check region bounds, duration, period, step fraction and step interval.")
    if not (0 <= args.distance_variation <= 0.9 and 0 <= args.timing_variation <= 0.9):
        parser.error("Distance and timing variation must be between 0 and 0.9.")
    with wave.open(str(args.wav), "rb") as wav:
        frames = wav.getnframes()
    if frames == 0:
        parser.error("Source WAV is empty.")
    paths = trajectories(args.seconds * GRAINS_PER_SECOND, low=low, high=high,
                         period=args.period, step=args.step, step_interval=args.step_interval,
                         seed=args.seed, distance_variation=args.distance_variation,
                         timing_variation=args.timing_variation)
    if args.pattern:
        paths = {args.pattern: paths[args.pattern]}
    args.output.mkdir(parents=True, exist_ok=True)
    for name, positions in paths.items():
        starts = "".join(f"{position * (frames - 1):.17g}\n" for position in positions)
        subprocess.run([str(args.renderer.resolve()), str(args.wav),
                        str(args.output / f"{name}.wav")],
                       input=starts, text=True, check=True)
    report = {"source": args.wav.name, "source_frames": frames, "region": args.region,
              "grain_ms": 100, "grains_per_second": GRAINS_PER_SECOND,
              "period_seconds": args.period, "step": args.step,
              "step_interval_grains": args.step_interval, "seed": args.seed,
              "normalized_start_positions": paths}
    if args.distance_variation or args.timing_variation:
        report.update(distance_variation=args.distance_variation,
                      timing_variation=args.timing_variation)
    (args.output / "trajectories.json").write_text(json.dumps(report, indent=2) + "\n")


if __name__ == "__main__":
    main()
