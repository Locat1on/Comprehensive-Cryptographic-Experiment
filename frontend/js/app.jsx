const { useState, useEffect } = React;

const SERVICES = [
  { id: 'affine', group: '古典密码', label: '仿射加密',       sub: 'Affine Cipher',    icon: 'Af' },
  { id: 'bigint', group: '基础运算', label: '大整数运算',      sub: '128-bit BigInt',   icon: 'Bi' },
  { id: 'stream', group: '流密码',   label: '流密码加密',      sub: 'RC4 / LFSR+JK',   icon: 'SC' },
  { id: 'des',    group: '对称加密', label: 'DES 加密',        sub: 'Block Cipher',     icon: 'DE' },
  { id: 'rsa',    group: '非对称',   label: 'RSA 加密',        sub: 'Public Key',       icon: 'RS' },
  { id: 'dh',     group: '协议',     label: 'D-H 认证协议',    sub: 'Key Exchange',     icon: 'DH' },
  { id: 'hash',   group: '完整性',   label: '哈希 + 数字签名', sub: 'SHA-1 / RSA Sign', icon: 'HS' },
  { id: 'file',   group: '传输',     label: '大文件加密传输',  sub: '1GB+ Secure TX',  icon: 'FT' },
];

const PANELS = {
  affine: AffineCipher,
  bigint: BigIntOps,
  stream: StreamCipher,
  des:    DESCipher,
  rsa:    RSACipher,
  dh:     DHProtocol,
  hash:   HashSign,
  file:   LargeFileTransfer,
};

