"""Optional PyTorch memoryless waveform regressor; no JUCE integration."""
import json
import random
from pathlib import Path
import numpy as np
from audio_io import read_wav, write_wav


def load_pairs(manifest):
    path = Path(manifest)
    data = json.loads(path.read_text())
    if not isinstance(data.get('pairs'), list) or not data['pairs']:
        raise ValueError('manifest requires nonempty pairs')
    pairs = []
    for record in data['pairs']:
        if not all(k in record for k in ('dry', 'wet', 'kind')):
            raise ValueError('missing pair fields')
        dry_path, wet_path = (path.parent / record[k] for k in ('dry', 'wet'))
        x, sr = read_wav(dry_path)
        y, target_sr = read_wav(wet_path)
        if sr != target_sr or x.shape != y.shape:
            raise ValueError('mismatched paired audio')
        pairs.append((str(record['kind']), x, y, sr))
    return pairs


def split_pairs(pairs, holdout='noise'):
    train = [p for p in pairs if p[0] != holdout]
    test = [p for p in pairs if p[0] == holdout]
    if not train or not test:
        raise ValueError('holdout kind must exist and leave training data')
    return train, test


def metrics(reference, prediction):
    ref = np.asarray(reference, dtype=np.float64)
    pred = np.asarray(prediction, dtype=np.float64)
    if ref.shape != pred.shape or not ref.size or not np.all(np.isfinite(pred)):
        raise ValueError('invalid prediction')
    err = pred - ref
    mse = float(np.mean(err ** 2))
    power = float(np.mean(ref ** 2))
    return {'mse': mse, 'mae': float(np.mean(np.abs(err))),
            'snr_db': float(10 * np.log10((power + 1e-12) / (mse + 1e-12)))}


def _torch():
    try:
        import torch
    except ImportError as exc:
        raise RuntimeError('Install PyTorch: pip install -r requirements-ml.txt') from exc
    return torch


def build_model(torch, width=32):
    if width < 1:
        raise ValueError('width must be positive')
    return torch.nn.Sequential(torch.nn.Linear(1, width), torch.nn.Tanh(),
                               torch.nn.Linear(width, width), torch.nn.Tanh(),
                               torch.nn.Linear(width, 1))


def train(manifest, output_dir, *, holdout='noise', seed=14, epochs=60,
          batch_size=4096, lr=0.003, width=32, max_samples=120000):
    if epochs < 1 or batch_size < 1 or lr <= 0 or max_samples < 1 or width < 1:
        raise ValueError('invalid training parameters')
    torch = _torch()
    torch.set_num_threads(1)
    torch.manual_seed(seed)
    np.random.seed(seed)
    random.seed(seed)
    pairs = load_pairs(manifest)
    training, held_out = split_pairs(pairs, holdout)
    x = np.concatenate([p[1].reshape(-1) for p in training])
    y = np.concatenate([p[2].reshape(-1) for p in training])
    rng = np.random.default_rng(seed)
    if len(x) > max_samples:
        idx = rng.choice(len(x), size=max_samples, replace=False)
        x, y = x[idx], y[idx]
    model = build_model(torch, width)
    opt = torch.optim.Adam(model.parameters(), lr=lr)
    inputs = torch.from_numpy(x.astype('float32')).reshape(-1, 1)
    targets = torch.from_numpy(y.astype('float32')).reshape(-1, 1)
    history = []
    for epoch in range(epochs):
        perm = torch.randperm(len(inputs))
        model.train()
        for ids in perm.split(batch_size):
            opt.zero_grad()
            loss = torch.nn.functional.mse_loss(model(inputs[ids]), targets[ids])
            loss.backward()
            opt.step()
        model.eval()
        with torch.no_grad():
            loss = float(torch.nn.functional.mse_loss(model(inputs), targets))
        history.append(loss)
    out = Path(output_dir)
    out.mkdir(parents=True, exist_ok=True)
    model.eval()
    report = {'seed': seed, 'epochs': epochs, 'batch_size': batch_size,
              'learning_rate': lr, 'width': width, 'holdout_kind': holdout,
              'training_kinds': sorted(set(p[0] for p in training)),
              'train_samples': len(x), 'train_mse': history[-1],
              'held_out': [], 'loss_history': history}
    for kind, dry, wet, sr in held_out:
        with torch.no_grad():
            pred = model(torch.from_numpy(dry.astype('float32').reshape(-1, 1))).numpy().reshape(dry.shape)
        score = metrics(wet, pred)
        report['held_out'].append({'kind': kind, 'rate': sr, 'channels': dry.shape[1], **score})
        # Floating-point WAV writer rejects overs; clip explicitly for export only.
        write_wav(out / f'{kind}_predicted.wav', np.clip(pred, -1, 1), sr)
    torch.save({'state_dict': model.state_dict(), 'width': width, 'seed': seed}, out / 'model.pt')
    (out / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    return report


def predict(checkpoint, input_wav, output_wav):
    torch = _torch()
    # Only load trusted checkpoints; PyTorch weights_only prevents general pickle object loading.
    data = torch.load(checkpoint, map_location='cpu', weights_only=True)
    model = build_model(torch, int(data['width']))
    model.load_state_dict(data['state_dict'])
    model.eval()
    x, sr = read_wav(input_wav)
    with torch.no_grad():
        y = model(torch.from_numpy(x.astype('float32').reshape(-1, 1))).numpy().reshape(x.shape)
    write_wav(output_wav, np.clip(y, -1, 1), sr)
