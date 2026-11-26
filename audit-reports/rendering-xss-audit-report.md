# SUBAGENT 1 — Rendering XSS Audit Report

**Scope**: Message text + entity rendering paths in Web K (tweb) and Web A (telegram-tt)
**Verdict**: No confirmed cross-user XSS vulnerabilities found.

---

## Per-Sink Coverage Verdict Table

### Web K (tweb) — `wrapRichText.ts` + supporting modules

| Entity Type | Sink | Escaping/Safety | Verdict |
|---|---|---|---|
| `messageEntityTextUrl` | `element.href = url` via `wrapUrl()` → `normalizeUrlProtocol()` | Blocks `javascript:` protocol; DOM property assignment (not attribute string) | **SAFE** |
| `messageEntityUrl` | Same pipeline as TextUrl | Same | **SAFE** |
| `messageEntityCode` | `element.textContent = fullEntityText` | textContent is inherently safe | **SAFE** |
| `messageEntityPre` | `element.textContent` initially; `element.innerHTML = html` for Prism.js output | Prism.js HTML-encodes input; language param from fixed alias map | **SAFE** |
| `messageEntityCustomEmoji` | `dataset.stickerEmoji = fullEntityText` | Data attribute (not parsed as HTML) | **SAFE** |
| `messageEntitySpoiler` | `element.textContent = partText` via DotRenderer | textContent is inherently safe | **SAFE** |
| `messageEntityBlockquote` | `document.createElement('blockquote')` + child elements via DOM APIs | DOM API construction | **SAFE** |
| `messageEntityBold/Italic/Strike/Underline` | `document.createElement('b'/'i'/'s'/'u')` | DOM API construction | **SAFE** |
| `messageEntityMention` | `wrapRichText` recursive → anchor element | URL goes through wrapUrl pipeline | **SAFE** |
| `messageEntityBotCommand` | `document.createElement('a')` + text via DOM | DOM API construction | **SAFE** |
| Inline keyboard button URL | `createUrlButtonAnchor()` → reuses `wrapRichText` with `messageEntityTextUrl` entity | Same sanitization pipeline as text_url | **SAFE** |
| Link preview (title/desc) | `wrapRichText` for text rendering | DOM APIs | **SAFE** |
| Media captions | `wrapRichText` for text rendering | Same pipeline | **SAFE** |
| `setInnerHTML` helper | Uses `elem.textContent` or `elem.replaceChildren()` despite name | Does NOT use actual innerHTML | **SAFE** |
| `insertAdjacentHTML` (20 occurrences) | All use static HTML string literals | No attacker-controlled data | **SAFE** |

### Web A (telegram-tt) — `renderTextWithEntities.tsx` + supporting modules

| Entity Type | Sink | Escaping/Safety | Verdict |
|---|---|---|---|
| `TextUrl` / `Url` | `<SafeLink url={ensureProtocol(url)}>` JSX | Strict protocol allowlist (http/https/tg/ton/mailto/tel) + JSX escaping | **SAFE** |
| `Code` | `<code>` JSX element with text children | JSX automatic escaping | **SAFE** |
| `Pre` | `<CodeBlock text={entityText} language={entity.language}>` | JSX rendering; Prism output via safe path | **SAFE** |
| `CustomEmoji` | `<CustomEmoji documentId={entity.documentId}>` | JSX + numeric documentId | **SAFE** |
| `Spoiler` | `<Spoiler>` JSX component with text children | JSX automatic escaping | **SAFE** |
| `Blockquote` | `<Blockquote>` JSX component | JSX automatic escaping | **SAFE** |
| `Bold/Italic/Strike/Underline` | `<strong>/<em>/<del>/<ins>` JSX elements | JSX automatic escaping | **SAFE** |
| `MentionName` | `<MentionLink>` JSX component | JSX rendering | **SAFE** |
| `BotCommand` | `<a>` JSX with text children | JSX automatic escaping | **SAFE** |
| Inline keyboard button URL | `<Button onClick={...}>` JSX — URL not in href | Action dispatchers, no DOM href | **SAFE** |
| Inline bot results | `<BaseResult title={} description={}>` JSX | JSX automatic escaping | **SAFE** |
| Link preview | `<SafeLink>` + `renderText()` JSX | JSX escaping + SafeLink component | **SAFE** |
| Media captions | `renderTextWithEntities` JSX | Same pipeline | **SAFE** |
| `processEntityAsHtml()` | HTML strings with `escapeHtml()` / `escapeHtmlAttribute()` | Only for clipboard copy, not DOM rendering | **SAFE** |
| LaTeX (Instant View) | `element.innerHTML = markup` from temml worker | Temml produces MathML; source from server IV pages | **SAFE** (see notes) |
| `dangerouslySetInnerHTML` | Defined in Teact framework | Never used in any .tsx component file | **N/A** |

