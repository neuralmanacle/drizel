import json
import subprocess
import sys
from pathlib import Path
import numpy as np
import pytest
from effect import saturate
from signals import KINDS, generate
from audio_io import read_wav, write_wav


def test_bypass():
    x = generate('noise')
    assert np.array_equal(saturate(x, mix=0), x)
    assert np.array_equal(saturate(x, drive=0), x)


def test_bounds_and_silence():
    x = np.linspace(-1, 1, 101)[:, None]
    assert np.max(np.abs(saturate(x, drive=4))) <= 1
    assert np.array_equal(saturate(np.zeros((10, 2))), np.zeros((10, 2)))


def test_channels_and_determinism():
    for kind in KINDS:
        a = generate(kind, channels=2, seed=7)
        assert a.shape == (48000, 2)
        assert np.array_equal(a, generate(kind, channels=2, seed=7))
        assert saturate(a).shape == a.shape


@pytest.mark.parametrize('kwargs', [{'drive': -1}, {'mix': 2}, {'output_gain': -1}, {'drive': float('nan')}])
def test_invalid_parameters(kwargs):
    with pytest.raises(ValueError):
        saturate(np.zeros((5, 1)), **kwargs)


def test_invalid_audio():
    for x in [np.array([]), np.zeros((3, 3)), np.array([[float('nan')]]), np.array([[2.]])]:
        with pytest.raises(ValueError):
            saturate(x)


def test_wav_roundtrip(tmp_path):
    x = generate('sine', channels=2)
    path = tmp_path / 'test.wav'
    write_wav(path, x, 44100)
    y, rate = read_wav(path)
    assert rate == 44100 and y.shape == x.shape
    assert np.allclose(x, y, atol=1e-7)


def test_dataset_cli(tmp_path):
    cli = Path(__file__).with_name('cli.py')
    subprocess.run([sys.executable, str(cli), 'dataset', str(tmp_path), '--duration', '0.01'], check=True)
    manifest = json.loads((tmp_path / 'manifest.json').read_text())
    assert len(manifest['pairs']) == 5
    for item in manifest['pairs']:
        dry, rate = read_wav(tmp_path / item['dry'])
        wet, wet_rate = read_wav(tmp_path / item['wet'])
        assert rate == wet_rate == 48000 and dry.shape == wet.shape
