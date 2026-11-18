# Step 14.2 — Training a Neural Saturation Baseline

## Question
Can a small sample-wise neural network approximate the normalized tanh soft-saturation reference from Step 14.1?

## Reference and architecture
For drive `g > 0`, reference wet signal is `tanh(g*x)/tanh(g)`; dry/wet and output gain are applied by the Step 14.1 DSP reference. The model is a memoryless MLP: `Linear(1,32) → Tanh → Linear(32,32) → Tanh → Linear(32,1)`. It predicts output amplitude for each input sample. No recurrent state, convolution, oversampling, anti-aliasing or temporal modelling is implemented.

## Experiment
Use Step 14.1 `cli.py dataset` to generate paired float WAVs and manifest. Hold out one signal kind (default: noise) and train on all remaining kinds, with seeded sampling and PyTorch initialization. Minimize mean squared error using Adam. Save a weights-only state dictionary and JSON report. Evaluate on the held-out signal using MSE, MAE and signal-to-error ratio (`10 log10((mean(y²)+epsilon)/(mean((yhat-y)²)+epsilon))`). Compare predicted WAV against the reference by listening; report any level and harmonic differences.

## Limitations and acceptance
This is an offline proof of concept, not a trained model until the training command runs successfully. Five short synthetic fixtures are insufficient for broad musical generalization. The model is not conditioned on drive/mix/gain, and train/test splits by signal kind do not prevent all synthetic-distribution overlap. Report the exact environment, random seed, epochs, held-out kind, and observed metrics. Real-time C++/JUCE integration, performance profiling, oversampling and perceptual listening remain future work. Never load untrusted model checkpoints.
