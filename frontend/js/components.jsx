const { useState, useRef } = React;

function MonoLabel({ children, dark }) {
  return <span className={`label${dark ? ' label-dark' : ''}`}>{children}</span>;
}

function APIEndpoint({ method, path, show = true }) {
  if (!show) return null;
  return (
    <div className="api-badge light" style={{ marginTop: 6 }}>
      <span className="method">{method}</span>
      <span>/api/v1{path}</span>
    </div>
  );
}

function ResultBox({ value, label = 'OUTPUT', dark = false }) {
  const [copied, setCopied] = useState(false);
  if (!value) return null;
  return (
    <div className="result-appear">
      <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', marginBottom: 6 }}>
        <MonoLabel dark={dark}>{label}</MonoLabel>
        <button
          className={`btn btn-sm ${dark ? 'btn-ghost-dark' : 'btn-outline'}`}
          onClick={() => { copyText(value); setCopied(true); setTimeout(() => setCopied(false), 1500); }}>
          {copied ? 'Copied!' : 'Copy'}
        </button>
      </div>
      <div className={`code-out${dark ? '' : ' light'}`}>{value}</div>
    </div>
  );
}

function FileDropZone({ label = 'XML / KEY FILE', accept = '.xml,.key,.txt', onFile }) {
  const ref = useRef();
  const [fileName, setFileName] = useState(null);
  function handle(file) {
    if (!file) return;
    setFileName(file.name);
    const r = new FileReader();
    r.onload = e => onFile?.(e.target.result, file.name);
    r.readAsText(file);
  }
  return (
    <div
      className={`file-zone${fileName ? ' has-file' : ''}`}
      onClick={() => { ref.current.value = ''; ref.current.click(); }}
      onDragOver={e => e.preventDefault()}
      onDrop={e => { e.preventDefault(); ref.current.value = ''; handle(e.dataTransfer.files[0]); }}>
      <input ref={ref} type="file" accept={accept} style={{ display: 'none' }}
        onChange={e => handle(e.target.files[0])} />
      <div style={{ width: 28, height: 28, margin: '0 auto 6px', display: 'flex', alignItems: 'center', justifyContent: 'center' }}>
        {fileName
          ? <svg width="16" height="16" viewBox="0 0 16 16" fill="none"><path d="M2 8l4 4 8-8" stroke="#00944a" strokeWidth="2" strokeLinecap="round" /></svg>
          : <svg width="16" height="16" viewBox="0 0 16 16" fill="none"><path d="M8 2v9M4 7l4-5 4 5" stroke="#000" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" opacity=".6" /><path d="M2 13h12" stroke="#000" strokeWidth="1.5" strokeLinecap="round" opacity=".4" /></svg>}
      </div>
      <div style={{ fontSize: 13, fontWeight: 500, color: fileName ? '#00944a' : 'rgba(0,0,0,0.85)', letterSpacing: '-0.01em' }}>
        {fileName || label}
      </div>
      {!fileName && <div style={{ fontSize: 12, color: 'rgba(0,0,0,0.7)', marginTop: 4 }}>点击或拖拽上传</div>}
      {fileName && (
        <button
          className="btn btn-sm btn-outline"
          style={{ marginTop: 8 }}
          onClick={e => { e.stopPropagation(); setFileName(null); }}>
          重新上传
        </button>
      )}
    </div>
  );
}

function SectionHeader({ title, subtitle, badge }) {
  return (
    <div style={{ marginBottom: 32 }}>
      <div style={{ display: 'flex', alignItems: 'flex-start', justifyContent: 'space-between', flexWrap: 'wrap', gap: 12 }}>
        <div>
          {badge && <div style={{ fontFamily: 'var(--mono)', fontSize: 12, letterSpacing: '0.06em', textTransform: 'uppercase', color: 'rgba(0,0,0,0.6)', marginBottom: 10 }}>{badge}</div>}
          <h2 style={{ fontSize: 34, fontWeight: 600, letterSpacing: '-0.04em', color: '#010120', lineHeight: 1.1 }}>{title}</h2>
          {subtitle && <p style={{ fontSize: 16, color: 'rgba(0,0,0,0.75)', marginTop: 10, letterSpacing: '-0.01em', lineHeight: 1.5 }}>{subtitle}</p>}
        </div>
      </div>
    </div>
  );
}
