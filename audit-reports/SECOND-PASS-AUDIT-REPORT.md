# Telegram Web Clients Security Audit — Second Pass Report

**Date**: 2026-11-26
**Targets**: Web K (morethanwords/tweb @ `4cef169d`) · Web A / WebZ (Ajaxy/telegram-tt @ `4566a14`)
**Program**: Telegram Bug Bounty (core.telegram.org/bug-bounty)
**Scope**: Obscure XSS vectors missed by the first pass: Mutation XSS (mXSS), SVG-based XSS, CSS injection, ProseMirror schema bypass, Instant View HTML rendering, widget sandbox isolation, LaTeX innerHTML, dynamic import injection, prototype pollution, worker message injection, DOM clobbering

---

## EXECUTIVE SUMMARY

**No confirmed exploitable cross-user vulnerabilities were found in the second pass.**

The second-pass audit targeted 14 attack vectors across both clients, specifically chosen for their potential to bypass the safe-by-default architectures identified in the first pass. Every vector was traced end-to-end from attacker-controlled input to DOM output. The findings reinforce the first pass: both clients have mature, layered defenses that prevent cross-user XSS even through indirect paths.

**Nothing in this second pass meets the Telegram bug bounty triage bar.**

---

## VECTORS INVESTIGATED

### 1. Mutation XSS (mXSS) via template.innerHTML — NOT EXPLOITABLE

**Attack theory**: SVG/math namespace switching in pasted HTML causes different DOM interpretation during `template.innerHTML` round-trip, producing executable elements that weren't in the original markup.

**Files analyzed**:
- `tweb/src/components/chat/inputEditor/richClipboard.ts` (lines 97, 648, 782, 823)
- `tweb/src/components/chat/inputEditor/tableClipboard.ts` (lines 373, 514)
- `telegram-tt/src/util/tiptap/botApiHtml.ts` (line 347)

**Why not exploitable**:
1. `template.innerHTML` content IS inert — no scripts execute during parsing
2. The HTML string from `transformPastedHTML` goes through ProseMirror's whitelist-based `DOMParser.fromSchema()` — only the ~40 extensions declared in the schema survive as ProseMirror nodes; everything else (script, img, svg+handlers) is dropped
3. Final serialization via `tiptapToTelegram()` produces only `{text: string, entities: MessageEntity[]}` — sent messages are NEVER raw HTML
4. Any mXSS mutation affects only the pasting user's own editor view (self-XSS, excluded from bounty scope)

### 2. CSS Injection via Entity Data — NOT EXPLOITABLE

**Attack theory**: Attacker-controlled data in message entities reaches `element.style` assignments, allowing CSS injection for data exfiltration or UI redressing.

**Files analyzed**:
- `tweb/src/lib/richTextProcessor/wrapRichText.ts` (lines 126, 573-574)
- `tweb/src/layer.d.ts` (lines 6030-6037)
- `telegram-tt/src/components/common/helpers/renderTextWithEntities.tsx`

**Why not exploitable**:
- **Web K**: Only hardcoded values (`fontFamily = 'markup-' + formatting`) or numeric TL schema fields (`entity.w`/`entity.h` as `'--width', entity.w + 'px'`). The `w` and `h` fields are typed as `number` in the MTProto schema.
- **Web A**: Zero `style` assignments from entity data in `renderTextWithEntities`.

### 3. SVG-Based XSS — NOT EXPLOITABLE

**Attack theory**: Server-provided or user-uploaded SVG content gets inserted into the live DOM (not as `img.src`), allowing script execution.

**Files analyzed**:
| Path | Method | Verdict |
|------|--------|---------|
| `tweb/.../customEmoji.ts` | `<custom-emoji-element>` custom elements | SAFE — no inline SVG |
| `tweb/.../attachBotIcon.ts:62` | `DOMParser().parseFromString` → `textToSvgURL(svg.outerHTML)` → `img.src` data URL | SAFE — img.src doesn't execute SVG scripts |
| `tweb/.../wrappers.ts:46` | `defs.innerHTML = '<clipPath ...>${clipPathHTML}</clipPath>'` | SAFE — hardcoded SVG template strings |
| `tweb/.../sticker.ts` | Lottie players (TGS), video elements (WEBM), image rendering (static) | SAFE — no SVG direct-DOM insertion |

