import json
import numpy as np
import pytest
from audio_io import write_wav
from neural import load_pairs, split_pairs, metrics, train, predict


def test_metrics_perfect():
    m = metrics(np.array([0.5, -0.5]), np.array([0.5, -0.5]))
    assert m['mse'] == 0 and m['mae'] == 0


def test_metrics_shape_rejected():
    with pytest.raises(ValueError):
        metrics(np.ones(3), np.ones(2))


def test_split_holdout():
    tr, te = split_pairs([('sine', None, None, 48000), ('noise', None, None, 48000)])
    assert len(tr) == len(te) == 1 and te[0][0] == 'noise'
    with pytest.raises(ValueError):
        split_pairs([('sine', None, None, 48000)])


def test_load_pairs_rejects_mismatch(tmp_path):
    write_wav(tmp_path / 'dry.wav', np.zeros((20, 1)), 48000)
    write_wav(tmp_path / 'wet.wav', np.zeros((20, 2)), 48000)
    path = tmp_path / 'manifest.json'
    path.write_text(json.dumps({'pairs': [{'kind': 'sine', 'dry': 'dry.wav', 'wet': 'wet.wav'}]}))
    with pytest.raises(ValueError):
        load_pairs(path)


def test_torch_train_predict(tmp_path):
    pytest.importorskip('torch')
    from effect import saturate
    records = []
    for kind, data in [('sine', np.sin(np.linspace(0, 20, 256)).reshape(-1, 1)),
                       ('noise', np.random.default_rng(4).uniform(-0.7, 0.7, (256, 1)))]:
        write_wav(tmp_path / f'{kind}_dry.wav', data, 16000)
        write_wav(tmp_path / f'{kind}_wet.wav', saturate(data), 16000)
        records.append({'kind': kind, 'dry': f'{kind}_dry.wav', 'wet': f'{kind}_wet.wav'})
    manifest = tmp_path / 'manifest.json'
    manifest.write_text(json.dumps({'pairs': records}))
    out = tmp_path / 'run'
    result = train(manifest, out, epochs=2, batch_size=64, max_samples=512)
    assert len(result['held_out']) == 1
    assert (out / 'model.pt').exists()
    predict(out / 'model.pt', tmp_path / 'sine_dry.wav', out / 'inferred.wav')
    assert (out / 'inferred.wav').exists()
