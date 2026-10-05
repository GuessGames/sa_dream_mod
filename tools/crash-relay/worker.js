// SA Dream Mod crash relay (Cloudflare Worker).
// The launcher POSTs a crash report here; the worker stores it in the PRIVATE repository REPO with a GitHub token that
// lives only in the worker's secrets (never in the public launcher). The crash watcher on the developer PC pulls that repo.
//
// Secrets / vars (see README.md): GITHUB_TOKEN (secret, fine-grained, Contents read/write on REPO only), REPO, APP_KEY.

const MAX_LOG = 512 * 1024;          // characters
const MAX_DUMP = 24 * 1024 * 1024;   // bytes (base64 decoded); bigger dumps are dropped, the log is still stored

function json(status, body) {
  return new Response(JSON.stringify(body), { status, headers: { "content-type": "application/json" } });
}

function clean(s, max) {
  return String(s || "").replace(/[^\w.\-]/g, "_").slice(0, max);
}

async function put(env, path, base64, message) {
  const r = await fetch(`https://api.github.com/repos/${env.REPO}/contents/${path}`, {
    method: "PUT",
    headers: {
      authorization: `Bearer ${env.GITHUB_TOKEN}`,
      accept: "application/vnd.github+json",
      "user-agent": "sa-dream-crash-relay",
      "content-type": "application/json",
    },
    body: JSON.stringify({ message, content: base64 }),
  });
  if (!r.ok) throw new Error(`GitHub ${r.status}: ${(await r.text()).slice(0, 200)}`);
}

function toBase64Utf8(text) {
  const bytes = new TextEncoder().encode(text);
  let bin = "";
  for (let i = 0; i < bytes.length; i += 0x8000) bin += String.fromCharCode(...bytes.subarray(i, i + 0x8000));
  return btoa(bin);
}

export default {
  async fetch(request, env) {
    if (request.method !== "POST" || new URL(request.url).pathname !== "/report") return json(404, { ok: false });
    if (request.headers.get("x-app-key") !== env.APP_KEY) return json(403, { ok: false });

    let r;
    try { r = await request.json(); } catch { return json(400, { ok: false, error: "bad json" }); }
    const log = String(r.log || "");
    // only things that look like our crash reports
    if (!log || log.length > MAX_LOG || !/Unhandled exception|server crash/i.test(log)) return json(400, { ok: false, error: "not a report" });

    const now = new Date();
    const day = now.toISOString().slice(0, 10);
    const id = `${now.toISOString().slice(11, 19).replace(/:/g, "")}_${clean(r.kind, 8)}_${crypto.randomUUID().slice(0, 8)}`;
    const dir = `reports/${day}/${clean(r.release, 40) || "unknown"}/${id}`;
    const who = clean(r.nick, 24);

    try {
      await put(env, `${dir}/report.log`, toBase64Utf8(log), `crash ${clean(r.file, 60)} ${who}`);
      const dump = String(r.dump || "");
      if (dump && dump.length * 0.75 <= MAX_DUMP && /^[A-Za-z0-9+/=]+$/.test(dump))
        await put(env, `${dir}/report.dmp`, dump, `dump ${id}`);
    } catch (e) {
      return json(502, { ok: false, error: String(e.message || e) });
    }
    return json(200, { ok: true, id: `${day}/${id}` });
  },
};
