// TODO(deploy): 修改为实际后端地址
const API_BASE = 'http://localhost:8888/api/v1';

async function apiCall(path, body = {}) {
  let res;
  try {
    res = await fetch(`${API_BASE}${path}`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(body),
    });
  } catch {
    throw new Error('⚠ 后端未连接 — 请先启动 server.exe (localhost:8888)');
  }
  if (!res.ok) {
    const text = await res.text().catch(() => '');
    throw new Error(`服务器错误 HTTP ${res.status}${text ? ': ' + text : ''}`);
  }
  return res.json();
}

async function apiGet(path) {
  let res;
  try {
    res = await fetch(`${API_BASE}${path}`);
  } catch {
    throw new Error('⚠ 后端未连接 — 请先启动 server.exe (localhost:8888)');
  }
  if (!res.ok) throw new Error(`HTTP ${res.status}`);
  return res.json();
}

async function apiUpload(path, formData) {
  let res;
  try {
    res = await fetch(`${API_BASE}${path}`, { method: 'POST', body: formData });
  } catch {
    throw new Error('⚠ 后端未连接 — 请先启动 server.exe (localhost:8888)');
  }
  if (!res.ok) throw new Error(`HTTP ${res.status}`);
  return res.json();
}

async function apiUploadRaw(path, blob) {
  let res;
  try {
    res = await fetch(`${API_BASE}${path}`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/octet-stream' },
      body: blob,
    });
  } catch {
    throw new Error('后端未连接，请先启动 server.exe (localhost:8888)');
  }
  if (!res.ok) {
    const text = await res.text().catch(() => '');
    throw new Error(`HTTP ${res.status}${text ? ': ' + text : ''}`);
  }
  return res.json();
}

async function apiDownloadFile(path, filename) {
  let res;
  try {
    res = await fetch(`${API_BASE}${path}`);
  } catch {
    throw new Error('⚠ 后端未连接 — 请先启动 server.exe (localhost:8888)');
  }
  if (!res.ok) {
    const text = await res.text().catch(() => '');
    throw new Error(`下载失败 HTTP ${res.status}${text ? ': ' + text : ''}`);
  }
  const blob = await res.blob();
  const url = window.URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = filename || 'downloaded_file';
  document.body.appendChild(a);
  a.click();
  a.remove();
  window.URL.revokeObjectURL(url);
}

async function apiGetBinary(path) {
  let res;
  try {
    res = await fetch(`${API_BASE}${path}`);
  } catch {
    throw new Error('⚠ 后端未连接 — 请先启动 server.exe (localhost:8888)');
  }
  if (!res.ok) throw new Error(`HTTP ${res.status}`);
  return res.arrayBuffer();
}
