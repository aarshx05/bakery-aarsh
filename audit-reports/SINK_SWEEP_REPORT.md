# SUBAGENT 4 — SINK SWEEP: Cross-Cutting Dangerous Sink Inventory

**Repos audited:**
- Web K (tweb) @ commit `4cef169d` — `/home/user/bakery-aarsh/audit/tweb/`
- Web A (telegram-tt) @ commit `4566a14` — `/home/user/bakery-aarsh/audit/telegram-tt/`

---

## 1. PER-SINK INVENTORY

### 1.1 innerHTML

| Location | Client | Attacker-Reachable? | Verdict |
|---|---|---|---|
| `wrapRichText.ts:398` — syntax-highlighted code blocks | Web K | NO — input is output of `highlightCode()` (Prism.js), not raw user text. Code entity text is passed through highlight engine which produces safe HTML tokens. | **SAFE** |
| `htmlToSpan.ts:3` — `span.innerHTML = html` branch | Web K | NO — all callers pass DocumentFragment (from `wrapRichText()`), taking the `span.append(html)` branch instead. The string branch is dead code for message rendering. | **SAFE** |
| `htmlToDocumentFragment.ts:5` — `template.innerHTML = html` | Web K | NO — when input is already DocumentFragment (from `wrapRichText()`), returns early at line 2. String path used only for hardcoded HTML. | **SAFE** |
| `setInnerHTML.ts:7-9` — MISLEADING NAME | Web K | NO — strings go to `elem.textContent` (safe). DOM nodes use `elem.replaceChildren()`. Never uses actual innerHTML. | **SAFE** |
| `document.ts:264` — file attachment template | Web K | NO — hardcoded HTML template with class names only. File name set via `textContent` at line 276. | **SAFE** |
| `geo.ts:66` — map placeholder SVG | Web K | NO — entirely hardcoded SVG path data, no user input interpolated. | **SAFE** |
| `tableClipboard.ts:346` — `title.innerHTML = table.getAttribute(...)` | Web K | SELF-ONLY — clipboard paste from user's own clipboard. Not cross-user exploitable. | **SAFE (self-XSS only)** |
| `widget-sandbox/html-tree.ts:79` — `ctx.innerHTML = req.html` | Web K | ISOLATED — runs in inert document context within QuickJS WASM VM inside WebWorker. Cannot affect main page DOM. | **SAFE** |
| `Latex.tsx:31` — `element.innerHTML = markup` | Web A | NO — `markup` comes from temml LaTeX renderer with `trust: false`, `throwOnError: true`, size/node limits. Produces only MathML. | **SAFE** |
| `teact-dom.ts:866-867` — `dangerouslySetInnerHTML` | Web A | NO — zero component-level usages found in .tsx files. Only exists in framework implementation. | **SAFE** |

### 1.2 insertAdjacentHTML

| Location | Client | Attacker-Reachable? | Verdict |
|---|---|---|---|
| No attacker-reachable instances found in either repo. Used only in internal DOM manipulation with hardcoded content. | Both | NO | **SAFE** |

### 1.3 eval / new Function / setTimeout(string)

| Location | Client | Attacker-Reachable? | Verdict |
|---|---|---|---|
| `widget-sandbox/` — QuickJS eval | Web K | SANDBOXED — runs in isolated WASM VM in WebWorker. Widget code cannot access main page. | **SAFE** |
| Test files and build tooling only | Both | NO — not in production paths | **SAFE** |

### 1.4 location.href / window.open / location.assign

| Location | Client | Attacker-Reachable? | Verdict |
|---|---|---|---|
| `safeWindowOpen.ts` | Web K | FILTERED — applies `normalizeUrlProtocol()` (blocks `javascript:`) + `isSuspiciousUrl()` check. Opens with `noopener,noreferrer`. | **SAFE** |
| `SafeLink.tsx:84` | Web A | FILTERED — `href={ensureProtocol(url)}` applies protocol whitelist (http/https/tg/ton/mailto/tel only). `target="_blank" rel="noopener noreferrer"`. Click routes through `openUrl` action. | **SAFE** |
| `deeplink.ts` / `processDeepLink()` | Web A | FILTERED — all tg:// routes go through typed action handlers. No direct URL navigation. | **SAFE** |

### 1.5 Protocol Sanitization (URL → href)

| Mechanism | Client | Approach | Verdict |
|---|---|---|---|
| `matchUrlProtocol.ts` | Web K | **BLOCKLIST** — `SKIP_PROTOCOLS = new Set(['javascript:'])`. Strips `\t\n\r` before matching to prevent `java\nscript:` bypass. Blocked/missing protocols get `https://` prepended. | **SAFE but weaker design** |
| `url.ts` | Web A | **WHITELIST** — `PROTOCOL_WHITELIST = new Set(['http:', 'https:', 'tg:', 'ton:', 'mailto:', 'tel:'])`. Uses `new URL()` parser. Non-whitelisted protocols get `https://` prepended. | **SAFE — stronger design** |

