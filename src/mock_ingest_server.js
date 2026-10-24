// Bench-test server for SAFE_LINK devices. No dependencies: node tools/mock_ingest_server.js
// Verifies the HMAC signature, rejects replays, prints a decoded line, and answers HTTP 200 (the device's acknowledgment).
// Per-device keys: set DEVICE_KEYS='{"WG-023":"<64 hex chars>"}'. Default key = the example key in src/config.h.
const http = require('http'), crypto = require('crypto');
const DEFAULT_KEY = '000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f';
const KEYS = process.env.DEVICE_KEYS ? JSON.parse(process.env.DEVICE_KEYS) : {};
const lastSeq = {};
const FLAGS = [[1,'SOS'],[2,'FALL'],[4,'INACTIVITY'],[8,'STRAP_OPEN'],[16,'LOW_BATT'],[32,'CRIT_BATT'],[64,'BOOT'],[128,'LATE']];
const ACT = ['unknown','still','moving'];
function verify(body) {
  const i = body.indexOf(',"sig":"');
  if (i < 0) return { ok: false, why: 'no signature' };
  const prefix = body.slice(0, i), sig = body.slice(i + 8, i + 8 + 32);
  let msg; try { msg = JSON.parse(prefix + '}'); } catch (e) { return { ok: false, why: 'bad json' }; }
  const key = Buffer.from(KEYS[msg.id] || DEFAULT_KEY, 'hex');
  const mac = crypto.createHmac('sha256', key).update(prefix).digest('hex').slice(0, 32);
  if (mac !== sig) return { ok: false, why: 'bad signature' };
  return { ok: true, msg };
}
function handle(body) {
  const v = verify(body);
  if (!v.ok) return { code: 400, text: v.why };
  const m = v.msg, dup = lastSeq[m.id] !== undefined && m.n <= lastSeq[m.id];
  if (!dup) lastSeq[m.id] = Math.max(lastSeq[m.id] || 0, m.n);
  const flags = FLAGS.filter(([b]) => m.f & b).map(([, n]) => n).join('|') || '-';
  console.log(new Date().toISOString(), m.id, 'seq', m.n, dup ? '(duplicate)' : '', 'flags', flags,
    m.fx ? `pos ${m.la / 1e6},${m.lo / 1e6} (fix age ${m.fa}s, ${m.sa} sats)` : 'no fix',
    `batt ${m.mv}mV ${m.bp}%`, `hr ${m.hr || '-'} q${m.hq}`, ACT[m.ac], m.sw ? 'strap-closed' : 'STRAP-OPEN', `rssi ${m.rs}`);
  return { code: 200, text: JSON.stringify({ ok: true, dup }) };      // the device only treats HTTP 200 as acknowledged
}
if (process.argv[2] === '--selftest') {
  const sample = process.argv[3];
  const r = handle(sample); console.log('selftest ->', r.code, r.text); process.exit(r.code === 200 ? 0 : 1);
}
http.createServer((req, res) => {
  if (req.method !== 'POST' || req.url !== '/api/device/ingest') { res.writeHead(404); return res.end(); }
  let body = ''; req.on('data', c => { body += c; if (body.length > 1024) req.destroy(); });
  req.on('end', () => { const r = handle(body); res.writeHead(r.code, { 'Content-Type': 'application/json' }); res.end(r.text); });
}).listen(process.env.PORT || 8080, () => console.log('SAFE_LINK mock ingest listening on :' + (process.env.PORT || 8080)));
