
const FATAL = [/^Aborted\(/, /^anyps5 (runtime|loader):/, /^worker sent an error/, /^Uncaught /];
const DATA_PROGRESS = /^Downloading data\.\.\. \((\d+)\/(\d+)\)$/;
const LOG_LINES = 12;

export function isFatal(line) {
  return FATAL.some((pattern) => pattern.test(line));
}

export function parseDataProgress(text) {
  const match = DATA_PROGRESS.exec(text);
  return match ? { loaded: Number(match[1]), total: Number(match[2]) } : null;
}

export function formatBytes(bytes) {
  if (bytes >= 2 ** 30) return (bytes / 2 ** 30).toFixed(2) + ' GB';
  return (bytes / 2 ** 20).toFixed(1) + ' MB';
}

export function describeProgress(loaded, total, seconds) {
  const parts = [total > 0 ? `${formatBytes(loaded)} / ${formatBytes(total)} (${Math.floor((loaded / total) * 100)}%)` : formatBytes(loaded)];
  if (seconds >= 1 && loaded > 0) {
    parts.push(`${formatBytes(loaded / seconds)}/s`);
    if (total > loaded) parts.push(`about ${Math.ceil(((total - loaded) * seconds) / loaded)} s left`);
  }
  return parts.join(' · ');
}

export function startStatus(element, { now = () => performance.now(), every = (callback, ms) => clearInterval.bind(null, setInterval(callback, ms)) } = {}) {
  const add = (tag, className) => {
    const node = element.ownerDocument.createElement(tag);
    node.className = className;
    element.appendChild(node);
    return node;
  };
  const title = add('div', 'status-step');
  const bar = add('progress', 'status-bar');
  const detail = add('div', 'status-detail');
  const log = add('pre', 'status-log');
  const lines = [];
  let started = now();
  let download = null;
  let state = 'loading';
  bar.hidden = true;

  const seconds = (since) => (now() - since) / 1000;
  const render = () => {
    if (state === 'failed') return;
    if (download) detail.textContent = describeProgress(download.loaded, download.total, seconds(download.started));
    else if (state === 'running') detail.textContent = `for ${Math.floor(seconds(started))} s · click for the log`;
    else detail.textContent = `${Math.floor(seconds(started))} s`;
  };
  const stop = every(render, 1000);
  element.addEventListener('click', () => element.classList.toggle('expanded'));

  return {
    get state() {
      return state;
    },
    step(text) {
      if (state !== 'loading') return;
      title.textContent = text;
      started = now();
      download = null;
      bar.hidden = true;
      render();
    },
    progress(loaded, total) {
      if (state !== 'loading') return;
      download ??= { started: now() };
      download.loaded = loaded;
      download.total = total;
      bar.hidden = !(total > 0);
      bar.max = total;
      bar.value = loaded;
      render();
    },
    log(line) {
      lines.push(line);
      if (lines.length > LOG_LINES) lines.shift();
      log.textContent = lines.join('\n');
    },
    running() {
      if (state !== 'loading') return;
      state = 'running';
      title.textContent = 'Running';
      element.classList.add('running');
      download = null;
      bar.hidden = true;
      started = now();
      render();
    },
    fail(message) {
      if (state === 'failed') return;
      state = 'failed';
      stop();
      element.classList.remove('running');
      element.classList.add('failed');
      title.textContent = 'Stopped: ' + message;
      bar.hidden = true;
      detail.textContent = 'The last lines of the log are below; the browser console (F12) has all of them.';
    },
  };
}
