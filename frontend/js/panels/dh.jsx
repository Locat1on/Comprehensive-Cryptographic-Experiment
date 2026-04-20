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

  async function startExchange() {
    setLoading(true);
    setLog([]);
    setState(null);
    addLog(`协议初始化: p=${p}, g=${g}`);
    try {
      const init = await apiCall('/dh/init', { p, g });
      addLog(`服务端公钥 B = ${init.pubKey}`, '#5b57d1');
      setState(s => ({ ...s, ...init, step: 1 }));
      addLog('客户端公钥 A 已发送至服务端');

      const exch = await apiCall('/dh/exchange', { pubKey: init.pubKey, p, g });
      addLog(`共享密钥 K = ${exch.sharedKey}`, '#00944a');
      setState(s => ({ ...s, ...exch, step: 2 }));

      const verify = await apiCall('/dh/verify', { signature: init.signature ?? '', hash: exch.sharedKey ?? '' });
      addLog(verify.ok ? '✓ 签名验证通过，密钥协商成功！' : '✗ 签名验证失败', verify.ok ? '#00944a' : '#c54000');
      setState(s => ({ ...s, verified: verify.ok, step: 3 }));
    } catch (e) {
      addLog(`错误: ${e.message}`, '#c54000');
    } finally {
      setLoading(false);
    }
  }

  return (
    <div className="col">
      <SectionHeader title="D-H 密钥交换协议" badge="Key Exchange + Authentication"
        subtitle="基于 Diffie-Hellman 的双实体认证协议 — C/S 模式，含消息完整性与来源验证" />
      <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 20 }}>
        <div className="col">
          <div className="grid-2">
            <div><MonoLabel>大素数 p</MonoLabel><input className="inp" value={p} onChange={e => setP(e.target.value)} type="number" /></div>
            <div><MonoLabel>生成元 g</MonoLabel><input className="inp" value={g} onChange={e => setG(e.target.value)} type="number" /></div>
          </div>
          <button className="btn btn-accent" onClick={startExchange} disabled={loading} style={{ justifyContent: 'center' }}>
            {loading ? '协商中...' : '▶  开始密钥交换'}
          </button>
          {state && (
            <div className="result-appear" style={{ display: 'flex', gap: 6, flexWrap: 'wrap' }}>
              {['初始化', '公钥交换', '共享密钥', '验证'].map((label, i) => (
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
          <div style={{ background: 'rgba(1,1,32,0.04)', border: '1px solid var(--border-light)', borderRadius: 'var(--r-md)', padding: 16, flex: 1, overflow: 'auto', maxHeight: 260 }}>
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
              {state.verified ? '密钥协商成功 — 签名验证通过' : '签名验证失败'}
            </div>
          )}
        </div>
      </div>
    </div>
  );
}
