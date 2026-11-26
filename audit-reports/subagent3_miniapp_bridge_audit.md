# SUBAGENT 3 — Mini-App Bridge + Client Logic Audit Report

**Scope:** postMessage bridge security, prototype pollution, CSP/SW weaknesses, Mini App iframe escape vectors  
**Clients:** Web K (tweb, commit 4cef169d) · Web A (telegram-tt, commit 4566a14)  
**Triage bar:** Cross-account exploitation with demonstrated security impact (token/data theft, unauthorized action)

---

## Executive Summary

**No confirmed vulnerabilities meeting the triage bar were found.** Both clients implement a well-designed defense-in-depth model for the Mini App postMessage bridge. The source-based gating (WindowProxy identity check) is the primary security boundary, with origin checking as a secondary layer applied when the server-side `same_origin` flag is set. All event handlers that could perform sensitive actions are properly gated with additional validation (server config allowlists, user confirmation, capability flags).

---

## 1. Per-Event/Vector Coverage Table

### 1.1 postMessage Bridge Events (Mini Apps)

| Event | Web K (webApp.tsx) | Web A (useWebAppFrame.ts + WebAppTab.tsx) | Risk Assessment |
|---|---|---|---|
| `web_app_ready` | Marks ready | Marks ready | None — no side effects |
| `web_app_close` | Closes app | Closes app | None — UI only |
| `web_app_data_send` | Gated: `isSimpleWebView` only | Gated: `isSimpleView` only | SAFE — single-use, no replay |
| `web_app_open_link` | Protocol-validated via `normalizeUrlProtocol` + server `appConfig.allowedProtocols` allowlist | Protocol-validated via `isValidProtocol(url, allowedProtocols)` using `new URL()` parser | SAFE — `javascript:` blocked |
| `web_app_open_tg_link` | `getWebViewTgLink()` strips leading slashes, prepends `https://t.me/` | `TME_LINK_PREFIX + eventData.path_full` (prefix is `'https://t.me/'` with trailing slash) | SAFE — see §3.1 |
| `web_app_set_header_color` | CSS only, validated via `hexaToHsla` | CSS only, validated via `getValidHexColor` | None — visual only |
| `web_app_set_background_color` | CSS only | CSS only, validated | None — visual only |
| `web_app_set_bottom_bar_color` | CSS only | CSS only, validated | None — visual only |
| `web_app_setup_main_button` | Text rendered via `wrapEmojiText` (not innerHTML) | Text rendered via safe API | SAFE — no injection |
| `web_app_setup_secondary_button` | Text rendered via `wrapEmojiText` | Text rendered via safe API | SAFE — no injection |
| `web_app_open_invoice` | Calls `openInvoice` → server-validated payment flow | Calls `openInvoice` | SAFE — server-validated |
| `web_app_switch_inline_query` | Opens share dialog | Dispatches `switchBotInline` | None — UI action |
| `web_app_read_text_from_clipboard` | Gated: attached menu bots only + user interaction | Gated: `isAttachMenuBot` + `hasWriteAccessAllowed` | SAFE — permission checks |
| `web_app_request_file_download` | Server-validated then user-confirmed | (Handled similarly) | SAFE — double-gated |
| `web_app_verify_age` | Gated: `verify_age_bot_username` only | (Similar gating) | SAFE — bot-restricted |
| `web_app_open_scan_qr_popup` | Opens native QR scanner | Opens native QR scanner | None — returns QR data only |
| `web_app_biometry_*` | Returns UNSUPPORTED or manages biometric auth flow | Returns UNSUPPORTED | SAFE — web limitation |
| `web_app_set_emoji_status` | Server-mediated | Server-mediated | SAFE — API call, not direct |
| Device storage/secure storage | Returns UNSUPPORTED | Returns UNSUPPORTED | N/A |
| Sensors (gyroscope/accel) | Returns UNSUPPORTED | Returns UNSUPPORTED | N/A |

### 1.2 postMessage Bridge Events (Games)

| Event | Web K (browser.tsx) | Web A (GameModal.tsx) | Risk Assessment |
|---|---|---|---|
| `share_game` | Opens forward menu | Opens forward menu | None — UI only |
| `share_score` | Opens forward menu | Opens forward menu | None — UI only |
| Source check | WeakMap-based lookup | `event.source === iframe.contentWindow` | SAFE |
| Origin check | None (not needed — game iframe is cross-origin) | None | Acceptable — games only emit 2 events |

### 1.3 postMessage Bridge Events (Payment Verification)

