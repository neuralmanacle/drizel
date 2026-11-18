"""CLI for Step 14.2 offline training and inference."""
import argparse
import json
from neural import train, predict


def main():
    p = argparse.ArgumentParser(description='Drizel Step 14.2 neural saturation experiment')
    sub = p.add_subparsers(dest='command', required=True)
    t = sub.add_parser('train')
    t.add_argument('manifest')
    t.add_argument('output_dir')
    t.add_argument('--holdout', default='noise')
    t.add_argument('--seed', type=int, default=14)
    t.add_argument('--epochs', type=int, default=60)
    t.add_argument('--batch-size', type=int, default=4096)
    t.add_argument('--lr', type=float, default=0.003)
    t.add_argument('--width', type=int, default=32)
    t.add_argument('--max-samples', type=int, default=120000)
    q = sub.add_parser('predict')
    q.add_argument('checkpoint')
    q.add_argument('input_wav')
    q.add_argument('output_wav')
    a = p.parse_args()
    if a.command == 'train':
        print(json.dumps(train(a.manifest, a.output_dir, holdout=a.holdout,
                               seed=a.seed, epochs=a.epochs, batch_size=a.batch_size,
                               lr=a.lr, width=a.width, max_samples=a.max_samples), indent=2))
    else:
        predict(a.checkpoint, a.input_wav, a.output_wav)
        print('Wrote', a.output_wav)


if __name__ == '__main__':
    main()
