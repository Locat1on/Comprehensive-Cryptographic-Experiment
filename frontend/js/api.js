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
    throw new Error('⚠ 后端未连接 — 请先启动 server.exe (localhost:8080)');
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
    throw new Error('⚠ 后端未连接 — 请先启动 server.exe (localhost:8080)');
  }
  if (!res.ok) throw new Error(`HTTP ${res.status}`);
  return res.json();
}

async function apiUpload(path, formData) {
  let res;
  try {
    res = await fetch(`${API_BASE}${path}`, { method: 'POST', body: formData });
  } catch {
    throw new Error('⚠ 后端未连接 — 请先启动 server.exe (localhost:8080)');
  }
  if (!res.ok) throw new Error(`HTTP ${res.status}`);
  return res.json();
}
