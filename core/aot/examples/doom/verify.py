import argparse
import hashlib
import json
import os
import re
import shutil
import socket
import struct
import subprocess
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(value, reason):
    if not value:
        raise ValueError(reason)


def receive(connection, count):
    data = bytearray()
    while len(data) < count:
        part = connection.recv(min(count - len(data), 1048576))
        if not part:
            raise EOFError('incomplete capture packet')
        data.extend(part)
    return bytes(data)


def replay(binary, package, demo, expected, output, compare, references):
    output.mkdir()
    run = output / 'run'
    run.mkdir()
    shutil.copytree(package, run / 'app0')
    (run / 'download0').mkdir()
    with socket.socket() as reservation:
        reservation.bind(('127.0.0.1', 0))
        port = reservation.getsockname()[1]
    (run / 'app0/test.cfg').write_text(f'game doom1.wad\npresent cpu\ncapture 120,900\nserve {port}\n')
    started, errors = time.monotonic(), []
    with (output / 'run.log').open('w') as log:
        process = subprocess.Popen([str(binary), '-timedemo', demo], cwd=run, stdout=log, stderr=subprocess.STDOUT)
        timeout = False
        try:
            while process.poll() is None:
                if time.monotonic() - started > 180:
                    timeout = True
                    break
                try:
                    with socket.create_connection(('127.0.0.1', port), timeout=1) as connection:
                        connection.settimeout(5)
                        while True:
                            length = struct.unpack('<I', receive(connection, 4))[0]
                            if length == 0:
                                break
                            require(1 <= length <= 32, 'invalid capture name length')
                            name = receive(connection, length).decode('ascii')
                            require(re.fullmatch(r'shot\d{5}', name) or name == 'doom.log', 'invalid capture name')
                            size = struct.unpack('<Q', receive(connection, 8))[0]
                            require(0 < size <= 16777216, 'invalid capture size')
                            path = output / ('guest-doom.log' if name == 'doom.log' else name + '.tiled')
                            path.write_bytes(receive(connection, size))
                except ConnectionRefusedError:
                    pass
                except (OSError, EOFError, ValueError) as error:
                    errors.append(str(error))
                time.sleep(0.3)
        finally:
            if process.poll() is None:
                process.kill()
            code = process.wait()
    text = (output / 'run.log').read_text(errors='replace')
    result = {'demo': demo, 'exit': code, 'timeout': timeout, 'capture_errors': errors,
              'gametics': sorted(set(int(x) for x in re.findall(r'timed (\d+) gametics in', text))),
              'native': 'architecture=arm64 translated=0 original-inputs=2' in text,
              'cpu_scaler': 'video: CPU presentation' in text, 'guest_exit_1': 'ps5: exit 1' in text,
              'gpu_teardown': 'Destroyed VkDevice' in text,
              'module_teardown': 'original SCE bundle teardown completed' in text, 'frames': []}
    for frame in (120, 900):
        captured = output / f'shot{frame:05d}.tiled'
        if not captured.exists():
            result['frames'].append({'frame': frame, 'missing': True})
            continue
        comparison = subprocess.run([str(compare), str(captured), str(references / demo / f'frame{frame:05d}.raw'),
                                     str(output / f'frame{frame:05d}.ppm')], capture_output=True, text=True)
        record = json.loads(comparison.stdout) if comparison.stdout else {'comparison_error': comparison.stderr}
        record.update(frame=frame, exit=comparison.returncode, sha256=digest(captured))
        result['frames'].append(record)
    result['passed'] = (code == 0 and not timeout and not errors and result['gametics'] == [expected]
                        and all(result[key] for key in ('native', 'cpu_scaler', 'guest_exit_1', 'gpu_teardown', 'module_teardown'))
                        and all(frame.get('exit') == 0 and frame.get('different_rgb_pixels') == 0 for frame in result['frames']))
    (output / 'receipt.json').write_text(json.dumps(result, indent=2) + '\n')
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--binary', type=Path, required=True)
    parser.add_argument('--package', type=Path, required=True)
    parser.add_argument('--source-root', type=Path, required=True)
    parser.add_argument('--references', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    binary, package, output = args.binary.resolve(), args.package.resolve(), args.output.resolve()
    manifest = json.loads((HERE / 'bindings.json').read_text())
    require(not output.exists(), 'choose a fresh output directory')
    require(binary.read_bytes()[:8] == struct.pack('<II', 0xfeedfacf, 0x0100000c), 'single architecture ARM64 Mach-O required')
    require(digest(package / 'wads/doom1.wad') == manifest['wad_sha256'], 'WAD input changed')
    tiling = args.source_root.resolve() / 'src/ps5/tiling.h'
    require(digest(tiling) == manifest['tiling_header_sha256'], 'capture addressing source changed')
    for path, expected in manifest['reference_frames'].items():
        require(digest(args.references / path) == expected, f'reference input changed: {path}')
    for variable in ('SDL_VULKAN_LIBRARY', 'VK_ICD_FILENAMES'):
        require(bool(os.environ.get(variable)), f'explicit {variable} required')
    output.mkdir(parents=True)
    compare = output / 'compare'
    subprocess.run(['xcrun', 'clang++', '-std=c++20', '-O2', '-I' + str(tiling.parent), str(HERE / 'compare.cpp'), '-o', str(compare)], check=True)
    result = {'binary_sha256': digest(binary), 'manifest_sha256': digest(HERE / 'bindings.json'), 'replays': [],
              'scope': 'original ELF AOT, CPU scaler and exact RGB; not AGC, gameplay compatibility or performance qualification'}
    for demo, expected in [('demo1', 5026), ('demo2', 3836), ('demo3', 2134)]:
        item = replay(binary, package, demo, expected, output / demo, compare, args.references.resolve())
        result['replays'].append(item)
        result['passed'] = len(result['replays']) == 3 and all(r['passed'] for r in result['replays'])
        (output / 'receipt.json').write_text(json.dumps(result, indent=2) + '\n')
        print(demo, 'PASS' if item['passed'] else 'FAIL', flush=True)
        require(item['passed'], f'{demo} failed; inspect its receipt')


if __name__ == '__main__':
    main()
