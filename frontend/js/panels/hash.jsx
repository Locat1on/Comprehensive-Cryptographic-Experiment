function HashSign({ apiShow }) {
  const { useState } = React;
  const [algo, setAlgo] = useState('SHA1');
  const [msg, setMsg] = useState('Verify this message integrity');
  const [hash, setHash] = useState('');
  const [sig, setSig] = useState('');
  const [verified, setVerified] = useState(null);
  const [loading, setLoading] = useState(false);

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
      const json = await apiCall('/sign/rsa', { message: msg, algo });
      setHash(json.hash ?? '');
      setSig(json.signature ?? '');
    } catch (e) {
      setSig(e.message);
    } finally {
      setLoading(false);
    }
  }

  async function verifyMsg() {
    setLoading(true);
    try {
      const json = await apiCall('/sign/verify', { message: msg, signature: sig, algo });
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
          <div>
            <MonoLabel>散列算法</MonoLabel>
            <div className="toggle-group">
              {['SHA1', 'MD5'].map(a => (
                <button key={a} className={`toggle-btn${algo === a ? ' active' : ''}`} onClick={() => setAlgo(a)}>{a}</button>
              ))}
            </div>
          </div>
          <div>
            <MonoLabel>消息内容</MonoLabel>
            <textarea className="inp" rows={4} value={msg} onChange={e => setMsg(e.target.value)} />
          </div>
          <div style={{ display: 'flex', gap: 8 }}>
            <button className="btn btn-outline" style={{ flex: 1, justifyContent: 'center' }} onClick={computeHash} disabled={loading}>
              # 计算哈希
            </button>
            <button className="btn btn-primary" style={{ flex: 1, justifyContent: 'center' }} onClick={signMsg} disabled={loading}>
              ✎ 签名
            </button>
            <button className="btn btn-outline" style={{ flex: 1, justifyContent: 'center' }} onClick={verifyMsg} disabled={loading || !sig}>
              ✓ 验证
            </button>
          </div>
          {verified !== null && (
            <div className={`status result-appear ${verified ? 'ok' : 'warn'}`}>
              <span className="dot" />
              {verified ? '签名验证通过 — 消息来源可信' : '签名验证失败 — 消息可能被篡改'}
            </div>
          )}
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
                { step: '01', title: '计算摘要', desc: `H = ${algo}(消息) — 固定长度指纹` },
                { step: '02', title: 'RSA 签名', desc: 'S = RSA_Sign(H, 私钥 d) — 发送方持有' },
                { step: '03', title: '发送', desc: '传输 (消息, 签名 S) — 签名不可伪造' },
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
        </div>
      </div>
    </div>
  );
}
