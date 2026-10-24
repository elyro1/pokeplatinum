#!/usr/bin/env python3
"""Check that a rebuild cannot change an archive already open in the game."""
import os
from pathlib import Path
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[2]


def main():
    with tempfile.TemporaryDirectory(prefix='pc-asset-copy-') as directory:
        root = Path(directory)
        build = root / 'build with spaces'
        source = build / 'source resources' / 'sound.sdat'
        source.parent.mkdir(parents=True)
        old = b'old archive index and Roserade cry'
        new = b'new archive with an extra music entry and different file offsets'
        source.write_bytes(old)
        stamp = int(time.time()) - 100
        os.utime(source, (stamp, stamp))
        (build / 'main').write_bytes(b'test executable')
        extras = root / 'res' / 'pc_port'
        extras.mkdir(parents=True)
        for name in ['header.bin', 'dwc_nv.bin', 'field_lights.txt']:
            (extras / name).write_bytes(b'original')
        manifest = root / 'files.csv'
        manifest.write_text('Source File,Target File\nsource resources/sound.sdat,/data/sound/sound.sdat\n')
        command = ['bash', str(ROOT / 'tools/scripts/copy_pc_files.sh'), str(build), str(manifest), 'game directory']

        def copy():
            subprocess.run(command, check=True, capture_output=True)

        copy()
        game = build / 'game directory'
        archive = game / 'data/sound/sound.sdat'
        save = game / 'save3.bin'
        save.write_bytes(b'unchanged save')
        with archive.open('rb') as running_game:
            assert running_game.read() == old
            source.write_bytes(new)
            os.utime(source, (stamp + 10, stamp + 10))
            copy()
            running_game.seek(0)
            assert running_game.read() == old, 'rebuild changed bytes behind a running game\'s cached index'
            assert archive.read_bytes() == new, 'next game run did not receive the new archive'

        inode = archive.stat().st_ino
        copy()
        assert archive.stat().st_ino == inode, 'unchanged resource was replaced'
        lights = game / 'field_lights.txt'
        lights.write_bytes(b'user settings')
        future = int(time.time()) + 10
        os.utime(lights, (future, future))
        copy()
        assert lights.read_bytes() == b'user settings'
        assert save.read_bytes() == b'unchanged save'
        source.unlink()
        failed = subprocess.run(command, capture_output=True)
        assert failed.returncode != 0, 'missing resource was silently ignored'
        assert archive.read_bytes() == new
        assert not list(game.rglob('*.tmp.*')), 'temporary asset files were left behind'

        source.write_bytes(new)
        manifest.write_text(
            'Source File,Target File\n'
            'source resources/sound.sdat,/data/sound/sound.sdat\n'
            'res/moves/anim_scripts.narc,/wazaeffect/we.arc\n'
            'res/battle/scripts/waza_seq.narc,/battle/skill/waza_seq.narc\n')
        (build / 'main').unlink()
        (build / 'main.exe').write_bytes(b'Windows test executable')
        windows_command = command + ['--skip-anim-scripts']
        subprocess.run(windows_command, check=True, capture_output=True)
        assert (game / 'main.exe').read_bytes() == b'Windows test executable'
        assert not (game / 'wazaeffect/we.arc').exists()
        assert not (game / 'battle/skill/waza_seq.narc').exists()
        failed = subprocess.run(command, capture_output=True)
        assert failed.returncode != 0, 'animation archives must be required without the skip option'
        assert b'res/moves/anim_scripts.narc' in failed.stderr

        animations = build / 'res/moves/anim_scripts.narc'
        animations.parent.mkdir(parents=True)
        animations.write_bytes(b'move animations')
        failed = subprocess.run(command, capture_output=True)
        assert failed.returncode != 0, 'move scripts must also be required without the skip option'
        assert b'res/battle/scripts/waza_seq.narc' in failed.stderr
        scripts = build / 'res/battle/scripts/waza_seq.narc'
        scripts.parent.mkdir(parents=True)
        scripts.write_bytes(b'move scripts')
        copy()
        assert (game / 'wazaeffect/we.arc').read_bytes() == b'move animations'
        assert (game / 'battle/skill/waza_seq.narc').read_bytes() == b'move scripts'

        source.unlink()
        failed = subprocess.run(windows_command, capture_output=True)
        assert failed.returncode != 0, 'skipping animations must not hide other missing resources'
        assert b'sound.sdat' in failed.stderr
    print('PASS: running readers keep old archive bytes; new readers get the replacement')
    print('PASS: unchanged resources, newer user settings and saves are preserved; missing inputs fail safely')
    print('PASS: CI skips only the configured animation archives and stages main.exe with its extension')


if __name__ == '__main__':
    main()
