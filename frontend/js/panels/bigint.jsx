function BigIntOps({ apiShow }) {
  const { useState } = React;
  const [a, setA] = useState('340282366920938463463374607431768211455');
  const [b, setB] = useState('170141183460469231731687303715884105727');
  const [op, setOp] = useState('+');
  const [result, setResult] = useState('');
  const [hex, setHex] = useState('');
  const [loading, setLoading] = useState(false);

  async function run() {
    setLoading(true);
    setResult('');
    setHex('');
    try {
      const json = await apiCall('/bigint/calc', { a, b, op });
      setResult(json.result ?? '');
      setHex(json.hex ?? '');
    } catch (e) {
      setResult(e.message);
    } finally {
      setLoading(false);
    }
  }

  const ops = [
    { sym: '+', label: '加法 ADD' },
    { sym: '-', label: '减法 SUB' },
    { sym: '*', label: '乘法 MUL' },
  ];

  return (
    <div className="col">
      <SectionHeader title="128-bit 大整数运算" badge="Big Integer Arithmetic"
        subtitle="自建类实现两个 128-bit 整数的加、减、乘运算" />
      <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 20 }}>
        <div className="col">
          <div>
            <MonoLabel>操作数 A (128-bit)</MonoLabel>
            <input className="inp mono" value={a} onChange={e => setA(e.target.value)} placeholder="十进制整数" />
          </div>
          <div>
            <MonoLabel>操作数 B (128-bit)</MonoLabel>
            <input className="inp mono" value={b} onChange={e => setB(e.target.value)} placeholder="十进制整数" />
          </div>
          <div>
            <MonoLabel>运算类型</MonoLabel>
            <div style={{ display: 'flex', gap: 8 }}>
              {ops.map(o => (
                <button key={o.sym} className={`btn ${op === o.sym ? 'btn-primary' : 'btn-outline'}`}
                  style={{ flex: 1, justifyContent: 'center' }} onClick={() => setOp(o.sym)}>
                  {o.label}
                </button>
              ))}
            </div>
          </div>
          <button className="btn btn-primary" onClick={run} disabled={loading} style={{ justifyContent: 'center' }}>
            {loading ? '请求中...' : `▶  计算 A ${op} B`}
          </button>
          {apiShow && <APIEndpoint method="POST" path="/bigint/calc" />}
        </div>
        <div className="col">
          <div style={{ background: 'rgba(1,1,32,0.03)', border: '1px solid var(--border-light)', borderRadius: 'var(--r-md)', padding: 20 }}>
            <MonoLabel>运算说明</MonoLabel>
            <div style={{ display: 'flex', flexDirection: 'column', gap: 12, marginTop: 12 }}>
              {[
                { step: '+', title: '128-bit 加法', desc: '逐 64-bit 分块相加，低位进位传递至高位，结果最大 129-bit' },
                { step: '-', title: '128-bit 减法', desc: '被减数不足时借位，支持负数结果（补码表示）' },
                { step: '*', title: '128-bit 乘法', desc: 'Karatsuba 分治算法，结果最大 256-bit，自建 BigInt256 类承载' },
              ].map(st => (
                <div key={st.step} style={{ display: 'flex', gap: 12, alignItems: 'flex-start' }}>
                  <div style={{ width: 28, height: 28, borderRadius: 'var(--r-sm)', background: 'var(--dark)', color: '#bdbbff', fontFamily: 'var(--mono)', fontSize: 14, fontWeight: 700, display: 'flex', alignItems: 'center', justifyContent: 'center', flexShrink: 0 }}>{st.step}</div>
                  <div>
                    <div style={{ fontSize: 13, fontWeight: 600, letterSpacing: '-0.02em', marginBottom: 2, color: '#010120' }}>{st.title}</div>
                    <div style={{ fontSize: 12, color: 'rgba(0,0,0,0.75)', lineHeight: 1.5 }}>{st.desc}</div>
                  </div>
                </div>
              ))}
            </div>
          </div>
          {result && (
            <div className="result-appear col" style={{ gap: 8 }}>
              <ResultBox value={result} label="十进制结果" />
              {hex && <ResultBox value={hex} label="十六进制结果" />}
            </div>
          )}
        </div>
      </div>
    </div>
  );
}
