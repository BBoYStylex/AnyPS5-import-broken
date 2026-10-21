import { test } from 'node:test';
import assert from 'node:assert/strict';
import { describeProgress, formatBytes, isFatal, parseDataProgress, startStatus } from '../status.mjs';

class FakeNode {
  constructor(document, tag) {
    this.ownerDocument = document;
    this.tagName = tag;
    this.children = [];
    this.classes = new Set();
    this.listeners = {};
    this.textContent = '';
    this.hidden = false;
    this.classList = {
      add: (name) => this.classes.add(name),
      remove: (name) => this.classes.delete(name),
      toggle: (name) => (this.classes.has(name) ? this.classes.delete(name) : this.classes.add(name)),
    };
  }

  appendChild(node) {
    this.children.push(node);
  }

  addEventListener(type, listener) {
    this.listeners[type] = listener;
  }
}

function fakeStatus() {
  const document = { createElement: (tag) => new FakeNode(document, tag) };
  const element = new FakeNode(document, 'div');
  let time = 0;
  const status = startStatus(element, { now: () => time, every: () => () => {} });
  const [title, bar, detail, log] = element.children;
  return { status, element, title, bar, detail, log, advance: (ms) => (time += ms) };
}

test('abort and runtime errors are fatal, other output is not', () => {
  assert.ok(isFatal('Aborted(the runtime does not provide NID rbknaUjpqWo (called from 0x101365c76))'));
  assert.ok(isFatal('anyps5 runtime: sceKernelMapDirectMemory threw into guest code: overlap'));
  assert.ok(isFatal('anyps5 loader: TypeError: WebAssembly compilation aborted'));
  assert.ok(isFatal('worker sent an error! http://127.0.0.1:8080/runtime/anyps5-runtime-web.js:1: Uncaught RuntimeError: unreachable'));
  assert.ok(!isFatal('anyps5 storage: saved'));
  assert.ok(!isFatal('[src/Open.cpp:226 sceKernelOpen] path=/app0/Media/boot.config'));
});

test('the data loader progress is parsed', () => {
  assert.deepEqual(parseDataProgress('Downloading data... (1048576/1056803997)'), { loaded: 1048576, total: 1056803997 });
  assert.equal(parseDataProgress('Downloading data...'), null);
  assert.equal(parseDataProgress('Running...'), null);
});

test('progress shows the size, the speed and the time left', () => {
  assert.equal(formatBytes(5 * 2 ** 20), '5.0 MB');
  assert.equal(formatBytes(3 * 2 ** 30), '3.00 GB');
  assert.equal(describeProgress(100 * 2 ** 20, 400 * 2 ** 20, 10), '100.0 MB / 400.0 MB (25%) · 10.0 MB/s · about 30 s left');
  assert.equal(describeProgress(2 ** 20, 0, 0), '1.0 MB');
  assert.equal(describeProgress(2 ** 20, 4 * 2 ** 20, 0.1), '1.0 MB / 4.0 MB (25%)', 'no speed before a second has passed');
});

test('steps and downloads update the panel until the game runs', () => {
  const { status, element, title, bar, detail, advance } = fakeStatus();
  status.step('Downloading game code (game.wasm)');
  advance(2000);
  status.progress(2 ** 20, 4 * 2 ** 20);
  assert.equal(title.textContent, 'Downloading game code (game.wasm)');
  assert.equal(bar.hidden, false);
  assert.equal(detail.textContent, '1.0 MB / 4.0 MB (25%)');
  status.running();
  assert.equal(title.textContent, 'Running');
  assert.ok(element.classes.has('running'));
  assert.equal(bar.hidden, true);
  status.step('ignored after the start');
  assert.equal(title.textContent, 'Running');
});

test('the first failure stays on the panel with the last log lines', () => {
  const { status, element, title, log } = fakeStatus();
  for (let index = 0; index < 20; index += 1) status.log('line ' + index);
  status.running();
  status.fail('Aborted(first)');
  status.fail('worker sent an error!');
  status.running();
  assert.equal(status.state, 'failed');
  assert.equal(title.textContent, 'Stopped: Aborted(first)');
  assert.ok(element.classes.has('failed') && !element.classes.has('running'));
  assert.equal(log.textContent.split('\n').length, 12);
  assert.ok(log.textContent.endsWith('line 19'));
});
