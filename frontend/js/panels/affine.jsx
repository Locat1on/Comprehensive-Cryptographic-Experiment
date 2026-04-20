function AffineCipher({ apiShow }) {
  const { useState } = React;
  const [text, setText] = useState('HELLO WORLD');
  const [a, setA] = useState('7');
  const [b, setB] = useState('3');
  const [mode, setMode] = useState('encrypt');
  const [result, setResult] = useState('');
  const [loading, setLoading] = useState(false);

  async function run() {
    setLoading(true);
    setResult('');
    try {
      const body = mode === 'encrypt'
        ? { plaintext: text, a: parseInt(a), b: parseInt(b) }
        : { ciphertext: text, a: parseInt(a), b: parseInt(b) };
      const json = await apiCall(`/affine/${mode}`, body);
      setResult(json.ciphertext ?? json.plaintext ?? JSON.stringify(json));
    } catch (e) {
      setResult(e.message);
    } finally {
      setLoading(false);
    }
  }

  function parseKeyFile(content) {
    const ma = content.match(/<a>(\d+)<\/a>/);
    const mb = content.match(/<b>(\d+)<\/b>/);
    if (ma) setA(ma[1]);
    if (mb) setB(mb[1]);
  }

  return (
    <div className="col">
      <SectionHeader title="仿射加密" badge="Classical Cipher"
        subtitle="E(x) = (ax + b) mod 26  —  密钥参数可通过 XML 文件配置" />
      <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 20 }}>
        <div className="col">
          <div>
            <MonoLabel>明文 / 密文</MonoLabel>
            <textarea className="inp mono" rows={4} value={text} onChange={e => setText(e.target.value)} />
          </div>
          <div className="grid-2">
            <div>
              <MonoLabel>参数 a (与26互质)</MonoLabel>
              <select className="sel" value={a} onChange={e => setA(e.target.value)}>
                {[1, 3, 5, 7, 9, 11, 15, 17, 19, 21, 23, 25].map(v => <option key={v}>{v}</option>)}
              </select>
            </div>
            <div>
              <MonoLabel>参数 b (0–25)</MonoLabel>
              <input className="inp" type="number" min="0" max="25" value={b} onChange={e => setB(e.target.value)} />
            </div>
          </div>
          <div className="toggle-group">
            <button className={`toggle-btn${mode === 'encrypt' ? ' active' : ''}`} onClick={() => setMode('encrypt')}>加密 Encrypt</button>
            <button className={`toggle-btn${mode === 'decrypt' ? ' active' : ''}`} onClick={() => setMode('decrypt')}>解密 Decrypt</button>
          </div>
          <button className="btn btn-primary" style={{ justifyContent: 'center' }} onClick={run} disabled={loading}>
            {loading ? '请求中...' : (mode === 'encrypt' ? '▶  加密' : '▶  解密')}
          </button>
          {apiShow && (
            <div style={{ display: 'flex', flexDirection: 'column', gap: 4 }}>
              <APIEndpoint method="POST" path="/affine/encrypt" />
              <APIEndpoint method="POST" path="/affine/decrypt" />
            </div>
          )}
        </div>
        <div className="col">
          <div>
            <MonoLabel>XML 密钥文件</MonoLabel>
            <FileDropZone label="拖入 key.xml 配置文件" accept=".xml,.key" onFile={parseKeyFile} />
            <div style={{ marginTop: 8, fontSize: 12, color: 'rgba(0,0,0,0.35)', fontFamily: 'var(--mono)' }}>
              {'<!-- key.xml -->\n<key><a>7</a><b>3</b></key>'}
            </div>
          </div>
          <ResultBox value={result} label="结果" />
        </div>
      </div>
    </div>
  );
}