**Differential note:** Web K's blocklist approach is inherently weaker than Web A's whitelist. If a new dangerous protocol emerges (e.g., `vbscript:`, custom scheme handlers), Web K would need to update its blocklist while Web A is safe by default. However, Web K currently handles known bypass vectors (tab/newline stripping).

### 1.6 postMessage / Mini App Bridge

| Location | Client | Source Verification | Verdict |
|---|---|---|---|
| `telegramWebView.ts` | Web K | WeakMap lookup: `weakMap.get(e.source as Window)?.onMessage(e.origin, e.data)`. Origin check for same-origin apps. JSON parse validation. | **SAFE** |
| `useWebAppFrame.ts:181-188` | Web A | `isMessageFromIframe()` checks `event.source === iframe.contentWindow` AND optionally `event.origin === expectedOrigin`. | **SAFE** |
| `iframe.ts:30-36` | Web A | Sandbox attributes: `allow-scripts allow-popups allow-forms allow-modals allow-same-origin allow-storage-access-by-user-activation`. | **SAFE** |
| `sendEvent()` to Mini App | Web A | Posts to `webAppOrigin || '*'`. When origin unknown, uses wildcard `*`. | **NEEDS-INVESTIGATION** (see Finding 1) |

### 1.7 setAttribute('onclick', ...)

| Location | Client | Attacker-Reachable? | Verdict |
|---|---|---|---|
| `wrapRichText.ts:747` — `element.setAttribute('onclick', onclick + '(this)')` | Web K | VALIDATED — `onclick` value comes from `wrapUrl()` → `getAnchorHandlerName()` → must pass `/^[A-Za-z_$][\w$]*$/` regex AND `hasAnchorHandler()` checks it's an actual registered function on window. | **SAFE** |
| `wrapRichText.ts:200` — `element.setAttribute('onclick', 'showMaskedAlert(this)')` | Web K | HARDCODED — string literal, not from user input. | **SAFE** |

### 1.8 CSP Configuration

| Client | Config | Verdict |
|---|---|---|
| Web K (`index.html:9`) | `img-src 'self' data: blob:; media-src 'self' data: blob:; font-src 'self' data:; style-src 'self' 'unsafe-inline'; form-action 'none'; object-src 'none'; base-uri 'self'` — **NO script-src directive** | **NEEDS-INVESTIGATION** (see Finding 2) |
| Web A | Not found in index.html — likely server-delivered | N/A |

### 1.9 Prototype Pollution

| Location | Client | Attacker-Reachable? | Verdict |
|---|---|---|---|
| `Object.assign()` in mtproto | Web K | NO — used only for error construction and internal message assembly with known shapes. No `__proto__` injection path from user/server data. | **SAFE** |
| State management (Teact signals, Solid stores) | Both | NO — both use typed state management. No deep recursive merge of arbitrary server data. | **SAFE** |

### 1.10 foreignObject / SVG Injection

| Location | Client | Attacker-Reachable? | Verdict |
|---|---|---|---|
| `mediaViewer/base.ts:1717-1719` — foreignObject sizing | Web K | NO — sets width/height attributes on existing elements. No user content injected. | **SAFE** |
| `wrappers.ts:7-56` — foreignObject for thumbnails | Web K | NO — creates DOM elements programmatically. Appends Image elements. No innerHTML. | **SAFE** |
| `AttachBotIcon.tsx:39-45` — DOMParser + outerHTML → data: URI | Web A | NO — SVG loaded from Telegram CDN (bot icon), parsed by DOMParser, output used as `<img src>` data URI. Browsers don't execute scripts in `<img>` SVGs. | **SAFE** |
| `pathBytesToSvg` → data: URI | Web A | NO — constructs SVG from binary path bytes (server thumbnail data). Used as `<img>` src. | **SAFE** |

### 1.11 data: URI Handling

| Client | Handling | Verdict |
|---|---|---|
| Web K | `data:text/html` appears only in widget sandbox fixture (test) and test file. CSP allows `data:` for `img-src`, `media-src`, `font-src`. Protocol filter blocks `data:` as a link protocol (not in skip list, so gets `https://` prepended). | **SAFE** |
| Web A | `data:` URIs used only for SVG thumbnails via `<img>` src (pathBytesToSvg). `data:` not in protocol whitelist, so would be blocked as a link href by `ensureProtocol()`. | **SAFE** |

