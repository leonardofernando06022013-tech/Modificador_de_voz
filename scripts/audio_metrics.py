#!/usr/bin/env python3
"""Measure latency and scale-aligned SNR for BlackVoice A/B recordings.

The script intentionally uses only the Python standard library. Input files must
be PCM WAV recordings made from the same clean reference and sample rate.
"""

from __future__ import annotations

import argparse
import json
import math
import struct
import wave
from pathlib import Path
from typing import Iterable


def _decode_pcm(raw: bytes, sample_width: int) -> list[float]:
    if sample_width == 1:
        return [(value - 128) / 128.0 for value in raw]
    if sample_width == 2:
        count = len(raw) // 2
        return [value / 32768.0 for value in struct.unpack(f"<{count}h", raw)]
    if sample_width == 3:
        result: list[float] = []
        for offset in range(0, len(raw), 3):
            value = int.from_bytes(raw[offset : offset + 3], "little", signed=False)
            if value & 0x800000:
                value -= 1 << 24
            result.append(value / 8388608.0)
        return result
    if sample_width == 4:
        count = len(raw) // 4
        return [value / 2147483648.0 for value in struct.unpack(f"<{count}i", raw)]
    raise ValueError(f"PCM de {sample_width * 8} bits não é suportado")


def read_mono(path: Path, seconds: float) -> tuple[int, list[float]]:
    with wave.open(str(path), "rb") as source:
        channels = source.getnchannels()
        sample_rate = source.getframerate()
        frames = min(source.getnframes(), int(seconds * sample_rate))
        decoded = _decode_pcm(source.readframes(frames), source.getsampwidth())

    if channels == 1:
        return sample_rate, decoded
    mono = []
    for offset in range(0, len(decoded) - channels + 1, channels):
        mono.append(sum(decoded[offset : offset + channels]) / channels)
    return sample_rate, mono


def remove_dc(samples: Iterable[float]) -> list[float]:
    values = list(samples)
    if not values:
        return values
    mean = sum(values) / len(values)
    return [value - mean for value in values]


def estimate_latency(
    reference: list[float], candidate: list[float], sample_rate: int, max_lag_ms: float
) -> int:
    target_rate = 2000
    stride = max(1, sample_rate // target_rate)
    ref = remove_dc(reference[::stride])
    test = remove_dc(candidate[::stride])
    maximum = max(1, int(max_lag_ms * sample_rate / 1000.0 / stride))
    best_lag = 0
    best_score = -1.0

    for lag in range(-maximum, maximum + 1):
        if lag >= 0:
            left = ref[: min(len(ref), len(test) - lag)]
            right = test[lag : lag + len(left)]
        else:
            right = test[: min(len(test), len(ref) + lag)]
            left = ref[-lag : -lag + len(right)]
        if len(left) < target_rate // 4:
            continue
        dot = sum(a * b for a, b in zip(left, right))
        energy_left = sum(value * value for value in left)
        energy_right = sum(value * value for value in right)
        score = abs(dot) / math.sqrt(max(energy_left * energy_right, 1.0e-30))
        if score > best_score:
            best_score = score
            best_lag = lag

    return best_lag * stride


def aligned(reference: list[float], candidate: list[float], lag: int) -> tuple[list[float], list[float]]:
    if lag >= 0:
        count = min(len(reference), len(candidate) - lag)
        return reference[:count], candidate[lag : lag + count]
    count = min(len(candidate), len(reference) + lag)
    return reference[-lag : -lag + count], candidate[:count]


def scale_aligned_snr(reference: list[float], candidate: list[float], lag: int) -> float:
    clean, measured = aligned(reference, candidate, lag)
    if not clean:
        raise ValueError("As gravações não possuem região sobreposta")
    measured_power = sum(value * value for value in measured)
    scale = sum(a * b for a, b in zip(clean, measured)) / max(measured_power, 1.0e-30)
    signal_power = sum(value * value for value in clean)
    error_power = sum((a - scale * b) ** 2 for a, b in zip(clean, measured))
    return 10.0 * math.log10(max(signal_power, 1.0e-30) / max(error_power, 1.0e-30))


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Calcula SNR e latência em gravações A/B PCM WAV do BlackVoice."
    )
    parser.add_argument("--reference", type=Path, required=True, help="fala limpa de referência")
    parser.add_argument("--before", type=Path, required=True, help="captura sem processamento")
    parser.add_argument("--after", type=Path, required=True, help="captura processada")
    parser.add_argument("--duration", type=float, default=10.0, help="segundos analisados (padrão: 10)")
    parser.add_argument("--max-lag-ms", type=float, default=250.0, help="latência máxima pesquisada")
    args = parser.parse_args()

    reference_rate, reference = read_mono(args.reference, args.duration)
    before_rate, before = read_mono(args.before, args.duration)
    after_rate, after = read_mono(args.after, args.duration)
    if len({reference_rate, before_rate, after_rate}) != 1:
        raise SystemExit("Os três arquivos precisam usar a mesma sample rate.")

    before_lag = estimate_latency(reference, before, reference_rate, args.max_lag_ms)
    after_lag = estimate_latency(reference, after, reference_rate, args.max_lag_ms)
    before_snr = scale_aligned_snr(reference, before, before_lag)
    after_snr = scale_aligned_snr(reference, after, after_lag)
    report = {
        "sample_rate_hz": reference_rate,
        "duration_seconds": min(args.duration, len(reference) / reference_rate),
        "before": {
            "snr_db": round(before_snr, 3),
            "latency_ms": round(before_lag * 1000.0 / reference_rate, 3),
        },
        "after": {
            "snr_db": round(after_snr, 3),
            "latency_ms": round(after_lag * 1000.0 / reference_rate, 3),
        },
        "snr_improvement_db": round(after_snr - before_snr, 3),
        "added_latency_ms": round((after_lag - before_lag) * 1000.0 / reference_rate, 3),
    }
    print(json.dumps(report, indent=2, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
