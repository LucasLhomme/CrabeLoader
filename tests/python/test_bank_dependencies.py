"""Independent AES/XML checks for test_bank_dependencies (requires pycryptodome)."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile
import xml.etree.ElementTree as ET
from Crypto.Cipher import AES

KEY = bytes.fromhex('681bbeea63160188f9b7945104a51499')
def crypt(data):
    return AES.new(KEY, AES.MODE_CTR, nonce=b'').encrypt(data)
def require(value, message):
    if not value:
        raise AssertionError(message)
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('harness', type=Path)
    args = parser.parse_args()
    count = 0
    with tempfile.TemporaryDirectory(prefix='crabe_bank_patch_') as temporary:
        root = Path(temporary)
        source = root / 'bankloads.bdp'
        sounds = root / 'sound'; sounds.mkdir()
        cache = root / 'cache'
        original = b'''<?xml version="1.0" encoding="utf-8"?>
<bankloads><!-- keep this comment --><bank name="Cars_Test.bnk" priority="2"><loaditem>old.dbnk</loaditem></bank><bank name="Other.bnk" priority="7"><loaditem>other.dbnk</loaditem><extra keep="yes"/></bank></bankloads>'''
        source.write_bytes(crypt(original))
        initial_hash = hashlib.sha256(source.read_bytes()).hexdigest()
        for name in ['cars_test.bnk', 'other.bnk', 'old.dbnk', 'other.dbnk', 'carbody.bnk', 'horns.bnk', 'dialog.dlg']:
            (sounds / name).write_bytes(b'fixture')
        def patch(name, body):
            path = root / name
            path.write_text(json.dumps(body), encoding='utf-8')
            return path
        one = patch('a.json', {'version': 1, 'banks': {'Cars_Test.bnk': ['CARBODY.bnk', 'carbody.bnk', 'old.dbnk']}})
        two = patch('b.json', {'version': 1, 'banks': {'cars_test.bnk': ['horns.bnk', 'dialog.dlg']}})
        def run(paths, success=True, input_path=source):
            nonlocal count
            result = subprocess.run([str(args.harness), str(input_path), str(cache), str(sounds), *map(str, paths)], capture_output=True, text=True)
            require((result.returncode == 0) == success, f'unexpected exit: {result.stdout} {result.stderr}')
            count += 1
            return result.stdout.strip()
        out = Path(run([one, two]))
        clear = crypt(out.read_bytes())
        table = ET.fromstring(clear)
        before = ET.fromstring(original)
        require(table.tag == 'bankloads' and len(table) == len(before), 'bank records changed')
        require(table[0].attrib == before[0].attrib, 'bank attributes changed')
        require([n.text for n in table[0]] == ['old.dbnk', 'CARBODY.bnk', 'horns.bnk', 'dialog.dlg'], 'merge order or dedup wrong')
        require(ET.tostring(table[1]) == ET.tostring(before[1]), 'unrelated bank modified')
        require(b'keep this comment' in clear, 'comment lost')
        require(initial_hash == hashlib.sha256(source.read_bytes()).hexdigest(), 'source modified')
        require(Path(run([one, two])) == out, 'cache address not stable')
        patched_source = root / 'patched.bdp'; patched_source.write_bytes(out.read_bytes())
        require(run([one, two], input_path=patched_source) == 'UNCHANGED', 'patching not idempotent')
        invalid = [
            {'version': 2, 'banks': {'Cars_Test.bnk': ['horns.bnk']}},
            {'version': 1, 'banks': {'Cars_Test.bnk': ['../horns.bnk']}},
            {'version': 1, 'banks': {'Cars_Test.bnk': ['missing.bnk']}},
            {'version': 1, 'banks': {'Unknown.bnk': ['horns.bnk']}},
            {'version': 1, 'banks': {'Cars_Test.bnk': ['cars_test.bnk']}},
            {'version': 1, 'banks': {'Cars_Test.bnk': [12]}},
            {'version': 1, 'banks': {'Cars_Test.bnk': ['horns.bnk']}, 'typo': True},
            {'version': 1, 'banks': {'Cars_Test.bnk': ['Other.bnk'], 'Other.bnk': ['Cars_Test.bnk']}},
        ]
        snapshot = {p.name: p.read_bytes() for p in cache.iterdir()}
        for i, body in enumerate(invalid):
            run([one, patch(f'invalid-{i}.json', body)], success=False)
        malformed = root / 'malformed.json'; malformed.write_text('{')
        run([malformed], success=False)
        oversized = root / 'oversized.json'; oversized.write_bytes(b' ' * (128 * 1024 + 1))
        run([oversized], success=False)
        for i, xml in enumerate([b'not XML', b'<wrong/>', b'<!DOCTYPE bankloads [<!ENTITY x SYSTEM "file:///C:/Windows/win.ini">]><bankloads>&x;</bankloads>', b'<bankloads><bank name="a.bnk"/><bank name="A.bnk"/></bankloads>']):
            bad = root / f'bad-{i}.bdp'; bad.write_bytes(crypt(xml))
            run([one], success=False, input_path=bad)
        require(snapshot == {p.name: p.read_bytes() for p in cache.iterdir()}, 'failed patch published files')
        require(initial_hash == hashlib.sha256(source.read_bytes()).hexdigest(), 'failed patch modified source')
        out.write_bytes(b'corrupt cache')
        run([one, two], success=False)
        require(out.read_bytes() == b'corrupt cache', 'corrupt cache was overwritten')
        game = root / 'game'
        native = game / 'assets/sound'; native.mkdir(parents=True)
        native_table = native / 'bankloads.bdp'; native_table.write_bytes(crypt(original))
        for name in ['cars_test.bnk', 'other.bnk', 'old.dbnk', 'other.dbnk', 'carbody.bnk', 'horns.bnk', 'dialog.dlg']:
            (native / name).write_bytes(b'fixture')
        complete = game / 'mods/DisneyInfinityComplete/assets/sound'
        complete.mkdir(parents=True)
        complete_xml = original.replace(b'priority="2"', b'priority="9"')
        complete_table = complete / 'bankloads.bdp'; complete_table.write_bytes(crypt(complete_xml))
        mod = game / 'mods/TestCars'; mod.mkdir()
        request = mod / 'bank_dependencies.json'; request.write_bytes(one.read_bytes())
        def vfs(disabled=None):
            nonlocal count
            command = [str(args.harness), '--vfs', str(game)]
            if disabled:
                command.append(disabled)
            result = subprocess.run(command, capture_output=True, text=True)
            require(result.returncode == 0, result.stderr)
            count += 1
            return next(line[7:] for line in result.stdout.splitlines() if line.startswith('RESULT='))
        merged = Path(vfs())
        require('.crabeloader-cache' in str(merged), 'VFS did not select generated overlay')
        require(ET.fromstring(crypt(merged.read_bytes()))[0].get('priority') == '9', 'Complete override was not preserved')
        require(complete_table.read_bytes() == crypt(complete_xml), 'Complete file modified')
        require(native_table.read_bytes() == crypt(original), 'native file modified')
        require(Path(vfs('TestCars')) == complete_table, 'disabled patch still applies')
        native_overlay = Path(vfs('DisneyInfinityComplete'))
        require(ET.fromstring(crypt(native_overlay.read_bytes()))[0].get('priority') == '2', 'disabled source override still applies')
        language = native / 'dialog/enu'; language.mkdir(parents=True)
        (language / 'localized.dbnk').write_bytes(b'fixture')
        request.write_text(json.dumps({'version': 1, 'banks': {'Cars_Test.bnk': ['localized.dbnk']}}))
        localized = Path(vfs())
        require(ET.fromstring(crypt(localized.read_bytes()))[0][-1].text == 'localized.dbnk', 'localized dependency not accepted')
        (language / 'localized.dbnk').unlink()
        require(Path(vfs()) == complete_table, 'missing localized dependency did not fall back')
        request.write_text('{')
        require(Path(vfs()) == complete_table, 'invalid patch failed to fall back to effective table')
        request.unlink()
        require(Path(vfs()) == complete_table, 'removed patch left stale cache mapping')
        complete_table.unlink()
        require(vfs() == 'ORIGINAL', 'native fallback was overridden without patches')
    print(f'[bank-dependencies] {count} independent encrypted-fixture and VFS checks passed')
if __name__ == '__main__':
    main()