function App() {
  const [active, setActive]   = useState(localStorage.getItem('crypto_service') || 'affine');
  const [apiShow, setApiShow] = useState(false);
  const [sidebarW, setSidebarW] = useState(270);

  useEffect(() => { localStorage.setItem('crypto_service', active); }, [active]);

  useEffect(() => {
    window.addEventListener('message', e => {
      if (e.data?.type === '__activate_edit_mode')   document.getElementById('tweaks-panel').classList.add('open');
      if (e.data?.type === '__deactivate_edit_mode') document.getElementById('tweaks-panel').classList.remove('open');
    });
    window.parent.postMessage({ type: '__edit_mode_available' }, '*');
    document.getElementById('tw-api-on').classList.toggle('active', apiShow);
    document.getElementById('tw-api-off').classList.toggle('active', !apiShow);
    document.getElementById('tw-sidebar').value = sidebarW;
    window.toggleAPI = v => {
      setApiShow(v);
      document.getElementById('tw-api-on').classList.toggle('active', v);
      document.getElementById('tw-api-off').classList.toggle('active', !v);
    };
    window.setDensity = (d, el) => {
      document.querySelectorAll('.toggle-btn').forEach(b => b.classList.remove('active'));
      el.classList.add('active');
      document.documentElement.style.setProperty('--density-gap', d === 'compact' ? '8px' : '12px');
    };
    document.getElementById('tw-sidebar').addEventListener('input', e => setSidebarW(+e.target.value));
  }, []);

  const Panel = PANELS[active];
  const svc   = SERVICES.find(s => s.id === active);

  const groups = {};
  SERVICES.forEach(s => { if (!groups[s.group]) groups[s.group] = []; groups[s.group].push(s); });

  return (
    <div style={{ display: 'flex', height: '100%', overflow: 'hidden', width: '100%' }}>
      {/* Sidebar */}
      <div style={{ width: sidebarW, flexShrink: 0, background: 'var(--dark)', display: 'flex', flexDirection: 'column', borderRight: '1px solid rgba(255,255,255,0.06)', overflow: 'hidden', position: 'relative' }}>
        <div className="blob" style={{ width: 200, height: 200, background: 'var(--magenta)', top: -60, left: -60 }} />
        <div className="blob" style={{ width: 160, height: 160, background: 'var(--lavender)', bottom: 80, right: -40 }} />
        <div style={{ padding: '24px 20px 20px', borderBottom: '1px solid rgba(255,255,255,0.06)', position: 'relative' }}>
          <div style={{ fontFamily: 'var(--mono)', fontSize: 11, letterSpacing: '0.1em', textTransform: 'uppercase', color: 'rgba(255,255,255,0.35)', marginBottom: 8 }}>PLATFORM</div>
          <div style={{ fontSize: 20, fontWeight: 600, letterSpacing: '-0.04em', lineHeight: 1 }}>CRYPTOLOGY</div>
          <div style={{ fontSize: 12, color: 'rgba(255,255,255,0.35)', marginTop: 6 }}>加解密综合服务平台</div>
          <div style={{ marginTop: 12 }}>
            <div className="status ok" style={{ padding: '2px 7px' }}><span className="dot" />运行中</div>
          </div>
        </div>
        <div style={{ flex: 1, overflow: 'auto', padding: '12px 8px' }}>
          {Object.entries(groups).map(([group, items]) => (
            <div key={group} style={{ marginBottom: 20 }}>
              <div style={{ fontFamily: 'var(--mono)', fontSize: 11, letterSpacing: '0.08em', textTransform: 'uppercase', color: 'rgba(255,255,255,0.4)', padding: '0 12px', marginBottom: 6 }}>{group}</div>
              {items.map(s => (
                <button key={s.id} onClick={() => setActive(s.id)}
                  style={{ width: '100%', display: 'flex', alignItems: 'center', gap: 10, padding: '9px 12px', borderRadius: 'var(--r-sm)', border: 'none', cursor: 'pointer', textAlign: 'left', background: active === s.id ? 'rgba(255,255,255,0.1)' : 'transparent', color: active === s.id ? '#fff' : 'rgba(255,255,255,0.55)', transition: 'all 0.15s', marginBottom: 1 }}>
                  <div style={{ width: 34, height: 34, borderRadius: 'var(--r-sm)', flexShrink: 0, background: active === s.id ? 'rgba(255,255,255,0.15)' : 'rgba(255,255,255,0.06)', display: 'flex', alignItems: 'center', justifyContent: 'center', fontFamily: 'var(--mono)', fontSize: 11, fontWeight: 600 }}>{s.icon}</div>
                  <div style={{ flex: 1, minWidth: 0 }}>
                    <div style={{ fontSize: 14, fontWeight: 500, letterSpacing: '-0.02em', lineHeight: 1.2, whiteSpace: 'nowrap', overflow: 'hidden', textOverflow: 'ellipsis' }}>{s.label}</div>
                    <div style={{ fontSize: 11, fontFamily: 'var(--mono)', color: 'rgba(255,255,255,0.3)', marginTop: 2 }}>{s.sub}</div>
                  </div>
                  {active === s.id && <div style={{ width: 3, height: 20, background: 'var(--lavender)', borderRadius: 2, flexShrink: 0 }} />}
                </button>
              ))}
            </div>
          ))}
        </div>
      </div>

      {/* Main */}
      <div style={{ flex: 1, overflow: 'auto', background: '#fafafa', display: 'flex', flexDirection: 'column' }}>
        <div style={{ borderBottom: '1px solid var(--border-light)', padding: '14px 32px', display: 'flex', alignItems: 'center', justifyContent: 'space-between', background: '#fff', position: 'sticky', top: 0, zIndex: 10 }}>
          <div style={{ display: 'flex', alignItems: 'center', gap: 12 }}>
            <div style={{ width: 32, height: 32, borderRadius: 'var(--r-sm)', background: 'var(--dark)', display: 'flex', alignItems: 'center', justifyContent: 'center', fontFamily: 'var(--mono)', fontSize: 11, fontWeight: 600, color: '#fff' }}>{svc?.icon}</div>
            <div>
              <div style={{ fontSize: 15, fontWeight: 600, letterSpacing: '-0.03em', color: '#010120' }}>{svc?.label}</div>
              <div style={{ fontSize: 12, fontFamily: 'var(--mono)', color: 'rgba(0,0,0,0.55)', letterSpacing: '0.04em', textTransform: 'uppercase' }}>{svc?.group}</div>
            </div>
          </div>
          {apiShow && (
            <div style={{ fontFamily: 'var(--mono)', fontSize: 11, color: 'rgba(0,0,0,0.35)', padding: '4px 10px', border: '1px solid var(--border-light)', borderRadius: 'var(--r-sm)' }}>
              /api/v1/{active}
            </div>
          )}
        </div>
        <div style={{ flex: 1, padding: '36px 40px', maxWidth: 1400, width: '100%', boxSizing: 'border-box' }}>
          <Panel apiShow={apiShow} />
        </div>
      </div>
    </div>
  );
}

ReactDOM.createRoot(document.getElementById('root')).render(<App />);