### 1.12 outerHTML

| Location | Client | Attacker-Reachable? | Verdict |
|---|---|---|---|
| `AttachBotIcon.tsx:45` | Web A | NO — DOMParser output → data URI → `<img src>`. Scripts don't execute in `<img>` context. | **SAFE** |
| `customEmoji.ts:62` — `img.outerHTML` | Web A | NO — reads outerHTML of an `<img>` element the code created. Output goes into editor HTML. | **SAFE** |
| `richEditorMarkdown.ts:244,251` | Web A | NO — reads outerHTML of elements the code constructed for markdown conversion. | **SAFE** |
| `widget-sandbox/capture.ts:684` — tracks TREE_SETTERS | Both | SANDBOXED — widget sandbox capture/intercept layer. | **SAFE** |

### 1.13 Entity Rendering Core

| Client | Mechanism | Verdict |
|---|---|---|
| Web K — `wrapRichText()` | Returns **DocumentFragment** via DOM API (`document.createDocumentFragment()`). Creates elements with `document.createElement()`, sets text with `textContent`, sets attributes with `setAttribute()`. Never builds HTML strings from entity content. | **SAFE** |
| Web A — `renderTextWithEntities.tsx` | Two paths: JSX (virtual DOM, auto-escaped by Teact) and HTML string (content goes through `escape_html` + `escapeHtmlAttribute()` for attributes). URLs via `SafeLink` component or `getHtmlLinkUrl()` → `escapeHtmlAttribute(ensureProtocol(url))`. | **SAFE** |

### 1.14 Link Preview / WebPage Rendering

| Client | Mechanism | Verdict |
|---|---|---|
| Web K — `webPageDescription.ts` | Passes `webPage.description` through `wrapRichText()` (returns DocumentFragment). Title handled via `wrapEmojiText()` (also DOM-based). Site name via `textContent`. | **SAFE** |
| Web A — `WebPage.tsx:273,276` | Title rendered via `renderText(title)` (JSX path, auto-escaped). Description via `renderText(truncatedDescription, ['emoji', 'br'])` (also JSX). | **SAFE** |

### 1.15 Profile / Peer Name Rendering

| Client | Mechanism | Verdict |
|---|---|---|
| Web K — `getPeerTitle.ts:10` | Returns `wrapEmojiText(title)` — DOM-based emoji wrapping, text content is auto-escaped. `plainText` option returns raw string (used in non-HTML contexts). | **SAFE** |
| Web A | Names rendered through Teact JSX (auto-escaped virtual DOM). No innerHTML for user names. | **SAFE** |

### 1.16 Suspicious URL / Homograph Detection

| Client | Mechanism | Verdict |
|---|---|---|
| Web K — `isSuspiciousUrl.ts` | Mixed-script detection, default ignorable code points, credential-in-URL detection. Well-tested (test file covers Cyrillic/Latin homoglyphs, Armenian confusables, CJK combinations). | **SAFE** |
| Web A — `url.ts:106-129` `isSuspiciousUrl()` | Same approach: punycode conversion, mixed-script analysis per domain label, credential detection, default ignorable pattern matching. Full Unicode Script_Extensions coverage. | **SAFE** |

### 1.17 Service Worker / Media Handling

| Client | Mechanism | Verdict |
|---|---|---|
| Web K | Service worker handles media caching. Media loaded via MTProto into `blob:` URIs. CSP restricts media-src to `'self' data: blob:`. | **SAFE** |
| Web A | Similar pattern — media via worker, blob URIs for playback. | **SAFE** |

---

## 2. DIFFERENTIAL ANALYSIS (K vs A)

### 2.1 Protocol Filtering: Blocklist (K) vs Whitelist (A)

**Web K** blocks only `javascript:` and relies on `\t\n\r` stripping to prevent obfuscation. **Web A** whitelists only `http:`, `https:`, `tg:`, `ton:`, `mailto:`, `tel:` via `new URL()` parsing.

**Risk:** Web K's blocklist is inherently less secure. A new dangerous protocol not in the blocklist would pass through. However, since Web K's `normalizeUrlProtocol()` prepends `https://` to any blocked/missing protocol, the practical risk is limited — unknown protocols become `https://unknown-protocol:...` which is harmless.

**Verdict:** Architectural weakness in Web K, but not currently exploitable.

### 2.2 Entity Rendering: DocumentFragment (K) vs Virtual DOM + HTML String (A)

**Web K** builds all entity rendering via DOM API (DocumentFragment), never constructing HTML strings from user content. **Web A** has two paths — JSX (safe by Teact auto-escaping) and HTML string (safe by explicit `escape_html` filtering).

