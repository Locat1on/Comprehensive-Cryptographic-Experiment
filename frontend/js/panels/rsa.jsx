function RSACipher({ apiShow }) {
  const { useState } = React;
  const [p, setP] = useState('61');
  const [q, setQ] = useState('53');
  const [e, setE] = useState('17');
  const [msg, setMsg] = useState('Hello RSA');
  const [keys, setKeys] = useState(null);
  const [encrypted, setEncrypted] = useState(null);
  const [result, setResult] = useState('');
  const [loading, setLoading] = useState(false);

  async function genKeys() {
    setLoading(true);
    setResult('');
    setEncrypted(null);
    try {
      const json = await apiCall('/rsa/keygen', { p: parseInt(p), q: parseInt(q), e: parseInt(e) });
      setKeys(json);
      setResult('');
    } catch (err) {
      setResult(err.message);
    } finally {
      setLoading(false);
    }
  }

  async function encrypt() {
    if (!keys) { setResult('请先生成密钥'); return; }
    setLoading(true);
    setResult('');
    try {
      const json = await apiCall('/rsa/encrypt', { message: msg, key: { n: keys.n, e: keys.e } });
      setEncrypted(json.blocks);
      setResult(`密文 (分块): ${Array.isArray(json.blocks) ? json.blocks.join(' | ') : json.blocks}`);
    } catch (err) {
      setResult(err.message);
    } finally {
      setLoading(false);
    }
  }

  async function decrypt() {
    if (!keys || !encrypted) { setResult('请先加密'); return; }
    setLoading(true);
    try {
      const json = await apiCall('/rsa/decrypt', { blocks: encrypted, key: { n: keys.n, d: keys.d } });
      setResult(`解密结果: ${json.message ?? JSON.stringify(json)}`);
    } catch (err) {
      setResult(err.message);
    } finally {
      setLoading(false);
    }
  }

  return (
    <div className="col">
      <SectionHeader title="RSA 非对称加密" badge="Asymmetric Cipher"
        subtitle="自构造 RSA — 模数 n &lt; 16bit，支持大于 16bit 消息的分块加密" />
      <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 20 }}>
        <div className="col">
          <div className="grid-3">
            <div><MonoLabel>素数 p</MonoLabel><input className="inp mono" value={p} onChange={e => setP(e.target.value)} type="number" /></div>
            <div><MonoLabel>素数 q</MonoLabel><input className="inp mono" value={q} onChange={e => setQ(e.target.value)} type="number" /></div>
            <div><MonoLabel>公钥指数 e</MonoLabel><input className="inp mono" value={e} onChange={e2 => setE(e2.target.value)} type="number" /></div>
          </div>
          <button className="btn btn-outline" onClick={genKeys} disabled={loading} style={{ justifyContent: 'center' }}>
            {loading ? '请求中...' : '生成密钥对'}
          </button>
          {keys && (
            <div className="result-appear" style={{ background: 'rgba(1,1,32,0.04)', border: '1px solid var(--border-light)', borderRadius: 'var(--r-md)', padding: 16 }}>
              <div style={{ fontFamily: 'var(--mono)', fontSize: 12, lineHeight: 1.8 }}>
                <div>n = <strong>{keys.n}</strong>  |  φ(n) = <strong>{keys.phi}</strong></div>
                <div style={{ color: '#5b57d1' }}>公钥: (e={keys.e}, n={keys.n})</div>
                <div style={{ color: '#c54000' }}>私钥: (d={keys.d}, n={keys.n})</div>
              </div>
            </div>
          )}
          <div>
            <MonoLabel>消息</MonoLabel>
            <textarea className="inp" rows={3} value={msg} onChange={e => setMsg(e.target.value)} />
          </div>
          <div style={{ display: 'flex', gap: 8 }}>
            <button className="btn btn-primary" style={{ flex: 1, justifyContent: 'center' }} onClick={encrypt} disabled={loading}>▶ 加密</button>
            <button className="btn btn-outline" style={{ flex: 1, justifyContent: 'center' }} onClick={decrypt} disabled={loading || !encrypted}>◀ 解密</button>
          </div>
          {apiShow && (
            <div style={{ display: 'flex', flexDirection: 'column', gap: 4 }}>
              <APIEndpoint method="POST" path="/rsa/keygen" />
              <APIEndpoint method="POST" path="/rsa/encrypt" />
              <APIEndpoint method="POST" path="/rsa/decrypt" />
            </div>
          )}
        </div>
        <div className="col">
          <div style={{ background: 'rgba(1,1,32,0.03)', border: '1px solid var(--border-light)', borderRadius: 'var(--r-md)', padding: 20 }}>
            <MonoLabel>分块加密方案</MonoLabel>
            <div style={{ display: 'flex', flexDirection: 'column', gap: 12, marginTop: 12 }}>
              {[
                { step: '01', title: '密钥生成', desc: '选取素数 p、q，计算 n=p×q，φ(n)=(p-1)(q-1)，选 e 满足 gcd(e,φ)=1，求 d≡e⁻¹(mod φ)' },
                { step: '02', title: '分块策略', desc: 'n < 65536，消息按字节切分，每字节 m 独立计算 c=mᵉ mod n' },
                { step: '03', title: '解密', desc: '逐块计算 m=cᵈ mod n，拼接还原原始消息' },
              ].map(s => (
                <div key={s.step} style={{ display: 'flex', gap: 12, alignItems: 'flex-start' }}>
                  <div style={{ width: 28, height: 28, borderRadius: 'var(--r-sm)', background: 'var(--dark)', color: '#bdbbff', fontFamily: 'var(--mono)', fontSize: 10, fontWeight: 600, display: 'flex', alignItems: 'center', justifyContent: 'center', flexShrink: 0 }}>{s.step}</div>
                  <div>
                    <div style={{ fontSize: 13, fontWeight: 600, letterSpacing: '-0.02em', marginBottom: 2, color: '#010120' }}>{s.title}</div>
                    <div style={{ fontSize: 12, color: 'rgba(0,0,0,0.75)', lineHeight: 1.5 }}>{s.desc}</div>
                  </div>
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
