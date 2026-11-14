"""Preview and apply an editable grain-scan policy."""

import argparse
import json
import re
import subprocess
import sys
from pathlib import Path

PHRASES = {
    "go around in circles": "cyclic",
    "loop around": "cyclic",
    "cycle through the sample": "cyclic",
    "one step forward two steps back": "stepped",
    "one step forward two step back": "stepped",
    "one forward then two back": "stepped",
    "very chaotic": "irregular",
    "move unpredictably": "irregular",
    "jump around randomly": "irregular",
}
DEFAULTS = {
    "region": [0.05, 0.9],
    "seconds": 6,
    "period_seconds": 2,
    "step_fraction": 0.18,
    "step_interval_grains": 5,
    "seed": 12013,
}


VARIATION = {
    "distance_variation": "--distance-variation",
    "timing_variation": "--timing-variation",
}
COMBINED = (
    r"(?:please )?(?:make (?:the grains?|it) )?(?:more )?"
    r"(?:chaotic and stepped|stepped and chaotic)",
    r"move the grains in steps but make the jumps less predictable and vary the timing a little",
)


def preview(prompt, active=None):
    phrase = " ".join(re.findall(r"[a-z]+", prompt.lower()))
    if any(re.fullmatch(pattern, phrase) for pattern in COMBINED):
        policy = {"target": "grain_position", "boundary": "wrap", **DEFAULTS}
        if active is not None:
            scan_arguments(active)
            policy.update(active)
        relative = active is not None and "more" in phrase.split()
        amount = 0.15 if phrase.endswith("a little") else 0.30
        for field in VARIATION:
            policy[field] = min(0.9, round(policy.get(field, 0) + 0.15, 8)) if relative else amount
        return {**policy, "schema_version": 2, "pattern": "stepped"}
    if phrase not in PHRASES:
        raise ValueError("Unrecognized movement. Try 'make the grain more chaotic and stepped', "
                         "'go around in circles', or 'very chaotic'.")
    if active is not None:
        raise ValueError("--active applies to combined stepped-and-chaotic commands.")
    return {"schema_version": 1, "target": "grain_position", "boundary": "wrap",
            "pattern": PHRASES[phrase], **DEFAULTS}


def scan_arguments(policy):
    fields = {"schema_version", "target", "boundary", "pattern", *DEFAULTS}
    if not isinstance(policy, dict):
        raise ValueError("Expected a policy object.")
    version = policy.get("schema_version")
    if type(version) is not int or version not in (1, 2):
        raise ValueError("Expected policy version 1 or 2.")
    if version == 2:
        fields.update(VARIATION)
    if policy.keys() != fields:
        raise ValueError("Policy fields must match the preview schema.")
    if policy["target"] != "grain_position" or policy["boundary"] != "wrap":
        raise ValueError("Expected grain_position target and wrap boundary.")
    if policy["pattern"] not in ("cyclic", "stepped", "irregular"):
        raise ValueError("Unsupported movement pattern.")
    region = policy["region"]
    if (not isinstance(region, list) or len(region) != 2
            or any(type(value) not in (int, float) for value in region)):
        raise ValueError("Region must be two normalized numbers.")
    options = ["--pattern", policy["pattern"], "--region", *map(str, region)]
    for field, option, types in (
        ("seconds", "--seconds", (int,)),
        ("period_seconds", "--period", (int, float)),
        ("step_fraction", "--step", (int, float)),
        ("step_interval_grains", "--step-interval", (int,)),
        ("seed", "--seed", (int,)),
    ):
        if type(policy[field]) not in types:
            raise ValueError(f"Invalid number type for {field}.")
        options.extend((option, str(policy[field])))
    if version == 2:
        if policy["pattern"] != "stepped":
            raise ValueError("Distance and timing variation require a stepped policy.")
        for field, option in VARIATION.items():
            value = policy[field]
            if type(value) not in (int, float) or not 0 <= value <= 0.9:
                raise ValueError(f"{field} must be between 0 and 0.9.")
            options.extend((option, str(value)))
    return options


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="action", required=True)
    propose = commands.add_parser("preview", help="Print JSON without rendering")
    propose.add_argument("phrase")
    propose.add_argument("--active", type=Path, help="Current policy for relative changes")
    apply = commands.add_parser("apply", help="Render an accepted JSON policy")
    for name in ("policy", "wav", "renderer", "output"):
        apply.add_argument(name, type=Path)
    args = parser.parse_args()
    try:
        if args.action == "preview":
            active = json.loads(args.active.read_text()) if args.active else None
            if args.active and active is None:
                raise ValueError("Expected an active policy object.")
            print(json.dumps(preview(args.phrase, active), indent=2))
            return 0
        policy = json.loads(args.policy.read_text())
        options = scan_arguments(policy)
    except (ValueError, OSError) as error:
        parser.error(str(error))
    scan = Path(__file__).with_name("scan.py")
    result = subprocess.run([sys.executable, str(scan), str(args.wav), str(args.renderer),
                             str(args.output), *options])
    return result.returncode


if __name__ == "__main__":
    sys.exit(main())