**Risk:** Web K's approach is more defense-in-depth (no HTML string construction at all). Web A's HTML string path relies on correct escaping at every call site, but current code consistently applies `escape_html` and `escapeHtmlAttribute()`.

**Verdict:** Both safe; Web K's architecture is slightly more robust.

### 2.3 PostMessage Origin Handling

**Web K** uses WeakMap-based source verification with explicit origin checking for same-origin apps. **Web A** uses `event.source === iframe.contentWindow` check plus optional origin verification.

**Web A's `sendEvent()`** posts to `webAppOrigin || '*'` — when origin is unknown, it uses wildcard targeting, meaning any window could receive messages intended for the Mini App. However, the messages are event data for the Mini App protocol, and the sensitive direction (Mini App → client) has source verification.

**Verdict:** Web A's wildcard `postMessage` to Mini Apps is a minor information leak vector but not exploitable for XSS.

### 2.4 CSP: Missing script-src (K) vs Unknown (A)

**Web K** intentionally omits `script-src` from its CSP to allow Mini Apps/games in iframes. This means if an XSS were found, CSP would not mitigate it.

**Verdict:** Defense-in-depth gap in Web K. Not a vulnerability on its own but increases impact of any future XSS.

### 2.5 Inline onclick Handlers (K only)

**Web K** uses `setAttribute('onclick', handlerName + '(this)')` for deep link handling. **Web A** uses React-style event handling through Teact.

**Risk:** Web K's approach requires strict validation of handler names, which it implements via regex + window function existence check. Web A avoids this pattern entirely.

**Verdict:** Both safe; Web A's approach is architecturally cleaner.

---

## 3. FINDINGS

### Finding 1: Web A — postMessage Wildcard Target Origin

**Severity:** LOW (Information leak, not XSS)
**File:** `src/components/modals/browser/hooks/useWebAppFrame.ts:96-100`
**Detail:** `sendEvent()` posts to `webAppOrigin || '*'`. When the Mini App origin is unknown (before the iframe loads or if origin extraction fails), messages intended for the Mini App are broadcast to all windows. These messages contain Mini App protocol events (theme data, viewport info, user data tokens).

**Exploitability:** An attacker would need to have another window/iframe in the same browsing context AND the Mini App origin must be unknown. The practical attack scenario is narrow.

**Verdict:** NEEDS-INVESTIGATION — worth verifying what data is sent before origin is established, but unlikely to be bounty-worthy.

### Finding 2: Web K — No script-src CSP Directive

**Severity:** INFORMATIONAL (defense-in-depth gap)
**File:** `index.html:9`
**Detail:** The CSP meta tag omits `script-src`, which means inline scripts and `eval()` are unrestricted by CSP. This is intentional (Mini Apps need script execution in their iframes), but it means CSP provides zero XSS mitigation for the main document.

**Exploitability:** Not exploitable on its own. Increases the impact of any hypothetical XSS by removing the CSP safety net.

**Verdict:** SAFE (by design) — but worth noting as a hardening gap.

### Finding 3: Web K — Protocol Blocklist vs Whitelist

**Severity:** INFORMATIONAL (architectural weakness)
**File:** `src/lib/richTextProcessor/matchUrlProtocol.ts:1`
**Detail:** `SKIP_PROTOCOLS = new Set(['javascript:'])` — only blocks `javascript:`. While tab/newline stripping prevents known obfuscation bypasses, any future dangerous protocol would need to be manually added.

**Exploitability:** Not currently exploitable. All unknown protocols get `https://` prepended, which neutralizes them.

**Verdict:** SAFE — but weaker design than Web A's whitelist approach.

---

## 4. OVERALL ASSESSMENT

Both Telegram web clients demonstrate **strong security posture** in their HTML rendering and URL handling:

1. **No exploitable XSS sinks found** in either client's message rendering pipeline.
2. **Entity rendering is well-protected** — Web K uses DocumentFragment (never constructs HTML from user input), Web A uses virtual DOM auto-escaping + explicit HTML escaping.
3. **URL handling is robust** — both clients filter dangerous protocols, detect suspicious/homograph URLs, and use safe link opening patterns.
4. **PostMessage bridges are source-verified** in both clients for the critical direction (Mini App → client).
5. **SVG/foreignObject usage is safe** — programmatic DOM construction only, no user HTML injection.
6. **innerHTML usage is minimal and controlled** — no attacker-reachable innerHTML paths in either client's message/entity rendering.

**Bounty-eligible findings: NONE.** No cross-user XSS, session hijack, or token theft vectors identified in the dangerous sink analysis. The findings are informational/architectural only.
