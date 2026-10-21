#!/usr/bin/env python3
"""Reserve Iris's native resource slots using existing Platinum templates."""
import shutil
import subprocess
import sys
from pathlib import Path
narc, order, output, private = sys.argv[1:]
private = Path(private)
(private / 'champion_iris').mkdir(exist_ok=True)
for name in ['front.NCGR', 'front.NCLR', 'front_cell.NCER', 'front_anim.NANR', 'front_scan.NCGR']:
    shutil.copyfile(private / 'champion_cynthia' / name, private / 'champion_iris' / name)
subprocess.run([narc, '--create', '--index', '--files-from', order, '--file', output, str(private)], check=True)
