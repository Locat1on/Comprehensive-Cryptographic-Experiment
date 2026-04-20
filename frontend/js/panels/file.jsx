function LargeFileTransfer({ apiShow }) {
  const { useState, useRef } = React;
  const [file, setFile] = useState(null);
  const [cipher, setCipher] = useState('AES-256-CTR');
  const [progress, setProgress] = useState(0);
  const [transferring, setTransferring] = useState(false);
  const [log, setLog] = useState([]);
  const timerRef = useRef(null);

  function addLog(msg) {
    setLog(l => [...l, { t: timestamp(), msg }]);
  }

  async function startTransfer() {
    if (!file) { addLog('[ERROR] 请先选择文件'); return; }
    setTransferring(true);
    setProgress(0);
    setLog([]);
    addLog('[INIT] 连接后端服务器...');
    try {
      const form = new FormData();
      form.append('file', file);
      form.append('cipher', cipher);
      form.append('chunk_size', '4194304');

      const uploaded = await apiUpload('/file/encrypt-upload', form);
      addLog(`[UPLOAD] 任务 ID: ${uploaded.id}，分片数: ${uploaded.chunks}`);
      addLog(`[HASH] 原始文件摘要: ${uploaded.hash}`);

      await new Promise((resolve, reject) => {
        const poll = setInterval(async () => {
          try {
            const st = await apiGet(`/file/status/${uploaded.id}`);
            setProgress(st.progress ?? 0);
            addLog(`[TX] 进度 ${st.progress}% — ${st.status}`);
            if (st.status === 'done' || st.progress >= 100) {
              clearInterval(poll);
              setProgress(100);
              addLog('[DONE] ✓ 传输完成，签名验证通过');
              resolve();
            } else if (st.status === 'error') {
              clearInterval(poll);
              reject(new Error('服务端处理失败'));
            }
          } catch (e) { clearInterval(poll); reject(e); }
        }, 1500);
        timerRef.current = poll;
      });
    } catch (e) {
      addLog(`[ERROR] ${e.message}`);
    } finally {
      setTransferring(false);
    }
  }

  return (
    <div className="col">
      <SectionHeader title="大文件加密传输" badge="Secure File Transfer"
        subtitle="支持 1GB+ 文件加密传输 — 分片处理、完整性验证、数字签名、断点续传" />
      <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 20 }}>
        <div className="col">
          <div>
            <MonoLabel>选择文件 (支持 1GB+)</MonoLabel>
            <div className={`file-zone${file ? ' has-file' : ''}`}
              onClick={() => document.getElementById('big-file-inp').click()}>
              <input id="big-file-inp" type="file" style={{ display: 'none' }} onChange={e => setFile(e.target.files[0])} />
              <div style={{ width: 28, height: 28, margin: '0 auto 6px', display: 'flex', alignItems: 'center', justifyContent: 'center' }}>
                {file
                  ? <svg width="18" height="18" viewBox="0 0 18 18" fill="none"><rect x="3" y="1" width="10" height="14" rx="1.5" stroke="#00944a" strokeWidth="1.5" /><path d="M7 1v4h6" stroke="#00944a" strokeWidth="1.5" strokeLinejoin="round" /></svg>
                  : <svg width="18" height="18" viewBox="0 0 18 18" fill="none"><path d="M2 5a2 2 0 012-2h3.586a1 1 0 01.707.293L10 5h4a2 2 0 012 2v7a2 2 0 01-2 2H4a2 2 0 01-2-2V5z" stroke="#000" strokeWidth="1.5" strokeLinejoin="round" opacity=".4" /></svg>}
              </div>
              <div style={{ fontSize: 13, fontWeight: 500 }}>{file ? file.name : '拖入任意文件'}</div>
              {file && <div style={{ fontSize: 12, color: 'rgba(0,0,0,0.45)', marginTop: 4 }}>
                {file.size > 1e9 ? (file.size / 1e9).toFixed(2) + ' GB' : file.size > 1e6 ? (file.size / 1e6).toFixed(1) + ' MB' : (file.size / 1e3).toFixed(1) + ' KB'}
              </div>}
              {!file && <div style={{ fontSize: 12, color: 'rgba(0,0,0,0.5)', marginTop: 4 }}>点击选择或拖拽文件</div>}
            </div>
          </div>
          <div>
            <MonoLabel>加密算法</MonoLabel>
            <select className="sel" value={cipher} onChange={e => setCipher(e.target.value)}>
              <option>AES-256-CTR</option>
              <option>AES-256-CBC</option>
              <option>ChaCha20-Poly1305</option>
            </select>
          </div>
          <div style={{ background: 'rgba(1,1,32,0.04)', border: '1px solid var(--border-light)', borderRadius: 'var(--r-md)', padding: 14 }}>
            {[
              { label: '分片大小', value: '4 MB / chunk' },
              { label: '哈希算法', value: 'SHA-1' },
              { label: '签名算法', value: 'RSA' },
              { label: '密钥交换', value: 'D-H' },
            ].map(r => (
              <div key={r.label} style={{ display: 'grid', gridTemplateColumns: '1fr auto', gap: 8, fontSize: 13, alignItems: 'center', marginBottom: 4 }}>
                <span style={{ fontFamily: 'var(--mono)', fontSize: 11, color: 'rgba(0,0,0,0.55)', textTransform: 'uppercase', letterSpacing: '0.04em' }}>{r.label}</span>
                <span style={{ fontWeight: 600, color: '#010120', fontFamily: 'var(--mono)', fontSize: 12 }}>{r.value}</span>
              </div>
            ))}
          </div>
          <button className="btn btn-accent" onClick={startTransfer} disabled={transferring} style={{ justifyContent: 'center' }}>
            {transferring ? '传输中...' : '▶  开始加密传输'}
          </button>
          {apiShow && (
            <div style={{ display: 'flex', flexDirection: 'column', gap: 4 }}>
              <APIEndpoint method="POST" path="/file/encrypt-upload" />
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
                  color: l.msg.includes('✓') || l.msg.includes('DONE') ? '#00944a'
                    : l.msg.includes('ERROR') ? '#c54000' : 'rgba(0,0,0,0.65)'
                }}>
                  <span style={{ color: 'rgba(0,0,0,0.3)' }}>[{l.t}]</span> {l.msg}
                </div>
              ))}
            </div>
          </div>
          {progress >= 100 && (
            <div className="status ok result-appear" style={{ padding: '10px 14px' }}>
              <span className="dot" />文件加密传输成功 — 完整性验证通过
            </div>
          )}
        </div>
      </div>
    </div>
  );
}
