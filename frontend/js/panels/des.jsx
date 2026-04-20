function DESCipher({ apiShow }) {
  const { useState } = React;
  const [key, setKey] = useState('133457799BBCDFF1');
  const [plaintext, setPlaintext] = useState('0123456789ABCDEF');
  const [mode, setMode] = useState('encrypt');
  const [cipherMode, setCipherMode] = useState('ECB');
  const [result, setResult] = useState('');
  const [loading, setLoading] = useState(false);

  async function run() {
    setLoading(true);
    setResult('');
    try {
      const json = await apiCall(`/des/${mode}`, {
        data: plaintext.replace(/\s/g, ''),
        key: key.replace(/\s/g, ''),
        mode: cipherMode,
      });
      setResult(json.result ?? JSON.stringify(json));
    } catch (e) {
      setResult(e.message);
    } finally {
      setLoading(false);
    }
  }

  return (
    <div className="col">
      <SectionHeader title="DES 对称加密" badge="Symmetric Cipher"
        subtitle="数据加密标准 — 64-bit 分组，56-bit 有效密钥，16 轮 Feistel 结构" />
      <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 20 }}>
        <div className="col">
          <div>
            <MonoLabel>DES 密钥 (64-bit HEX)</MonoLabel>
            <input className="inp mono" value={key} onChange={e => setKey(e.target.value)} placeholder="16个HEX字符" />
          </div>
          <div>
            <MonoLabel>XML 密钥文件</MonoLabel>
            <FileDropZone label="des-key.xml" accept=".xml,.key"
              onFile={c => { const m = c.match(/<key>([^<]+)<\/key>/); if (m) setKey(m[1].trim()); }} />
          </div>
          <div>
            <MonoLabel>明文 / 密文 (HEX)</MonoLabel>
            <textarea className="inp mono" rows={3} value={plaintext} onChange={e => setPlaintext(e.target.value)} />
          </div>
          <div className="grid-2">
            <div>
              <MonoLabel>操作</MonoLabel>
              <div className="toggle-group">
                <button className={`toggle-btn${mode === 'encrypt' ? ' active' : ''}`} onClick={() => setMode('encrypt')}>加密</button>
                <button className={`toggle-btn${mode === 'decrypt' ? ' active' : ''}`} onClick={() => setMode('decrypt')}>解密</button>
              </div>
            </div>
            <div>
              <MonoLabel>模式</MonoLabel>
              <div className="toggle-group">
                <button className={`toggle-btn${cipherMode === 'ECB' ? ' active' : ''}`} onClick={() => setCipherMode('ECB')}>ECB</button>
                <button className={`toggle-btn${cipherMode === 'CBC' ? ' active' : ''}`} onClick={() => setCipherMode('CBC')}>CBC</button>
              </div>
            </div>
          </div>
          <button className="btn btn-primary" onClick={run} disabled={loading} style={{ justifyContent: 'center' }}>
            {loading ? '请求中...' : `▶  执行 DES ${mode === 'encrypt' ? '加密' : '解密'}`}
          </button>
          {apiShow && (
            <div style={{ display: 'flex', flexDirection: 'column', gap: 4 }}>
              <APIEndpoint method="POST" path="/des/encrypt" />
              <APIEndpoint method="POST" path="/des/decrypt" />
            </div>
          )}
        </div>
        <div className="col">
          <div style={{ background: 'rgba(1,1,32,0.03)', border: '1px solid var(--border-light)', borderRadius: 'var(--r-md)', padding: 20 }}>
            <MonoLabel>Feistel 结构</MonoLabel>
            <div style={{ display: 'flex', flexDirection: 'column', gap: 12, marginTop: 12 }}>
              {[
                { step: 'IP', desc: '初始置换 (64-bit 输入)' },
                { step: '×16', desc: '每轮: L(i)=R(i-1), R(i)=L(i-1)⊕f(R(i-1),Kᵢ)', accent: true },
                { step: 'f()', desc: 'E扩展 → XOR子密钥 → 8个S盒 → P置换' },
                { step: 'IP⁻¹', desc: '逆置换输出 64-bit 密文' },
              ].map(s => (
                <div key={s.step} style={{ display: 'flex', gap: 12, alignItems: 'flex-start' }}>
                  <div style={{ minWidth: 36, height: 28, borderRadius: 'var(--r-sm)', background: s.accent ? 'var(--dark)' : 'rgba(1,1,32,0.08)', color: s.accent ? '#bdbbff' : '#010120', fontFamily: 'var(--mono)', fontSize: 10, fontWeight: 600, display: 'flex', alignItems: 'center', justifyContent: 'center', flexShrink: 0, padding: '0 4px' }}>{s.step}</div>
                  <div style={{ fontSize: 12, color: 'rgba(0,0,0,0.75)', lineHeight: 1.6, paddingTop: 5 }}>{s.desc}</div>
                </div>
              ))}
            </div>
          </div>
          <ResultBox value={result} label="结果" />
        </div>
      </div>
    </div>
  );
}
