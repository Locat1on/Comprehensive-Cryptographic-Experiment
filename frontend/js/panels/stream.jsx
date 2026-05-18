function StreamCipher({ apiShow }) {
  const { useState } = React;
  const [method, setMethod] = useState('RC4');
  const [seed, setSeed] = useState('53 65 63 72 65 74 4B 65 79');
  const [plaintext, setPlaintext] = useState('Hello, Cryptology!');
  const [ciphertext, setCiphertext] = useState('');
  const [keystream, setKeystream] = useState('');
  const [result, setResult] = useState('');
  const [loading, setLoading] = useState(false);

  async function encrypt() {
    setLoading(true);
    setResult('');
    setKeystream('');
    try {
      const json = await apiCall(`/stream/${method.toLowerCase()}/encrypt`, { seed, plaintext });
      setCiphertext(json.ciphertext ?? '');
      setKeystream(json.keystream ?? '');
    } catch (e) {
      setResult(e.message);
    } finally {
      setLoading(false);
    }
  }

  async function decrypt() {
    setLoading(true);
    setResult('');
    setKeystream('');
    try {
      const json = await apiCall(`/stream/${method.toLowerCase()}/decrypt`, { seed, ciphertext });
      setResult(json.plaintext ?? '');
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
            <MonoLabel>种子密钥 (HEX bytes)</MonoLabel>
            <input className="inp mono" value={seed} onChange={e => setSeed(e.target.value)} placeholder="e.g. 53 65 63 72 65 74" />
          </div>
          <div>
            <MonoLabel>算法</MonoLabel>
            <div className="toggle-group">
              <button className={`toggle-btn${method === 'RC4' ? ' active' : ''}`} onClick={() => setMethod('RC4')}>RC4</button>
              <button className={`toggle-btn${method === 'LFSR' ? ' active' : ''}`} onClick={() => setMethod('LFSR')}>LFSR</button>
            </div>
          </div>
          <div>
            <MonoLabel>明文</MonoLabel>
            <textarea className="inp mono" rows={3} value={plaintext} onChange={e => setPlaintext(e.target.value)} />
          </div>
          <button className="btn btn-primary" onClick={encrypt} disabled={loading} style={{ justifyContent: 'center' }}>
            {loading ? '请求中...' : '▶  加密'}
          </button>
          <div>
            <MonoLabel>密文 (HEX)</MonoLabel>
            <textarea className="inp mono" rows={3} value={ciphertext} onChange={e => setCiphertext(e.target.value)} placeholder="加密后自动填入，或手动粘贴..." />
          </div>
          <button className="btn btn-outline" onClick={decrypt} disabled={loading} style={{ justifyContent: 'center' }}>
            {loading ? '请求中...' : '◀  解密'}
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
          {keystream && <ResultBox value={keystream} label="密钥流 (HEX)" />}
          <ResultBox value={result} label="解密结果" />
        </div>
      </div>
    </div>
  );
}
