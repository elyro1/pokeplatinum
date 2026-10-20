#!/usr/bin/env python3
"""Check the compiled trainer quads against their shoulder/elbow artwork without opening a window."""

import argparse
import json
from pathlib import Path
import shlex
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, default=ROOT / "build_linux")
    args = parser.parse_args()
    build = args.build.resolve()
    commands = json.loads((build / "compile_commands.json").read_text())
    entry = next(c for c in commands if c["file"].endswith("/system/mcss.c"))
    flags = shlex.split(entry["command"])
    flags = flags[:flags.index("-MD")]
    with tempfile.TemporaryDirectory(prefix="trainer-joints-") as directory:
        executable = Path(directory) / "test"
        subprocess.run(flags + ["-ffunction-sections", "-fdata-sections",
                               str(ROOT / "tools/scripts/bw_trainer_backs_test.c"),
                               str(build / "subprojects/libntr/libraries/fx/libfx.a.p/src_fx_sincos.c.o"),
                               "-Wl,--gc-sections", "-lm", "-o", str(executable)], cwd=build, check=True)
        subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    main()