---

## Protocol Handling Comparison

| | Web K | Web A |
|---|---|---|
| Approach | Blocklist (`javascript:` only) | Allowlist (`http:`, `https:`, `tg:`, `ton:`, `mailto:`, `tel:`) |
| `javascript:` | Blocked | Blocked (not in allowlist) |
| `data:` | NOT blocked | Blocked (not in allowlist) |
| `vbscript:` | NOT blocked | Blocked (not in allowlist) |
| `blob:` | NOT blocked | Blocked (not in allowlist) |
| Fallback | Prepends `https://` if no protocol | Prepends `https://` if protocol not in allowlist |

**Web K's blocklist gap analysis**: While `data:`, `vbscript:`, and `blob:` are not blocked:
- `data:text/html,<script>...` navigation from `<a>` clicks is blocked by Chrome (since v60), Firefox (since v59), and Safari
- `data:text/html,<script>...` via `window.open()` is also blocked by modern browsers
- `vbscript:` only works in IE (not supported in modern browsers)
- `blob:` URLs require same-origin blob creation (attacker cannot reference a blob from another origin)
- Telegram servers validate entity URLs before storing, providing server-side defense in depth

**Verdict**: The protocol gap is a defense-in-depth weakness, NOT an exploitable XSS vulnerability.

---

## CSP Analysis

### Web K
```
img-src 'self' data: blob:; media-src 'self' data: blob:; font-src 'self' data:;
style-src 'self' 'unsafe-inline'; form-action 'none'; object-src 'none'; base-uri 'self'
```
- **No `script-src` or `default-src`** — intentional for mini app iframes
- Even without CSP script restrictions, XSS requires bypassing the DOM API safety first

### Web A
- CSP set by service worker, not in meta tag

---

## Architectural Safety Summary

**Web K** uses DOM APIs exclusively for message rendering:
- `document.createElement()` for structure
- `element.textContent` for text content
- `element.append()` for composition
- `element.href = sanitizedUrl` for links (property assignment)
- The ONLY `innerHTML` in the rendering path is for Prism.js code highlighting output (Prism HTML-encodes)

**Web A** uses React/Teact JSX for message rendering:
- All entity types return JSX elements (`<strong>`, `<SafeLink>`, `<CodeBlock>`, etc.)
- JSX automatically escapes string content
- URLs always go through `ensureProtocol()` with strict allowlist
- No `dangerouslySetInnerHTML` in any component

Both approaches are "safe by default" — an attacker would need to find a path that bypasses the fundamental rendering mechanism, not just craft a malicious payload.

---

## Confirmed Findings

**None.**

## Promising Leads (Unconfirmed / Theoretical)

### Lead 1: Web K protocol blocklist gap (LOW confidence, NOT exploitable)
- **What**: Web K only blocks `javascript:` protocol, while `data:`, `vbscript:`, `blob:` pass through
- **Why not exploitable**: Modern browsers block `data:` navigation from anchor links and `window.open()`. Server-side URL validation provides additional defense. No browser in current use supports `vbscript:`.
- **Recommendation**: Defense-in-depth improvement — switch to an allowlist like Web A's

### Lead 2: LaTeX innerHTML in Web A Instant View (VERY LOW confidence)
- **What**: `Latex.tsx` sets `innerHTML` from temml worker output for Instant View pages
- **Why not exploitable**: Source is Telegram's server-side IV parser; temml produces MathML (limited element set); would require both a temml XSS bug AND Telegram's IV parser preserving attacker LaTeX; only affects users who click "Instant View" on specific links
- **Recommendation**: No action needed — multiple layers of defense

### Lead 3: postMessage handler in Web K TelegramWebView (outside core scope)
- **What**: Handles postMessage from mini app iframes, validates origin
- **Why deferred**: Mini app security is a separate attack surface from message rendering; would need separate audit of the bridge protocol
- **Recommendation**: Separate audit of mini app → main app communication

---

## Methodology

1. Full code read of core rendering files in both clients
2. `innerHTML` / `insertAdjacentHTML` / `dangerouslySetInnerHTML` grep across both codebases
3. URL sanitization pipeline trace (entity URL → wrapUrl/ensureProtocol → href attribute)
4. Protocol filtering analysis (blocklist vs allowlist approach)
5. Entity-by-entity handler audit in both clients' renderers
6. CSP analysis
7. Inline keyboard button, inline bot result, link preview, and media caption path verification
8. HTML string building path analysis (`processEntityAsHtml` in Web A)