| Event | Web K (paymentVerification.tsx) | Web A (ConfirmPayment.tsx) | Risk Assessment |
|---|---|---|---|
| `web_app_open_tg_link` | `getWebViewTgLink()` strips leading `/` | `TME_LINK_PREFIX + eventData.path_full!` | SAFE — see §3.2 |
| `payment_form_submit` | Passes credentials to handler | Calls `onPaymentFormSubmit` callback | SAFE — credentials go to Telegram's API |
| Source check | WeakMap | `event.source === iframe.contentWindow` | SAFE |
| Origin check | None when `this.origin` not set | None (`isMessageFromIframe` without expectedOrigin) | See §3.3 |

### 1.4 postMessage Bridge Events (Instant View Embeds)

| Event | Web K (instantView.tsx + embed.html) | Web A (EmbedFrame.tsx) | Risk Assessment |
|---|---|---|---|
| `resize_frame` | Handled for height adjustment | Handled for height adjustment | None — numeric only |
| `embed_ready` / `embed_html` | Used for opaque-origin HTML injection via `document.write` | N/A (URL embeds only) | SAFE — opaque origin, see §3.4 |
| Sandbox | HTML: NO `allow-same-origin` (opaque). Cross-origin URL: `allow-same-origin`. Same-origin URL: NO `allow-same-origin` | `IFRAME_SANDBOX_ATTRIBUTES` for all (includes `allow-same-origin`) | See §3.5 |

---

## 2. Security Architecture Assessment

### 2.1 Bridge Source Validation

**Web K:** Uses a WeakMap mapping `WindowProxy → TelegramWebView` instance. The `onMessage` handler in `telegramWebView.ts:64-76` first looks up `event.source` in the WeakMap. If not found, the message is silently dropped. This is the strongest possible source check — WeakMap keys are identity-based and cannot be spoofed.

**Web A:** Uses `isMessageFromIframe()` in `iframe.ts:30-36` which checks `event.source === iframe.contentWindow`. This is equivalent to Web K's approach — `event.source` is a browser-provided WindowProxy identity that cannot be forged by an attacker.

**Verdict:** Both implementations are correct. The source check is the primary security boundary.

### 2.2 Origin Validation

**Web K:** Origin is checked only when `this.origin` is set (via `same_origin` server flag). When not set, the string-type check (`typeof(data) !== 'string'`) serves as a secondary filter.

**Web A:** `isMessageFromIframe` checks origin only when `expectedOrigin` is defined. For non-same-origin Mini Apps, `webAppOrigin` is derived from the URL and checked. For same-origin apps, the check is skipped.

**Verdict:** Origin checking is by-design conditional. The source (WindowProxy) check is sufficient because:
1. `event.source` is set by the browser and cannot be spoofed
2. Only the iframe that the client created can match the stored reference
3. A malicious page in another tab/window cannot match another iframe's WindowProxy

### 2.3 Iframe Sandbox Configuration

Both clients use: `allow-scripts allow-same-origin allow-popups allow-forms allow-modals`

