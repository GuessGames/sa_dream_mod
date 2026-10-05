# Crash relay

The player launcher sends crash reports here automatically (after the player agreed once). The worker stores them in the
**private** repository `GuessGames/sa_dream_crashes`; the GitHub token never leaves the worker. The crash watcher clones
that repository to `D:\CoopAndreasDev\crash-reports` and analyzes `reports/<day>/<release>/<id>/report.log` (+ `report.dmp`).

## One-time setup (the repository owner does this)

1. **Private repository:** create `GuessGames/sa_dream_crashes` on GitHub, *Private*, with a README (so it has a `main` branch).
2. **Token:** GitHub → Settings → Developer settings → Fine-grained tokens → *Generate new token*:
   repository access *Only select repositories* → `sa_dream_crashes`; permissions → *Contents: Read and write*. Nothing else.
3. **Cloudflare (free):** create an account at https://dash.cloudflare.com, then in a terminal in this folder:
   ```
   npx wrangler login
   npx wrangler secret put GITHUB_TOKEN      (paste the token when asked)
   npx wrangler deploy
   ```
   `deploy` prints the address, e.g. `https://sa-dream-crash-relay.<name>.workers.dev`.
4. Put `<address>/report` into `CrashReports.RelayUrl` in `tools/manager/CoopManager.cs`, rebuild, publish.

## Limits and abuse
* Only POST `/report` with the header `x-app-key` and a body that looks like a crash report is accepted; logs up to 512 KB,
  dumps up to 24 MB (full "Save Dump" dumps are not sent).
* The app key is public (it is in the launcher) — it only keeps scanners away. If somebody spams: rotate `APP_KEY` in
  `wrangler.toml` + the launcher, or revoke the token. The token can only write files into `sa_dream_crashes`.
