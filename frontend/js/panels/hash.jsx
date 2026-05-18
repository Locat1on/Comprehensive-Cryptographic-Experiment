function HashSign({ apiShow }) {
  const { useState } = React;
  const [p, setP] = useState('61');
  const [q, setQ] = useState('53');
  const [e, setE] = useState('17');
  const [signKey, setSignKey] = useState(null);
  const [algo, setAlgo] = useState('SHA1');
  const [msg, setMsg] = useState('Verify this message integrity');
  const [hash, setHash] = useState('');
  const [sig, setSig] = useState('');
  const [verified, setVerified] = useState(null);
  const [loading, setLoading] = useState(false);

  async function genKeys() {
    setLoading(true);
    setSignKey(null);
    setVerified(null);
    try {
      const json = await apiCall('/rsa/keygen', { p: parseInt(p), q: parseInt(q), e: parseInt(e) });
      setSignKey(json);
    } catch (err) {
      setHash(err.message);
    } finally {
      setLoading(false);
    }
  }

  async function computeHash() {
    setLoading(true);
    setHash('');
    try {
      const json = await apiCall('/hash/compute', { message: msg, algo });
      setHash(json.hash ?? '');
      return json.hash;
    } catch (e) {
      setHash(e.message);
      return null;
    } finally {
      setLoading(false);
    }
  }

  async function signMsg() {
    setLoading(true);
    setSig('');
    setVerified(null);
    try {
      const payload = { message: msg, algo };
      if (signKey) payload.key = { n: signKey.n, d: signKey.d, e: signKey.e };
      const json = await apiCall('/sign/rsa', payload);
      setHash(json.hash ?? '');
      setSig(json.signature ?? '');
    } catch (e) {
      setSig(e.message);
    } finally {
      setLoading(false);
    }
  }

  async function verifyMsg() {
    if (!sig.trim()) { setVerified(false); return; }
    setLoading(true);
    try {
      const payload = { message: msg, signature: sig, algo };
      if (signKey) payload.key = { n: signKey.n, e: signKey.e };
      const json = await apiCall('/sign/verify', payload);
      setHash(json.hash ?? '');
      setVerified(json.valid);
    } catch (e) {
      setSig(e.message);
      setVerified(false);
    } finally {
      setLoading(false);
    }
  }

  return (
    <div className="col">
      <SectionHeader title="哈希 + 数字签名" badge="Hash & Digital Signature"
        subtitle="SHA-1 / MD5 消息摘要 + RSA 数字签名，实现消息完整性与来源验证" />
      <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 28, alignItems: 'start' }}>
        <div className="col">
          <div className="grid-3">
            <div><MonoLabel>素数 p</MonoLabel><input className="inp mono" value={p} onChange={e => setP(e.target.value)} type="number" /></div>
            <div><MonoLabel>素数 q</MonoLabel><input className="inp mono" value={q} onChange={e => setQ(e.target.value)} type="number" /></div>
            <div><MonoLabel>公钥指数 e</MonoLabel><input className="inp mono" value={e} onChange={e2 => setE(e2.target.value)} type="number" /></div>
          </div>
          <button className="btn btn-outline" onClick={genKeys} disabled={loading} style={{ justifyContent: 'center' }}>
            {loading ? '请求中...' : '生成签名密钥对'}
          </button>
          {signKey && (
            <div className="result-appear" style={{ background: 'rgba(1,1,32,0.04)', border: '1px solid var(--border-light)', borderRadius: 'var(--r-md)', padding: 16 }}>
              <div style={{ fontFamily: 'var(--mono)', fontSize: 12, lineHeight: 1.8 }}>
                <div style={{ color: '#010120' }}>n = <strong>{signKey.n}</strong>  |  φ(n) = <strong>{signKey.phi}</strong></div>
                <div style={{ color: '#5b57d1' }}>公钥: (e={signKey.e}, n={signKey.n})</div>
                <div style={{ color: '#c54000' }}>私钥: (d={signKey.d}, n={signKey.n})</div>
              </div>
            </div>
          )}
          <div>
            <MonoLabel>散列算法</MonoLabel>
            <div className="toggle-group">
              {['SHA1', 'MD5'].map(a => (
                <button key={a} className={`toggle-btn${algo === a ? ' active' : ''}`} onClick={() => setAlgo(a)}>{a}</button>
              ))}
            </div>
          </div>
          <div>
            <MonoLabel>消息</MonoLabel>
            <textarea className="inp" rows={4} value={msg} onChange={e => setMsg(e.target.value)} />
          </div>
          <div style={{ display: 'flex', gap: 8 }}>
            <button className="btn btn-outline" style={{ flex: 1, justifyContent: 'center' }} onClick={computeHash} disabled={loading}>
              # 计算哈希
            </button>
            <button className="btn btn-primary" style={{ flex: 1, justifyContent: 'center' }} onClick={signMsg} disabled={loading}>
              ✎ 签名
            </button>
          </div>
          <div>
            <MonoLabel>数字签名 (HEX)</MonoLabel>
            <textarea className="inp mono" rows={3} value={sig} onChange={e => setSig(e.target.value)} placeholder="粘贴签名 hex 或点击「签名」生成..." />
          </div>
          <button className="btn btn-outline" style={{ justifyContent: 'center' }} onClick={verifyMsg} disabled={loading}>
            ✓ 验证签名
          </button>
          {apiShow && (
            <div style={{ display: 'flex', flexDirection: 'column', gap: 4 }}>
              <APIEndpoint method="POST" path="/hash/compute" />
              <APIEndpoint method="POST" path="/sign/rsa" />
              <APIEndpoint method="POST" path="/sign/verify" />
            </div>
          )}
        </div>
        <div className="col">
          <div style={{ background: 'rgba(1,1,32,0.03)', border: '1px solid var(--border-light)', borderRadius: 'var(--r-md)', padding: 20 }}>
            <MonoLabel>签名流程</MonoLabel>
            <div style={{ display: 'flex', flexDirection: 'column', gap: 12, marginTop: 12 }}>
              {[
                { step: '01', title: '生成密钥', desc: '选取素数 p、q 与公钥指数 e，生成签名用 RSA 密钥对 (d, n)' },
                { step: '02', title: '计算摘要', desc: `H = ${algo}(消息) — 固定长度指纹` },
                { step: '03', title: 'RSA 签名', desc: 'S = RSA_Sign(H, 私钥 d) — 发送方持有' },
                { step: '04', title: '接收验证', desc: `H' = ${algo}(消息), H'' = RSA_Verify(S, 公钥 e), 验证 H'==H''` },
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
          {hash && <ResultBox value={hash} label={`${algo} 摘要`} />}
          {sig && <ResultBox value={sig} label="RSA 数字签名 (HEX)" />}
          {verified !== null && (
            <div className={`status result-appear ${verified ? 'ok' : 'warn'}`}>
              <span className="dot" />
              {verified ? '签名验证通过 — 消息来源可信' : '签名验证失败 — 消息可能被篡改'}
            </div>
          )}
        </div>
      </div>
    </div>
  );
}
