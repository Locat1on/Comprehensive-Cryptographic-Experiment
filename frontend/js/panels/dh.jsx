function DHProtocol({ apiShow }) {
  const { useState } = React;
  const [p, setP] = useState('23');
  const [g, setG] = useState('5');
  const [log, setLog] = useState([]);
  const [state, setState] = useState(null);
  const [loading, setLoading] = useState(false);

  function addLog(msg, color) {
    setLog(l => [...l, { t: timestamp(), msg, color }]);
  }

  function modPow(base, exp, mod) {
    let b = BigInt(base) % BigInt(mod);
    let e = BigInt(exp);
    const m = BigInt(mod);
    let r = 1n;
    while (e > 0n) {
      if (e & 1n) r = (r * b) % m;
      b = (b * b) % m;
      e >>= 1n;
    }
    return r;
  }

  async function startExchange() {
    setLoading(true);
    setLog([]);
    setState(null);
    try {
      const pBig = BigInt(p);
      const gBig = BigInt(g);
      const clientPrivate = BigInt(2 + Math.floor(Math.random() * Math.max(2, Number(pBig - 3n))));
      const clientPub = modPow(gBig, clientPrivate, pBig).toString();
      const nonce = `${Date.now()}-${Math.random().toString(16).slice(2)}`;

      addLog(`Client: p=${p}, g=${g}, A=${clientPub}`);
      const init = await apiCall('/dh/init', { p, g, pubKey: clientPub, nonce });
      addLog(`Server: B=${init.pubKey}`);
      addLog(`Server signature: ${init.signature}`, '#5b57d1');
      setState({ ...init, clientPub, step: 1 });

      const exch = await apiCall('/dh/exchange', { sessionId: init.sessionId });
      const clientShared = modPow(BigInt(init.pubKey), clientPrivate, pBig).toString();
      addLog(`Client shared K=${clientShared}`);
      addLog(`Server shared K=${exch.sharedKey}`, '#00944a');
      setState(s => ({ ...s, ...exch, clientShared, step: 2 }));

      const verify = await apiCall('/dh/verify', { message: init.message, signature: init.signature });
      addLog(verify.ok ? 'RSA signature verified: source and integrity OK' : 'RSA signature verification failed', verify.ok ? '#00944a' : '#c54000');
      setState(s => ({ ...s, verified: verify.ok, step: 3 }));
    } catch (e) {
      addLog(`Error: ${e.message}`, '#c54000');
    } finally {
      setLoading(false);
    }
  }

  return (
    <div className="col">
      <SectionHeader title="D-H 认证协议" badge="C/S Authenticated Key Exchange"
        subtitle="Client/Server 两个实体通过 D-H 协商共享密钥，服务端用 RSA 签名握手消息，SHA-1 验证完整性" />
      <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 20 }}>
        <div className="col">
          <div className="grid-2">
            <div><MonoLabel>大素数 p</MonoLabel><input className="inp" value={p} onChange={e => setP(e.target.value)} type="number" /></div>
            <div><MonoLabel>生成元 g</MonoLabel><input className="inp" value={g} onChange={e => setG(e.target.value)} type="number" /></div>
          </div>
          <button className="btn btn-accent" onClick={startExchange} disabled={loading} style={{ justifyContent: 'center' }}>
            {loading ? '协商中...' : '开始 D-H 认证协商'}
          </button>
          {state && (
            <div className="result-appear" style={{ display: 'flex', gap: 6, flexWrap: 'wrap' }}>
              {['初始化', '密钥协商', '签名验证'].map((label, i) => (
                <div key={i} className="status" style={{
                  background: i < (state.step ?? 0) ? 'rgba(0,200,100,0.08)' : 'transparent',
                  borderColor: i < (state.step ?? 0) ? 'rgba(0,200,100,0.2)' : 'var(--border-light)',
                  color: i < (state.step ?? 0) ? '#00944a' : 'rgba(0,0,0,0.35)'
                }}>
                  <span className="dot" />S{i + 1} {label}
                </div>
              ))}
            </div>
          )}
          {apiShow && (
            <div style={{ display: 'flex', flexDirection: 'column', gap: 4 }}>
              <APIEndpoint method="POST" path="/dh/init" />
              <APIEndpoint method="POST" path="/dh/exchange" />
              <APIEndpoint method="POST" path="/dh/verify" />
            </div>
          )}
        </div>
        <div className="col">
          <div style={{ background: 'rgba(1,1,32,0.04)', border: '1px solid var(--border-light)', borderRadius: 'var(--r-md)', padding: 16, flex: 1, overflow: 'auto', maxHeight: 300 }}>
            <MonoLabel>通信日志</MonoLabel>
            <div style={{ fontFamily: 'var(--mono)', fontSize: 12, lineHeight: 1.8, marginTop: 6 }}>
              {log.length === 0 && <div style={{ color: 'rgba(0,0,0,0.5)' }}>等待协议启动...</div>}
              {log.map((l, i) => (
                <div key={i} style={{ color: l.color ?? 'rgba(0,0,0,0.65)' }}>
                  <span style={{ color: 'rgba(0,0,0,0.3)' }}>[{l.t}]</span> {l.msg}
                </div>
              ))}
            </div>
          </div>
          {state?.verified !== undefined && (
            <div className={`status result-appear ${state.verified ? 'ok' : 'warn'}`}>
              <span className="dot" />
              {state.verified ? '密钥协商成功，来源与完整性验证通过' : '签名验证失败'}
            </div>
          )}
        </div>
      </div>
    </div>
  );
}