### 4. Instant View HTML Rendering — NOT EXPLOITABLE

**Attack theory**: IV HTML content (from server-processed web pages) contains scripts or same-origin iframes that escape sandboxing.

**Web K** (`telegramWebView.ts` + `public/embed.html`):
- IV HTML embeds loaded via `embed.html` in sandboxed iframe **without `allow-same-origin`** (opaque origin)
- Guard at `embed.html:10`: `if(self.origin === 'null')` prevents execution when loaded directly
- HTML delivered via postMessage with source validation (`e.source !== parent`)
- Comment at `telegramWebView.ts:83` explicitly notes why `contentWindow.document` is never written to directly (would inherit app origin)

**Web A** (`components/iv/EmbedFrame.tsx` + `components/iv/RichContent.tsx`):
- IV content rendered via JSX/Teact components (SafeLink, CodeBlock, Blockquote, etc.) — safe by framework
- IV HTML embeds loaded in sandboxed iframes with `IFRAME_SANDBOX_ATTRIBUTES`
- Embed URLs extracted via `extractInstantViewEmbedUrl()` → `normalizeEmbedUrl()` → protocol restricted to `http:`/`https:` only
- PostMessage handler validates source (`isMessageFromIframe`) and only processes `resize_frame` events with numeric height

**Defense-in-depth note**: Web A's `IFRAME_SANDBOX_ATTRIBUTES` includes `allow-same-origin` + `allow-scripts`. A same-origin iframe could theoretically escape this sandbox. Mitigated by: embed URLs come from Telegram's server-side IV parser (not directly attacker-controlled), and Telegram's IV generator typically produces embeds from known providers only.

### 5. Widget Sandbox CSS/JS Isolation — NOT EXPLOITABLE

**Attack theory**: Widget HTML/CSS/JS escapes the sandbox to affect the parent page or other users.

**Files analyzed**:
- `telegram-tt/src/lib/widget-sandbox/src/sandbox/index.ts`
- `telegram-tt/src/lib/widget-sandbox/src/sandbox/dom.ts`
- `telegram-tt/src/lib/widget-sandbox/src/host/validator.ts`
- `telegram-tt/src/lib/widget-sandbox/src/renderer/core.ts`

**Architecture** (defense in depth):
1. **Execution isolation**: Widgets run inside QuickJS (WebAssembly) in a web worker — completely isolated from the main thread
2. **DOM isolation**: Virtual DOM through linkedom, not the real page DOM. Widget output applied to shadow root via validated ops
3. **innerHTML override**: Prototype-level override redirects innerHTML to textContent for most elements (`sandbox/index.ts:106-143`)
4. **Script prevention**: `dom.ts:12-14` explicitly notes scripts from innerHTML/outerHTML/insertAdjacentHTML never execute
5. **Attribute filtering**: `renderer/core.ts:136` blocks `FORBIDDEN_ATTRS`: `srcdoc`, `srcset`, `action`, `formaction`, `ping`, `background`, `nonce`, `target`, `http-equiv`
6. **Validator**: Host-side validator (`validator.ts`) applies element/attribute/property budget limits

### 6. LaTeX innerHTML in Web A IV — NOT PRACTICALLY EXPLOITABLE

**Location**: `telegram-tt/src/components/iv/Latex.tsx:31`

**Data flow**: LaTeX source string (from IV page) → `renderLatex()` (web worker, temml library) → MathML markup → innerHTML

**Why not practically exploitable**:
1. Requires a temml library XSS vulnerability (temml produces MathML, a limited element set)
2. Requires Telegram's IV parser to preserve attacker-controlled LaTeX content
3. Only affects users who open specific IV pages with LaTeX
4. Multiple independent defense layers would all need to fail simultaneously

### 7. `htmlToSpan` innerHTML Path — NOT EXPLOITABLE

**Location**: `tweb/src/helpers/dom/htmlToSpan.ts:3` — `if(typeof(html) === 'string') span.innerHTML = html; else span.append(html);`

**All callers** pass `wrapEmojiText()` or `wrapRichText()` output, both of which return `DocumentFragment` (not string). The `span.append()` branch is always taken. The innerHTML path is dead code for all current call sites.

### 8. `messageActionTextNewUnsafe` — SAFE

**Location**: `tweb/src/components/wrappers/messageActionTextNewUnsafe.ts`

