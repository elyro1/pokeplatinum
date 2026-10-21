#!/usr/bin/env python3
"""Validate the built Iris party and the dungeon exit encounter's wiring."""
import argparse
import enum
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def narc(path):
    data = path.read_bytes()
    assert data[:4] == b'NARC', path
    offset = 16
    while offset < len(data):
        magic, size = struct.unpack_from('<4sI', data, offset)
        if magic == b'BTAF':
            count = struct.unpack_from('<H', data, offset + 8)[0]
            entries = [struct.unpack_from('<II', data, offset + 12 + i * 8) for i in range(count)]
        elif magic == b'GMIF':
            image = data[offset + 8:offset + size]
        offset += size
    return [image[start:end] for start, end in entries]


def constants(build):
    result = {'SPECIES_HYDREIGON': 635}
    for name in ['trainers', 'trainer_classes', 'species', 'items', 'moves', 'natures', 'abilities', 'ai_flags']:
        scope = {}
        exec((build / 'generated' / (name + '.py')).read_text(), scope)
        for value in scope.values():
            if isinstance(value, type) and issubclass(value, (enum.IntEnum, enum.IntFlag)):
                result.update((member.name, int(member)) for member in value)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build_linux')
    parser.add_argument('--baseline-dir', type=Path, help='Optional original trdata.narc/trpoke.narc directory')
    args = parser.parse_args()
    build = args.build_dir
    ids = constants(build)
    trainer_id = ids['TRAINER_CHAMPION_IRIS']
    headers = narc(build / 'res/trainers/trdata.narc')
    parties = narc(build / 'res/trainers/trpoke.narc')
    header = struct.unpack('<4B4HII', headers[trainer_id])
    assert header[:4] == (4, ids['TRAINER_CLASS_CHAMPION_IRIS'], 0, 6), header
    assert header[4:8] == (0, 0, 0, 0)
    assert header[8] == sum(ids[name] for name in ['AI_FLAG_BASIC', 'AI_FLAG_EVAL_ATTACK', 'AI_FLAG_EXPERT', 'AI_FLAG_CHECK_HP', 'AI_FLAG_WEATHER'])
    assert header[9] == 0  # Singles
    team = json.loads((ROOT / 'res/trainers/data/champion_iris.json').read_text())['party']
    assert [m['species'] for m in team] == ['SPECIES_HYDREIGON', 'SPECIES_KYOGRE', 'SPECIES_PALKIA', 'SPECIES_ARCEUS', 'SPECIES_DIALGA', 'SPECIES_SKARMORY']
    assert len(parties[trainer_id]) == 26 * 6
    for i, expected in enumerate(team):
        mon = struct.unpack_from('<9H8B', parties[trainer_id], 26 * i)
        assert mon[:4] == (255, 100, ids[expected['species']], ids[expected['item']]), (i, mon)
        assert mon[4:8] == tuple(ids[move] for move in expected['moves'])
        assert mon[9:11] == (ids[expected['nature']], ids[expected['ability']])
        assert mon[11:] == tuple(expected['evs'])
        assert sum(mon[11:]) == 508
    if args.baseline_dir:
        for name, current in [('trdata', headers), ('trpoke', parties)]:
            original = narc(args.baseline_dir / (name + '.narc'))
            assert current[:len(original)] == original, name + ': original trainers changed'
    event = json.loads((ROOT / 'res/field/events/events_iron_island.json').read_text())
    iris = next(obj for obj in event['object_events'] if obj['id'] == 'LOCALID_IRIS')
    assert (iris['x'], iris['z'], iris['script'], iris['hidden_flag']) == (102, 488, 4, '0')
    land = (ROOT / 'res/field/maps/data/map_data_054.bin').read_bytes()
    for x, z in [(102, 488), (102, 489), (102, 490), (103, 490)]:
        tile = struct.unpack_from('<H', land, 16 + ((z % 32) * 32 + x % 32) * 2)[0]
        assert not tile & 0x8000, (x, z, tile)
    script = (ROOT / 'res/field/scripts/scripts_iron_island.s').read_text().split('IronIsland_Iris:', 1)[1]
    assert script.index('GoToIfUnset FLAG_GAME_COMPLETED') < script.index('StartTrainerBattle TRAINER_CHAMPION_IRIS')
    assert script.index('CheckIrisAssets VAR_RESULT') < script.index('GoToIfUnset FLAG_GAME_COMPLETED')
    assert 'IronIsland_Visitor:' in script
    assert 'BlackOutFromBattle' in script
    assert not (ROOT / 'res/graphics/field_sprites/npc/iris.png').exists()
    assert not (ROOT / 'res/pokemon/hydreigon').exists()
    assert not (ROOT / 'res/trainers/classes/champion_iris').exists()
    assert 'iris_nsbtx' in (ROOT / 'src/overlay005/object_event_gfx_data.c').read_text()
    assert 'zz_hydreigon_front_NCGR' in (build / 'res/pokemon/pl_otherpoke.naix').read_text()
    # Optional slots must contain only existing Platinum art in build/release data.
    trainer_graphics = narc(build / 'res/trainers/classes/trfgra.narc')
    iris_class = ids['TRAINER_CLASS_CHAMPION_IRIS'] * 5
    cynthia_class = ids['TRAINER_CLASS_CHAMPION_CYNTHIA'] * 5
    assert trainer_graphics[iris_class:iris_class + 5] == trainer_graphics[cynthia_class:cynthia_class + 5]
    pokemon_graphics = narc(build / 'res/pokemon/pl_otherpoke.narc')
    assert pokemon_graphics[253] == pokemon_graphics[254] == pokemon_graphics[0]
    assert pokemon_graphics[255] == pokemon_graphics[256] == pokemon_graphics[154]
    field_graphics = narc(build / 'res/graphics/field_sprites/field_sprites.narc')
    # Texture dictionary names differ, so compare the decoded pixel payload below
    # the TEX0 texture-data offset and before the palette-data offset.
    def texture_bytes(data):
        tex = struct.unpack_from('<I', data, 16)[0]
        start = tex + struct.unpack_from('<I', data, tex + 20)[0]
        end = tex + struct.unpack_from('<I', data, tex + 56)[0]
        return data[start:end]
    assert texture_bytes(field_graphics[470]) == texture_bytes(field_graphics[51])
    print('PASS: optional native slots contain Platinum templates, with no imported BW2 art or cry')
    sound = (build / 'res/sound/pl_sound_data.sdat').read_bytes()
    u32 = lambda offset: struct.unpack_from('<I', sound, offset)[0]
    assert u32(8) == len(sound), 'SDAT size does not cover the complete FILE block'
    fat = u32(32)
    for i in range(u32(fat + 8)):
        start, size = struct.unpack_from('<II', sound, fat + 12 + i * 16)
        assert start % 32 == 0, (i, start)
        assert start + size <= len(sound), (i, start, size)
        assert sound[start:start + 4] in (b'SSEQ', b'SBNK', b'SWAR', b'SSAR', b'STRM'), (i, start)
    registry = json.loads((ROOT / 'res/sound/pl_sound_data.json').read_text())
    assert registry['seqInfo'][-1]['name'] == 'SEQ_BA_IRIS'
    assert registry['seqInfo'][-1]['player'] == 'PLAYER_BGM'
    assert not any('HYDREIGON' in entry.get('name', '') for key in ['bankInfo', 'wavarcInfo'] for entry in registry[key])
    assert 'BATTLE_IRIS_sseq' in (build / 'res/sound/pl_sound_data.naix').read_text()
    print('PASS: SDAT FAT entries include header/padding and point to aligned sound files')
    print('PASS: six competitive level-100 Pokémon, party encoding, league gate, reachable exit ledge, graphics')
    if args.baseline_dir:
        print('PASS: all original trainer headers and parties remain byte-for-byte identical')


if __name__ == '__main__':
    main()
