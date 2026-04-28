function StreamCipher({ apiShow }) {
  const { useState } = React;
  const [method, setMethod] = useState('RC4');
  const [mode, setMode] = useState('encrypt');
  const [seed, setSeed] = useState('53 65 63 72 65 74 4B 65 79');
  const [text, setText] = useState('Hello, Cryptology!');
  const [result, setResult] = useState('');
  const [keystream, setKeystream] = useState('');
  const [loading, setLoading] = useState(false);

  async function run() {
    setLoading(true);
    setResult('');
    setKeystream('');
    try {
      const json = await apiCall(`/stream/${method.toLowerCase()}/${mode}`, {
        seed,
        [mode === 'encrypt' ? 'plaintext' : 'ciphertext']: text
      });
      if (mode === 'encrypt') {
        setResult(json.ciphertext ?? '');
        setKeystream(json.keystream ?? '');
      } else {
        setResult(json.plaintext ?? '');
      }
    } catch (e) {
      setResult(e.message);
    } finally {
      setLoading(false);
    }
  }

  return (
    <div className="col">
      <SectionHeader title="流密码加解密" badge="Stream Cipher"
        subtitle="支持 RC4 算法与 LFSR + J-K 触发器两种密钥流生成方式" />
      <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 20 }}>
        <div className="col">
          <div>
            <MonoLabel>明文 / 密文</MonoLabel>
            <textarea className="inp mono" rows={4} value={text} onChange={e => setText(e.target.value)} />
          </div>
          <div>
            <MonoLabel>种子密钥 (HEX bytes)</MonoLabel>
            <input className="inp mono" value={seed} onChange={e => setSeed(e.target.value)} placeholder="e.g. 53 65 63 72 65 74" />
          </div>
          <div className="grid-2">
            <div>
              <MonoLabel>算法</MonoLabel>
              <div className="toggle-group">
                <button className={`toggle-btn${method === 'RC4' ? ' active' : ''}`} onClick={() => setMethod('RC4')}>RC4</button>
                <button className={`toggle-btn${method === 'LFSR' ? ' active' : ''}`} onClick={() => setMethod('LFSR')}>LFSR</button>
              </div>
            </div>
            <div>
              <MonoLabel>操作</MonoLabel>
              <div className="toggle-group">
                <button className={`toggle-btn${mode === 'encrypt' ? ' active' : ''}`} onClick={() => setMode('encrypt')}>加密</button>
                <button className={`toggle-btn${mode === 'decrypt' ? ' active' : ''}`} onClick={() => setMode('decrypt')}>解密</button>
              </div>
            </div>
          </div>
          <button className="btn btn-primary" onClick={run} disabled={loading} style={{ justifyContent: 'center' }}>
            {loading ? '请求中...' : `▶  执行 ${method} ${mode === 'encrypt' ? '加密' : '解密'}`}
          </button>
          {apiShow && (
            <div style={{ display: 'flex', flexDirection: 'column', gap: 4 }}>
              <APIEndpoint method="POST" path="/stream/rc4/encrypt" />
              <APIEndpoint method="POST" path="/stream/rc4/decrypt" />
              <APIEndpoint method="POST" path="/stream/lfsr/encrypt" />
              <APIEndpoint method="POST" path="/stream/lfsr/decrypt" />
            </div>
          )}
        </div>
        <div className="col">
          <div>
            <MonoLabel>XML 种子密钥文件</MonoLabel>
            <FileDropZone label="seed.xml" accept=".xml,.key"
              onFile={c => { const m = c.match(/<seed>([^<]+)<\/seed>/); if (m) setSeed(m[1].trim()); }} />
            <div style={{ marginTop: 8, fontSize: 12, color: 'rgba(0,0,0,0.35)', fontFamily: 'var(--mono)' }}>
              {'&lt;seed&gt;53 65 63 72 65 74 4B 65 79&lt;/seed&gt;'}
            </div>
          </div>
          {mode === 'encrypt' && keystream && <ResultBox value={keystream} label="密钥流 (HEX)" />}
          <ResultBox value={result} label="结果" />
        </div>
      </div>
    </div>
  );
}
