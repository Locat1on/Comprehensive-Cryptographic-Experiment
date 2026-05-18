function LargeFileTransfer({ apiShow }) {
  const { useState, useEffect, useRef } = React;
  const [activeTab, setActiveTab] = useState('send'); // 'send' or 'receive'

  // Send state
  const [file, setFile] = useState(null);
  const [progress, setProgress] = useState(0);
  const [transferring, setTransferring] = useState(false);
  const [log, setLog] = useState([]);
  const [transferId, setTransferId] = useState(null);
  const [senderDH, setSenderDH] = useState(null);
  const [sharedKey, setSharedKey] = useState(null);

  // Receive state
  const [receiveId, setReceiveId] = useState('');
  const [receiveInfo, setReceiveInfo] = useState(null);
  const [receiveLog, setReceiveLog] = useState([]);
  const [downloading, setDownloading] = useState(false);
  const [receiverDH, setReceiverDH] = useState(null);
  const [receiveProgress, setReceiveProgress] = useState(0);

  const chunkSize = 4 * 1024 * 1024;
  const pollRef = useRef(null);

  function addLog(msg) {
    setLog(l => [...l.slice(-80), { t: timestamp(), msg }]);
  }

  function addReceiveLog(msg) {
    setReceiveLog(l => [...l.slice(-80), { t: timestamp(), msg }]);
  }

  function fileSizeText(size) {
    if (size >= 1024 ** 3) return `${(size / 1024 ** 3).toFixed(2)} GB`;
    if (size >= 1024 ** 2) return `${(size / 1024 ** 2).toFixed(1)} MB`;
    if (size >= 1024) return `${(size / 1024).toFixed(1)} KB`;
    return `${size} B`;
  }

  // --- 发送逻辑 ---

  async function startTransfer() {
    if (!file) { addLog('[ERROR] 请先选择文件'); return; }
    setTransferring(true);
    setProgress(0);
    setLog([]);
    setTransferId(null);
    setSharedKey(null);

    try {
      // 1. 生成 DH 密钥对
      const dh = generateDHKeyPair();
      setSenderDH(dh);
      addLog(`[DH] 生成本地密钥对: pub=${dh.pub}`);

      // 2. 初始化传输
      addLog(`[INIT] ${file.name}, ${fileSizeText(file.size)}`);
      const init = await apiCall('/file/init', {
        name: file.name,
        size: file.size,
        pubKey: dh.pub.toString(),
        chunkSize,
      });

      setTransferId(init.id);
      addLog(`[WAIT] 传输已创建 ID=${init.id}，等待接收方加入...`);

      // 3. 开始轮询接收方加入
      pollReceiver(init.id, dh);
    } catch (e) {
      addLog(`[ERROR] ${e.message}`);
      setTransferring(false);
    }
  }

  function pollReceiver(id, dh) {
    const timer = setInterval(async () => {
      try {
        const status = await apiGet(`/file/status/${id}`);
        if (status.status === 'paired' || status.status === 'transferring' || status.status === 'done') {
          clearInterval(timer);
          addLog(`[DH] 接收方已加入，公钥=${status.receiverPubKey}`);
          
          // 计算共享密钥
          const key = computeSharedKey(status.receiverPubKey, dh.priv);
          const fp = await sha1Hex(key.toString());
          setSharedKey(key);
          addLog(`[DH] 共享密钥协商完成，指纹=${fp.slice(0, 16)}`);
          
          // 开始加密上传
          performUpload(id, key);
        }
      } catch (e) {
        addLog(`[ERROR] 轮询失败: ${e.message}`);
        clearInterval(timer);
        setTransferring(false);
      }
    }, 2000);
    pollRef.current = timer;
  }

  async function performUpload(id, key) {
    try {
      const total = Math.ceil(file.size / chunkSize);
      let lastLogged = -1;

      for (let i = 0; i < total; i++) {
        const start = i * chunkSize;
        const end = Math.min(file.size, start + chunkSize);
        const chunk = await file.slice(start, end).arrayBuffer();
        
        // JS 端加密
        const encrypted = await xorCrypt(chunk, key.toString(), id, i);
        
        // 上传密文
        const uploaded = await apiUploadRaw(`/file/chunk/${id}/${i}`, new Blob([encrypted]));
        
        const pct = Math.floor(((i + 1) * 100) / total);
        setProgress(pct);
        if (pct >= lastLogged + 5 || i === total - 1) {
          addLog(`[CHUNK] ${i + 1}/${total}, progress=${pct}%, encrypted_hash=${uploaded.encryptedHash.slice(0, 12)}...`);
          lastLogged = pct;
        }
      }

      await apiCall(`/file/complete/${id}`, {});
      addLog(`[DONE] 端到端加密传输完成！`);
    } catch (e) {
      addLog(`[ERROR] 上传失败: ${e.message}`);
    } finally {
      setTransferring(false);
    }
  }

  // --- 接收逻辑 ---

  async function handleJoin() {
    if (!receiveId.trim()) { addReceiveLog('[ERROR] 请输入传输ID'); return; }
    setReceiveLog([]);
    setReceiveInfo(null);
    setDownloading(false);

    try {
      // 1. 生成 DH 密钥对
      const dh = generateDHKeyPair();
      setReceiverDH(dh);
      addReceiveLog(`[DH] 生成本地密钥对: pub=${dh.pub}`);

      // 2. 加入传输
      addReceiveLog(`[JOIN] 加入传输 ID: ${receiveId}`);
      const result = await apiCall(`/file/join/${receiveId}`, {
        pubKey: dh.pub.toString()
      });

      setReceiveInfo(result);
      addReceiveLog(`[DH] 发送方公钥: ${result.senderPubKey}`);
      
      const key = computeSharedKey(result.senderPubKey, dh.priv);
      const fp = await sha1Hex(key.toString());
      addReceiveLog(`[DH] 共享密钥协商完成，指纹=${fp.slice(0, 16)}`);

      // 3. 开始轮询上传进度
      pollUploadProgress(receiveId);
    } catch (e) {
      addReceiveLog(`[ERROR] ${e.message}`);
    }
  }

  function pollUploadProgress(id) {
    const timer = setInterval(async () => {
      try {
        const status = await apiGet(`/file/status/${id}`);
        const pct = Math.floor((status.receivedChunks * 100) / status.totalChunks);
        setReceiveProgress(pct);
        
        if (status.status === 'done') {
          clearInterval(timer);
          addReceiveLog(`[READY] 发送方已上传完成，可以解密下载`);
        }
      } catch (e) {
        clearInterval(timer);
      }
    }, 2000);
    pollRef.current = timer;
  }

  async function handleDecryptDownload() {
    if (!receiveInfo || !receiverDH) return;
    setDownloading(true);
    addReceiveLog(`[DOWNLOAD] 开始拉取分片并解密: ${receiveInfo.fileName}`);
    
    try {
      const key = computeSharedKey(receiveInfo.senderPubKey, receiverDH.priv);
      const chunks = [];
      const total = receiveInfo.totalChunks;

      for (let i = 0; i < total; i++) {
        // 下载密文分片
        const encrypted = await apiGetBinary(`/file/pull/${receiveId}/${i}`);
        
        // JS 端解密
        const decrypted = await xorCrypt(encrypted, key.toString(), receiveId, i);
        chunks.push(decrypted);
        
        const pct = Math.floor(((i + 1) * 100) / total);
        setReceiveProgress(pct);
        if (i % 5 === 0 || i === total - 1) {
          addReceiveLog(`[DECRYPT] 进度 ${pct}% (${i + 1}/${total})`);
        }
      }

      // 组装并触发下载
      const blob = new Blob(chunks, { type: 'application/octet-stream' });
      const url = window.URL.createObjectURL(blob);
      const a = document.createElement('a');
      a.href = url;
      a.download = receiveInfo.fileName;
      document.body.appendChild(a);
      a.click();
      a.remove();
      window.URL.revokeObjectURL(url);
      
      addReceiveLog(`[DONE] 解密下载完成！`);
    } catch (e) {
      addReceiveLog(`[ERROR] 下载解密失败: ${e.message}`);
    } finally {
      setDownloading(false);
    }
  }

  useEffect(() => {
    return () => { if (pollRef.current) clearInterval(pollRef.current); };
  }, []);

  return (
    <div className="col">
      <SectionHeader title="局域网 E2E 加密传输" badge="End-to-End Secure"
        subtitle="基于浏览器端 DH 密钥交换与 JS 流加密，服务端仅做密文中继，无法解密文件" />

      <div style={{ display: 'flex', gap: 10, marginBottom: 16 }}>
        <button className={`btn ${activeTab === 'send' ? 'btn-accent' : 'btn-outline'}`} onClick={() => setActiveTab('send')}>发送文件 (Sender)</button>
        <button className={`btn ${activeTab === 'receive' ? 'btn-accent' : 'btn-outline'}`} onClick={() => setActiveTab('receive')}>接收文件 (Receiver)</button>
      </div>

      {activeTab === 'send' && (
        <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 20 }}>
          <div className="col">
            <div>
              <MonoLabel>选择文件</MonoLabel>
              <div className={`file-zone${file ? ' has-file' : ''}`}
                onClick={() => document.getElementById('e2e-file-inp').click()}>
                <input id="e2e-file-inp" type="file" style={{ display: 'none' }} onChange={e => setFile(e.target.files[0])} />
                <div style={{ fontSize: 13, fontWeight: 600, color: '#010120' }}>{file ? file.name : '点击选择文件'}</div>
                <div style={{ fontSize: 12, color: 'rgba(0,0,0,0.8)', marginTop: 4 }}>
                  {file ? fileSizeText(file.size) : '支持超大文件，端到端加密'}
                </div>
              </div>
            </div>
            
            <div style={{ background: 'rgba(1,1,32,0.04)', border: '1px solid var(--border-light)', borderRadius: 'var(--r-md)', padding: 14 }}>
              {[
                { label: '密钥交换', value: 'D-H (Browser)' },
                { label: '加密算法', value: 'XOR Stream (JS)' },
                { label: '安全级别', value: 'End-to-End' },
                { label: '中继角色', value: 'Storage Only' },
              ].map(r => (
                <div key={r.label} style={{ display: 'grid', gridTemplateColumns: '1fr auto', gap: 8, fontSize: 13, alignItems: 'center', marginBottom: 4 }}>
                  <span style={{ fontFamily: 'var(--mono)', fontSize: 11, color: 'rgba(0,0,0,0.8)', textTransform: 'uppercase' }}>{r.label}</span>
                  <span style={{ fontWeight: 600, color: '#010120', fontFamily: 'var(--mono)', fontSize: 12 }}>{r.value}</span>
                </div>
              ))}
            </div>

            <button className="btn btn-accent" onClick={startTransfer} disabled={transferring} style={{ justifyContent: 'center' }}>
              {transferring ? '处理中...' : '创建加密传输'}
            </button>

            {transferId && (
              <div className="result-appear" style={{ marginTop: 10, padding: 16, background: '#f6f8fa', border: '1px dashed var(--border-light)', borderRadius: 'var(--r-md)' }}>
                <div style={{ fontSize: 13, color: '#010120', marginBottom: 8, fontWeight: 500 }}>传输 ID (告知接收方):</div>
                <div style={{ display: 'flex', alignItems: 'center', gap: 10 }}>
                  <code style={{ fontSize: 18, fontWeight: 'bold', color: '#b31d28', background: '#fff', padding: '4px 8px', borderRadius: 4, flex: 1, border: '1px solid #e1e4e8' }}>{transferId}</code>
                  <button className="btn btn-outline" onClick={() => navigator.clipboard.writeText(transferId)}>复制</button>
                </div>
              </div>
            )}
          </div>

          <div className="col">
            {(progress > 0 || transferring) && (
              <div className="result-appear col" style={{ gap: 10 }}>
                <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
                  <MonoLabel>上传进度</MonoLabel>
                  <span style={{ fontFamily: 'var(--mono)', fontSize: 13, fontWeight: 600 }}>{progress}%</span>
                </div>
                <div className="progress-track">
                  <div className="progress-fill" style={{ width: `${progress}%` }} />
                </div>
              </div>
            )}
            <div style={{ background: 'rgba(1,1,32,0.04)', border: '1px solid var(--border-light)', borderRadius: 'var(--r-md)', padding: 16, flex: 1, overflow: 'auto', maxHeight: 340 }}>
              <MonoLabel>发送日志</MonoLabel>
              <div style={{ fontFamily: 'var(--mono)', fontSize: 12, lineHeight: 1.9, marginTop: 6 }}>
                {log.length === 0 && <div style={{ color: 'rgba(0,0,0,0.5)' }}>等待操作...</div>}
                {log.map((l, i) => (
                  <div key={i} style={{ color: l.msg.includes('[DONE]') ? '#00944a' : l.msg.includes('[ERROR]') ? '#c54000' : 'rgba(0,0,0,0.85)' }}>
                    <span style={{ color: 'rgba(0,0,0,0.5)' }}>[{l.t}]</span> {l.msg}
                  </div>
                ))}
              </div>
            </div>
          </div>
        </div>
      )}

      {activeTab === 'receive' && (
        <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 20 }}>
          <div className="col">
            <div>
              <MonoLabel>传输 ID</MonoLabel>
              <div style={{ display: 'flex', gap: 10 }}>
                <input className="inp" style={{ flex: 1 }} value={receiveId} onChange={e => setReceiveId(e.target.value)} placeholder="输入发送方的 ID" />
                <button className="btn btn-outline" onClick={handleJoin}>加入</button>
              </div>
            </div>

            {receiveInfo && (
              <div className="result-appear" style={{ background: 'rgba(1,1,32,0.04)', border: '1px solid var(--border-light)', borderRadius: 'var(--r-md)', padding: 14, marginTop: 10 }}>
                <MonoLabel>待接收文件</MonoLabel>
                {[
                  { label: '文件名', value: receiveInfo.fileName },
                  { label: '大小', value: fileSizeText(receiveInfo.fileSize) },
                  { label: '分片数', value: receiveInfo.totalChunks },
                  { label: '当前进度', value: `${receiveProgress}%` },
                ].map(r => (
                  <div key={r.label} style={{ display: 'grid', gridTemplateColumns: '80px auto', gap: 8, fontSize: 13, marginBottom: 6 }}>
                    <span style={{ color: '#010120' }}>{r.label}</span>
                    <span style={{ fontWeight: 600 }}>{r.value}</span>
                  </div>
                ))}
                
                <button className="btn btn-accent" onClick={handleDecryptDownload} disabled={downloading || receiveProgress < 100} style={{ width: '100%', justifyContent: 'center', marginTop: 16 }}>
                  {downloading ? '下载并解密中...' : '下载并解密'}
                </button>
              </div>
            )}
          </div>

          <div className="col">
            {(receiveProgress > 0) && (
              <div className="result-appear col" style={{ gap: 10 }}>
                <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
                  <MonoLabel>{receiveProgress === 100 ? '文件已就绪' : '发送方进度'}</MonoLabel>
                  <span style={{ fontFamily: 'var(--mono)', fontSize: 13, fontWeight: 600 }}>{receiveProgress}%</span>
                </div>
                <div className="progress-track">
                  <div className="progress-fill" style={{ width: `${receiveProgress}%` }} />
                </div>
              </div>
            )}
            <div style={{ background: 'rgba(1,1,32,0.04)', border: '1px solid var(--border-light)', borderRadius: 'var(--r-md)', padding: 16, flex: 1, overflow: 'auto', maxHeight: 340 }}>
              <MonoLabel>接收日志</MonoLabel>
              <div style={{ fontFamily: 'var(--mono)', fontSize: 12, lineHeight: 1.9, marginTop: 6 }}>
                {receiveLog.length === 0 && <div style={{ color: 'rgba(0,0,0,0.5)' }}>等待加入传输...</div>}
                {receiveLog.map((l, i) => (
                  <div key={i} style={{ color: l.msg.includes('[DONE]') ? '#00944a' : l.msg.includes('[ERROR]') ? '#c54000' : 'rgba(0,0,0,0.85)' }}>
                    <span style={{ color: 'rgba(0,0,0,0.5)' }}>[{l.t}]</span> {l.msg}
                  </div>
                ))}
              </div>
            </div>
          </div>
        </div>
      )}
    </div>
  );
}