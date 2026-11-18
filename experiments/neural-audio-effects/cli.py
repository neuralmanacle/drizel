"""Run from this directory: python cli.py process|dataset ..."""
import argparse
import json
from pathlib import Path
import numpy as np
from audio_io import read_wav, write_wav
from effect import saturate
from signals import KINDS, generate


def main(argv=None):
    parser = argparse.ArgumentParser(description='Offline Step 14.1 saturation reference')
    sub = parser.add_subparsers(dest='command', required=True)
    process = sub.add_parser('process', help='Process a mono/stereo WAV')
    process.add_argument('input')
    process.add_argument('output')
    dataset = sub.add_parser('dataset', help='Create deterministic dry/wet WAV pairs')
    dataset.add_argument('output_dir')
    dataset.add_argument('--rate', type=int, default=48000)
    dataset.add_argument('--duration', type=float, default=1.0)
    dataset.add_argument('--seed', type=int, default=14)
    dataset.add_argument('--channels', type=int, choices=[1, 2], default=1)
    for p in (process, dataset):
        p.add_argument('--drive', type=float, default=2.0)
        p.add_argument('--mix', type=float, default=1.0)
        p.add_argument('--output-gain', type=float, default=1.0)
    args = parser.parse_args(argv)
    if args.command == 'process':
        x, rate = read_wav(args.input)
        write_wav(args.output, saturate(x, args.drive, args.mix, args.output_gain), rate)
        print(f'Wrote {args.output}: {len(x)} frames, {rate} Hz, {x.shape[1]} channel(s)')
    else:
        out = Path(args.output_dir)
        records = []
        for kind in KINDS:
            x = generate(kind, args.rate, args.duration, args.channels, args.seed)
            y = saturate(x, args.drive, args.mix, args.output_gain)
            dry, wet = out / f'{kind}_dry.wav', out / f'{kind}_wet.wav'
            write_wav(dry, x, args.rate)
            write_wav(wet, y, args.rate)
            records.append(dict(kind=kind, dry=dry.name, wet=wet.name, rate=args.rate, frames=len(x), channels=args.channels, seed=args.seed, drive=args.drive, mix=args.mix, output_gain=args.output_gain))
        (out / 'manifest.json').write_text(json.dumps({'version': 1, 'pairs': records}, indent=2) + '\n')
        print(f'Wrote {len(records)} pairs and manifest to {out}')


if __name__ == '__main__':
    main()