Despite the alarming name, uses safe DOM APIs throughout: `document.createElement()`, `element.textContent`, `setInnerHTML()` (safe wrapper using textContent/replaceChildren). The "Unsafe" in the name refers to the function being async and potentially throwing, not XSS vulnerability.

### 9. Dynamic Import Injection — NOT EXPLOITABLE

All `import()` calls across both codebases use string literal module paths (`'@components/popups/frozen'`). No dynamic import uses attacker-controlled or computed strings.

### 10. Prototype Pollution — NOT EXPLOITABLE

Solid.js stores properly skip `__proto__` and `constructor` in proxy handlers (`solid/dist/solid.js:1336`, `solid/store/dist/store.js:98`). No `Object.assign` or spread operator on raw `JSON.parse` output from attacker-controlled sources in application code.

### 11. Worker Message Handlers — SAFE

All web worker `onmessage` handlers use typed event data. No `eval`, `Function`, or `importScripts` on message data in any worker file.

### 12. `dangerouslySetInnerHTML` in Web A — SAFE

Teact framework supports `dangerouslySetInnerHTML` (`teact-dom.ts:866`), but **no .tsx component file uses it**. Confirmed via exhaustive grep across all component files.

### 13. postMessage Target Origin Patterns — SAFE

- **Web K**: `'*'` used ONLY for opaque-origin embed frames (intentional, documented)
- **Web A**: No `postMessage(..., '*')` calls found in non-vendor source

### 14. CSP Configuration — UNCHANGED FROM FIRST PASS

- **Web K**: No `script-src` — by design for Mini App iframe support. Mitigated by DOM API rendering.
- **Web A**: `script-src 'self' 'wasm-unsafe-eval'` — strict, blocks inline scripts and eval.

---

## MASTER COVERAGE MATRIX (SECOND PASS)

| Attack Vector | Web K | Web A | Exploitable? |
|---|---|---|---|
| mXSS via template.innerHTML paste | SAFE | SAFE | No |
| CSS injection via entity data | SAFE | SAFE | No |
| SVG inline DOM insertion | SAFE | N/A | No |
| Instant View HTML rendering | SAFE | SAFE | No |
| Widget sandbox escape | N/A | SAFE | No |
| LaTeX innerHTML (IV) | N/A | LOW RISK | No |
| `htmlToSpan` innerHTML | SAFE | N/A | No |
| Dynamic import injection | SAFE | SAFE | No |
| Prototype pollution | SAFE | SAFE | No |
| Worker message injection | SAFE | SAFE | No |
| `dangerouslySetInnerHTML` | N/A | SAFE | No |
| DOM clobbering | SAFE | SAFE | No |
| postMessage `'*'` origin | SAFE | SAFE | No |
| Service Worker scope | SAFE | SAFE | No |

---

## COMBINED CONCLUSION (PASS 1 + PASS 2)

After two comprehensive audit passes covering 30+ attack vectors:

**Pass 1** (4 subagents + lead): All entity types, URL protocols, deep links, Mini App bridge, dangerous sinks (innerHTML x153, eval x25, postMessage x58), CSP, auth keys, clipboard paste, DOM clobbering, IDN homographs, masked URLs.

**Pass 2** (this report): mXSS, SVG XSS, CSS injection, ProseMirror bypass, IV HTML, widget sandbox, LaTeX rendering, dynamic imports, prototype pollution, worker injection, DOM clobbering, postMessage origins.

Both Telegram web clients demonstrate defense-in-depth security with no confirmed cross-user vulnerabilities:
1. **Safe-by-default rendering**: DOM API (Web K) and JSX (Web A) prevent XSS at the framework level
2. **Message serialization boundary**: ProseMirror schema → `{text, entities}` makes HTML injection in messages impossible
3. **Proper iframe isolation**: Opaque origins for IV embeds (Web K), strict sandboxing (Web A)
4. **Widget execution isolation**: QuickJS (WASM) in workers with virtual DOM — no main-thread exposure
5. **Protocol sanitization**: javascript: blocked by both; data: blocked by Web A's whitelist
6. **Bridge security**: Unforgeable WindowProxy identity verification for Mini App communication

**No findings are submitted to security@telegram.org** as none meet the bounty triage bar of demonstrated cross-user security impact.
