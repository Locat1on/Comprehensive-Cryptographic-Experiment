function StreamCipher({ apiShow }) {
  const { useState } = React;
  const [method, setMethod] = useState('RC4');
  const [seed, setSeed] = useState('53 65 63 72 65 74 4B 65 79');
  const [plaintext, setPlaintext] = useState('Hello, Cryptology!');
  const [result, setResult] = useState('');
  const [keystream, setKeystream] = useState('');
  const [loading, setLoading] = useState(false);

  async function run() {
    setLoading(true);
    setResult('');
    setKeystream('');
    try {
      const path = method === 'RC4' ? '/stream/rc4' : '/stream/lfsr';
      const json = await apiCall(path, { seed, plaintext });
      setResult(json.ciphertext ?? '');
      setKeystream(json.keystream ?? '');
    } catch (e) {
      setResult(e.message);
    } finally {
      setLoading(false);
    }
  }

  return (
    <div className="col">
      <SectionHeader title="流密码加密" badge="Stream Cipher"
        subtitle="支持 RC4 算法与 LFSR + J-K 触发器两种密钥流生成方式" />
      <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 28, alignItems: 'start' }}>
        <div className="col">
          <div>
            <MonoLabel>密钥流生成方式</MonoLabel>
            <div className="toggle-group">
              <button className={`toggle-btn${method === 'RC4' ? ' active' : ''}`} onClick={() => setMethod('RC4')}>RC4</button>
              <button className={`toggle-btn${method === 'LFSR' ? ' active' : ''}`} onClick={() => setMethod('LFSR')}>LFSR + J-K</button>
            </div>
          </div>
          <div>
            <MonoLabel>种子密钥 (HEX bytes)</MonoLabel>
            <input className="inp mono" value={seed} onChange={e => setSeed(e.target.value)} placeholder="e.g. 53 65 63 72 65 74" />
          </div>
          <div>
            <MonoLabel>XML 种子密钥文件</MonoLabel>
            <FileDropZone label="seed.xml" accept=".xml,.key"
              onFile={c => { const m = c.match(/<seed>([^<]+)<\/seed>/); if (m) setSeed(m[1].trim()); }} />
          </div>
          <div>
            <MonoLabel>明文</MonoLabel>
            <textarea className="inp mono" rows={3} value={plaintext} onChange={e => setPlaintext(e.target.value)} />
          </div>
          <button className="btn btn-primary" onClick={run} disabled={loading} style={{ justifyContent: 'center' }}>
            {loading ? '请求中...' : '▶  加密'}
          </button>
          {apiShow && (
            <div style={{ display: 'flex', flexDirection: 'column', gap: 4 }}>
              <APIEndpoint method="POST" path="/stream/rc4" show={method === 'RC4'} />
              <APIEndpoint method="POST" path="/stream/lfsr" show={method === 'LFSR'} />
            </div>
          )}
        </div>
        <div className="col">
          <div style={{ background: 'rgba(1,1,32,0.03)', border: '1px solid var(--border-light)', borderRadius: 'var(--r-md)', padding: 20 }}>
            <MonoLabel>加密流程</MonoLabel>
            <div style={{ display: 'flex', flexDirection: 'column', gap: 16, marginTop: 12 }}>
              {(method === 'RC4' ? [
                { step: '01', title: 'KSA', desc: '用种子密钥置换初始化 256-byte S 盒' },
                { step: '02', title: 'PRGA', desc: '流式生成伪随机密钥字节序列' },
                { step: '03', title: 'XOR', desc: '密钥流与明文逐字节异或得密文' },
              ] : [
                { step: '01', title: 'LFSR', desc: '线性反馈移位寄存器，多项式 f(x)=x⁸+x⁶+x⁴+x+1' },
                { step: '02', title: 'J-K 触发器', desc: '触发器位 [0,2,4,7] 引入非线性增强安全性' },
                { step: '03', title: 'XOR', desc: '密钥流与明文逐字节异或得密文' },
              ]).map(st => (
                <div key={st.step} style={{ display: 'flex', gap: 12, alignItems: 'flex-start' }}>
                  <div style={{ width: 28, height: 28, borderRadius: 'var(--r-sm)', background: 'var(--dark)', color: '#bdbbff', fontFamily: 'var(--mono)', fontSize: 10, fontWeight: 600, display: 'flex', alignItems: 'center', justifyContent: 'center', flexShrink: 0 }}>{st.step}</div>
                  <div>
                    <div style={{ fontSize: 13, fontWeight: 600, letterSpacing: '-0.02em', marginBottom: 2, color: '#010120' }}>{st.title}</div>
                    <div style={{ fontSize: 12, color: 'rgba(0,0,0,0.75)', lineHeight: 1.5 }}>{st.desc}</div>
                  </div>
                </div>
              ))}
            </div>
          </div>
          {keystream && <ResultBox value={keystream} label="密钥流 (HEX)" />}
          <ResultBox value={result} label="密文 (HEX)" />
        </div>
      </div>
    </div>
  );
}