The `allow-scripts + allow-same-origin` combination is safe here because:
- Mini App iframes load from third-party domains (the bot's web app URL)
- Their origin differs from `web.telegram.org` (the parent)
- Cross-origin restrictions prevent access to parent's DOM, localStorage, cookies, IndexedDB
- The sandbox still prevents top-navigation without `allow-top-navigation`

### 2.4 CSP Analysis

**Web K (`index.html:9`):**
```
img-src 'self' data: blob:; media-src 'self' data: blob:; font-src 'self' data:;
style-src 'self' 'unsafe-inline'; form-action 'none'; object-src 'none'; base-uri 'self'
```
- **No `default-src` or `script-src` directive** — scripts and frames are unrestricted
- This is **intentional** per documentation: "Frames, scripts and connections are not restricted: mini apps, games and payment forms are third-party frames"
- The CSP targets widget security: preventing widget markup that gets past the validator from reaching the network

**Web A (`vite.config.ts:304-321`):**
```
default-src 'self'; script-src 'self' 'wasm-unsafe-eval' https://t.me/_websync_ ...;
worker-src 'self'; frame-src http: https: ...; base-uri 'none'; form-action 'none';
```
- Proper `default-src 'self'` with explicit `script-src`
- `frame-src http: https:` allows Mini App iframes (required)
- `base-uri 'none'` prevents base tag injection

**Verdict:** Web A has stricter CSP. Web K's permissive CSP is a design choice for widget compatibility, not a vulnerability — it still blocks image/media/font exfiltration. Neither CSP is exploitable for cross-account attacks because there's no injection vector to deliver a `<script>` tag into the parent page.

### 2.5 Service Worker Analysis

**Web K (`index.service.ts`):** Caches JS/CSS/assets. Handles stream/download/share/rtmp/hls/backgrounds paths. No attacker-reachable cache write path.

**Web A (`service.worker.ts`):** Media responses get `Content-Security-Policy: sandbox` + `X-Content-Type-Options: nosniff`. Widget worker gets restrictive CSP. No attacker-reachable cache write path.

**Verdict:** Service workers in both clients are not exploitable. Cache writes are keyed to known asset paths and don't accept arbitrary URLs from Mini Apps.

---

## 3. Investigated Leads (All Below Triage Bar)

### 3.1 `web_app_open_tg_link` Path Sanitization Difference

**Web K** (`getWebViewTgLink.ts`):
```typescript
return 'https://t.me/' + (pathFull || '').replace(/^\/+/, '');
```
Strips leading slashes to prevent `//attacker.com` becoming `https://t.me///attacker.com`.

**Web A** (`WebAppTab.tsx:716`):
```typescript
const linkUrl = TME_LINK_PREFIX + eventData.path_full;
// TME_LINK_PREFIX = 'https://t.me/'
```
No explicit stripping of leading slashes.

**Analysis:** A `path_full` of `//attacker.com` would produce `https://t.me///attacker.com`. When parsed by `new URL()`, the browser treats the first `/` after `t.me` as a path separator, so the host remains `t.me` and the path becomes `///attacker.com`. The URL resolves to a t.me page, not an external redirect.

Tested: `new URL('https://t.me/' + '//attacker.com')` → host: `t.me`, pathname: `///attacker.com`.

A `path_full` of `\@attacker.com` or other Unicode shenanigans would similarly stay under `t.me` because `URL()` parsing treats everything after the authority as path.

**Verdict:** Not exploitable. The trailing slash in the prefix prevents authority injection. Web K's stripping is defense-in-depth but not strictly necessary.

### 3.2 ConfirmPayment `path_full` Without Sanitization (Web A)

Same analysis as §3.1 applies. `TME_LINK_PREFIX + eventData.path_full!` at `ConfirmPayment.tsx:54` produces a URL under `t.me` regardless of `path_full` content.

Additionally, the payment iframe is loaded from a Telegram-controlled URL (the payment provider URL returned by the server), not from an arbitrary third-party. The source check (`isMessageFromIframe`) ensures only the actual payment iframe can send this event.

**Verdict:** Not exploitable.

### 3.3 Missing Origin Check in Payment/Game Iframes

Both clients omit origin checking for payment and game iframes, relying only on source (WindowProxy) checking.

**Analysis:** Since `event.source` identity is unforgeable, an attacker cannot send messages that pass the check without controlling the specific iframe element's content window. The only entity that can do this is the content loaded in that iframe — which is either:
- A Telegram-approved payment provider (for payments)
- The game URL specified by the bot owner (for games)

An attacker would need to compromise the payment provider's domain or the game's hosting to inject code — at which point they already have equivalent access.

**Verdict:** Not a vulnerability. Source checking alone is sufficient.

### 3.4 Instant View HTML Embed Security (Web K)

Web K's `embed.html` runs with opaque origin (`self.origin === 'null'`). It receives HTML from the parent via postMessage and writes it with `document.write`. The opaque origin means:
- No access to parent's DOM/storage/cookies
- No network access beyond what CSP allows
- Cannot navigate the parent frame

**Verdict:** Secure. The opaque origin sandbox is the strongest isolation possible.

### 3.5 Web A EmbedFrame `allow-same-origin` for All Embeds

Web A uses `IFRAME_SANDBOX_ATTRIBUTES` (includes `allow-same-origin`) for ALL embed types in `EmbedFrame.tsx:80`. Web K differentiates: HTML embeds get no `allow-same-origin`, while cross-origin URL embeds do.

**Analysis:** Web A only uses URL embeds in EmbedFrame (not HTML injection like Web K's embed.html). The `allow-same-origin` allows the embedded page to access its own origin's storage, which is correct behavior for legitimate embeds (YouTube, Twitter, etc.). Since these are cross-origin from `web.telegram.org`, they cannot access the parent's resources.

If the embed URL happened to be same-origin with the parent (e.g., `web.telegram.org`), `allow-same-origin` would allow sandbox escape. However:
1. Embed URLs come from Instant View pages, which are server-generated
2. Telegram's server would not generate same-origin embed URLs
3. Even if it did, the embed would need to contain attacker-controlled JavaScript

**Verdict:** Theoretical concern only. Not exploitable without server-side compromise.

### 3.6 Prototype Pollution via `deepMerge` (Web A)

`deepMerge.ts:24` iterates keys from both objects and sets them on a fresh `{}` accumulator:
```typescript
const allKeys = unique(Object.keys(value1).concat(Object.keys(value2)));
return allKeys.reduce((acc: AnyLiteral, key) => {
    acc[key] = deepMerge(oldValue, newValue);
    return acc;
}, {}) as T;
```

Setting `acc['__proto__']` would invoke the prototype setter. However:
1. `Object.keys()` returns only own enumerable properties, not `__proto__`
2. `deepMerge` is called from `sharedState.worker.ts` and `multitab.ts` — internal state sync between tabs/workers
3. Input data comes from the Solid.js store diff system (`deepDiff`), not from external/attacker sources
4. MTProto TL schema uses fixed named fields defined in the schema — an attacker cannot inject arbitrary keys like `__proto__` through the Telegram protocol

**Verdict:** Not reachable from attacker-controlled input. Not exploitable.

### 3.7 Prototype Pollution via `safeAssign` (Web K)

`safeAssign.ts`: `for(const i in fromObject) { object[i] = fromObject[i]; }`

The `for...in` iterates inherited properties, which could include `__proto__` if the source object's prototype chain has it as an enumerable property. However:
1. `safeAssign` is called with internal objects only
2. All data entering from MTProto is deserialized from the TL binary schema with fixed field names
3. No path exists where attacker-controlled arbitrary-key objects reach `safeAssign`

**Verdict:** Not reachable from attacker-controlled input. Not exploitable.

### 3.8 Web K Missing `script-src` CSP — Impact on Widget Security

Without `script-src` or `default-src`, any injected `<script>` tag in the page would execute. However:
1. There is no known XSS injection vector in the page itself (this is a separate audit concern)
2. Widget code runs in a QuickJS VM worker, not in the page
3. Widget DOM output is sanitized by the validator before being applied to shadow roots
4. The CSP specifically blocks widget markup from loading external images/media/fonts

**Verdict:** The missing directive is intentional and documented. It would only be exploitable if combined with an XSS vector in the main page, which is a separate finding class.

---

## 4. Confirmed Findings

**None.** No vulnerability meeting the triage bar (cross-account exploitation with demonstrated security impact) was identified.

---

## 5. Promising-but-Unconfirmed Leads for Parent Agent

### 5.1 [LOW] Web K CSP Permissiveness

Web K's CSP in `index.html:9` has no `default-src` or `script-src`, meaning inline/remote scripts are unrestricted. If another subagent finds an HTML injection or DOM XSS in the main page, the CSP provides no defense. Web A's stricter CSP would block such exploitation.

**Action:** Cross-reference with SUBAGENT 1 (Stored/DOM XSS) findings. If an XSS vector exists in Web K, the missing `script-src` elevates its severity.

### 5.2 [LOW] Web A EmbedFrame Uniform Sandbox

Web A applies `allow-same-origin` to all Instant View embeds uniformly, while Web K differentiates by embed type. If a path exists to inject a same-origin URL as an embed source (via server-side Instant View generation), it could bypass the sandbox.

**Action:** Would require server-side Instant View parsing to be audited — out of scope for client-side audit.

### 5.3 [INFO] `web_app_open_tg_link` Path Sanitization Gap

Web A does not strip leading slashes from `path_full` in `web_app_open_tg_link`. While the trailing slash in `TME_LINK_PREFIX` prevents host injection, Web K's explicit stripping provides better defense-in-depth. This is cosmetic (no security impact found) but worth noting as a hardening suggestion.

---

## 6. Architecture Quality Notes

Both clients demonstrate strong security design:

1. **Source-first validation:** WindowProxy identity is the primary gate — unforgeable by browser spec
2. **Defense in depth:** Origin checking, protocol validation, server allowlists, user confirmation dialogs
3. **Minimal event surface:** Each iframe type only handles events relevant to its function
4. **Server-side gating:** Sensitive operations (payments, file downloads, age verification) require server validation
5. **Proper sandbox:** `allow-top-navigation` is NOT present — Mini Apps cannot navigate the parent frame
6. **Text rendering safety:** Button text uses `wrapEmojiText` / safe DOM APIs, not `innerHTML`
7. **URL construction safety:** Protocol filtering blocks `javascript:` with tab/newline stripping (Web K) and `new URL()` parsing (Web A)

---

*Report generated for Telegram Bug Bounty security audit. Authorized security research on open-source clients.*
