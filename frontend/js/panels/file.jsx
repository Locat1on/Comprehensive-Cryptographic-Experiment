function LargeFileTransfer({ apiShow }) {
  const { useState } = React;
  const [file, setFile] = useState(null);
  const [cipher, setCipher] = useState('SHA1-CTR');
  const [progress, setProgress] = useState(0);
  const [transferring, setTransferring] = useState(false);
  const [log, setLog] = useState([]);
  const [result, setResult] = useState(null);

  const chunkSize = 4 * 1024 * 1024;

  function addLog(msg) {
    setLog(l => [...l.slice(-80), { t: timestamp(), msg }]);
  }

  function fileSizeText(size) {
    if (size >= 1024 ** 3) return `${(size / 1024 ** 3).toFixed(2)} GB`;
    if (size >= 1024 ** 2) return `${(size / 1024 ** 2).toFixed(1)} MB`;
    if (size >= 1024) return `${(size / 1024).toFixed(1)} KB`;
    return `${size} B`;
  }

  async function startTransfer() {
    if (!file) { addLog('[ERROR] 请先选择文件'); return; }
    setTransferring(true);
    setProgress(0);
    setLog([]);
    setResult(null);

    try {
      addLog(`[INIT] ${file.name}, ${fileSizeText(file.size)}, chunk=${fileSizeText(chunkSize)}`);
      const init = await apiCall('/file/init', {
        name: file.name,
        size: file.size,
        cipher,
        chunkSize,
      });

      addLog(`[DH] client=${init.clientPubKey}, server=${init.serverPubKey}, key fp=${init.keyFingerprint}`);
      addLog(`[RSA] handshake signature=${init.signature}`);

      const total = init.totalChunks;
      let lastLogged = -1;
      for (let i = 0; i < total; i++) {
        const start = i * chunkSize;
        const end = Math.min(file.size, start + chunkSize);
        const chunk = file.slice(start, end);
        const uploaded = await apiUploadRaw(`/file/chunk/${init.id}/${i}`, chunk);
        const pct = Math.floor(((i + 1) * 100) / total);
        setProgress(pct);
        if (pct >= lastLogged + 5 || i === total - 1) {
          addLog(`[CHUNK] ${i + 1}/${total}, progress=${pct}%, hash=${uploaded.chunkHash.slice(0, 12)}...`);
          lastLogged = pct;
        }
      }

      const done = await apiCall(`/file/complete/${init.id}`, {});
      setProgress(100);
      setResult(done);
      addLog(`[DONE] transfer id=${done.id}, chunks=${done.chunks}`);
      addLog(`[HASH] transfer digest=${done.hash}`);
      addLog(`[SIGN] RSA signature=${done.signature}`);
    } catch (e) {
      addLog(`[ERROR] ${e.message}`);
    } finally {
      setTransferring(false);
    }
  }

  return (
    <div className="col">
      <SectionHeader title="大文件加密传输" badge="1GB+ Secure Transfer"
        subtitle="浏览器按 4MB 分片上传，后端用 D-H 会话密钥派生流加密分片并落盘，SHA-1 校验完整性，RSA 签名摘要" />
      <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 20 }}>
        <div className="col">
          <div>
            <MonoLabel>选择文件</MonoLabel>
            <div className={`file-zone${file ? ' has-file' : ''}`}
              onClick={() => document.getElementById('big-file-inp').click()}>
              <input id="big-file-inp" type="file" style={{ display: 'none' }} onChange={e => setFile(e.target.files[0])} />
              <div style={{ fontSize: 13, fontWeight: 600 }}>{file ? file.name : '点击选择任意大小文件'}</div>
              <div style={{ fontSize: 12, color: 'rgba(0,0,0,0.5)', marginTop: 4 }}>
                {file ? fileSizeText(file.size) : '支持 1GB+，不会一次性读入内存'}
              </div>
            </div>
          </div>
          <div>
            <MonoLabel>分片加密模式</MonoLabel>
            <select className="sel" value={cipher} onChange={e => setCipher(e.target.value)}>
              <option>SHA1-CTR</option>
              <option>SHA1-CTR-DH</option>
            </select>
          </div>
          <div style={{ background: 'rgba(1,1,32,0.04)', border: '1px solid var(--border-light)', borderRadius: 'var(--r-md)', padding: 14 }}>
            {[
              { label: '分片大小', value: '4 MB / chunk' },
              { label: '摘要算法', value: 'SHA-1' },
              { label: '签名算法', value: 'RSA' },
              { label: '会话密钥', value: 'D-H' },
            ].map(r => (
              <div key={r.label} style={{ display: 'grid', gridTemplateColumns: '1fr auto', gap: 8, fontSize: 13, alignItems: 'center', marginBottom: 4 }}>
                <span style={{ fontFamily: 'var(--mono)', fontSize: 11, color: 'rgba(0,0,0,0.55)', textTransform: 'uppercase' }}>{r.label}</span>
                <span style={{ fontWeight: 600, color: '#010120', fontFamily: 'var(--mono)', fontSize: 12 }}>{r.value}</span>
              </div>
            ))}
          </div>
          <button className="btn btn-accent" onClick={startTransfer} disabled={transferring} style={{ justifyContent: 'center' }}>
            {transferring ? '传输中...' : '开始加密传输'}
          </button>
          {result && (
            <a className="btn btn-outline" style={{ justifyContent: 'center', textDecoration: 'none' }}
              href={`${API_BASE}/file/download/${result.id}`} target="_blank">
              下载后端加密文件
            </a>
          )}
          {apiShow && (
            <div style={{ display: 'flex', flexDirection: 'column', gap: 4 }}>
              <APIEndpoint method="POST" path="/file/init" />
              <APIEndpoint method="POST" path="/file/chunk/:id/:index" />
              <APIEndpoint method="POST" path="/file/complete/:id" />
              <APIEndpoint method="GET" path="/file/status/:id" />
              <APIEndpoint method="GET" path="/file/download/:id" />
            </div>
          )}
        </div>
        <div className="col">
          {(progress > 0 || transferring) && (
            <div className="result-appear col" style={{ gap: 10 }}>
              <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
                <MonoLabel>传输进度</MonoLabel>
                <span style={{ fontFamily: 'var(--mono)', fontSize: 13, fontWeight: 600 }}>{Math.min(100, progress).toFixed(0)}%</span>
              </div>
              <div className="progress-track">
                <div className="progress-fill" style={{ width: `${Math.min(100, progress)}%` }} />
              </div>
            </div>
          )}
          <div style={{ background: 'rgba(1,1,32,0.04)', border: '1px solid var(--border-light)', borderRadius: 'var(--r-md)', padding: 16, flex: 1, overflow: 'auto', maxHeight: 340 }}>
            <MonoLabel>传输日志</MonoLabel>
            <div style={{ fontFamily: 'var(--mono)', fontSize: 12, lineHeight: 1.9, marginTop: 6 }}>
              {log.length === 0 && <div style={{ color: 'rgba(0,0,0,0.5)' }}>等待传输启动...</div>}
              {log.map((l, i) => (
                <div key={i} className={i === log.length - 1 ? 'result-appear' : ''} style={{
                  color: l.msg.includes('[DONE]') ? '#00944a'
                    : l.msg.includes('[ERROR]') ? '#c54000' : 'rgba(0,0,0,0.65)'
                }}>
                  <span style={{ color: 'rgba(0,0,0,0.3)' }}>[{l.t}]</span> {l.msg}
                </div>
              ))}
            </div>
          </div>
          {result && <ResultBox value={`${result.hash}\n${result.signature}`} label="SHA-1 摘要 / RSA 签名" />}
        </div>
      </div>
    </div>
  );
}
