# SUBAGENT 2 REPORT: Profile/Metadata + Deep-Link Audit

## Scope
- **Web K** (tweb): commit `4cef169d`, cloned at `/home/user/bakery-aarsh/audit/tweb`
- **Web A** (telegram-tt): commit `4566a14`, cloned at `/home/user/bakery-aarsh/audit/telegram-tt`
- Attack classes: Stored/DOM XSS via profile metadata rendering, deep-link/URL routing abuse (open redirects, navigation to attacker-controlled destinations), URL protocol bypass

---

## CONFIRMED FINDINGS

**None.** Both clients are well-hardened against all attack classes audited.

---

## PROMISING BUT UNCONFIRMED LEADS

### 1. Web K `data:` Protocol Not Blocked (Theoretical Only)

**Location**: `src/lib/richTextProcessor/matchUrlProtocol.ts`, lines 3-4

```typescript
const SKIP_PROTOCOLS = new Set(['javascript:']);
```

**Analysis**: Web K's `SKIP_PROTOCOLS` only blocks `javascript:`, while `data:` passes through `normalizeUrlProtocol`. A crafted `messageEntityTextUrl` with `url: "data:text/html,<script>alert(1)</script>"` would survive protocol filtering.

**Why it is NOT practically exploitable**:
1. Chrome 60+ / Firefox 59+ / Safari block `data:` URL top-level navigation from anchor clicks
2. Telegram server-side validation is unlikely to relay `data:` URLs in entity URL fields
3. Masked URL confirmation dialog (`confirmOpenUrl`) adds a user interaction gate
4. `safeWindowOpen` re-applies `normalizeUrlProtocol` before `window.open`
5. CSP headers would still need to allow inline scripts on the `data:` page

**Contrast with Web A**: Web A uses a whitelist approach (`PROTOCOL_WHITELIST = ['http:', 'https:', 'tg:', 'ton:', 'mailto:', 'tel:']`) which blocks `data:` by design.

**Recommendation**: Web K should add `data:` to `SKIP_PROTOCOLS` (or switch to a whitelist approach like Web A) as defense-in-depth.

**Bug bounty triage**: Does NOT meet the bar. No cross-account impact. Browser-level mitigations prevent exploitation in all supported browsers.

---

## PER-AREA COVERAGE VERDICT TABLE

| Area | Web K (tweb) | Web A (telegram-tt) | Verdict |
|------|-------------|---------------------|---------|
| **URL Protocol Sanitization** | `SKIP_PROTOCOLS` = `['javascript:']` blocklist. `normalizeUrlProtocol` strips C0/tabs/newlines. `data:` passes. | `PROTOCOL_WHITELIST` = `['http:','https:','tg:','ton:','mailto:','tel:']`. Non-listed gets `https://` prepended. | Web A stronger (whitelist vs blocklist). Both block `javascript:`. |
| **Entity → DOM Rendering** | `wrapRichText` → `DocumentFragment` via DOM APIs. `setInnerHTML` uses `textContent`/`replaceChildren`, never actual `innerHTML`. | JSX mode: Teact elements (React-like). HTML mode: only used for clipboard copy, with `escapeHtmlAttribute`/`escape_html`. No `dangerouslySetInnerHTML`. | Both safe. No innerHTML injection path. |
| **Profile Name (Display Name)** | `wrapEmojiText` filters to only `messageEntityEmoji` entities → `wrapRichText` → `DocumentFragment` → `setInnerHTML` (safe). | Profile names rendered via Teact JSX components. No entity processing on names. | Both safe. No dangerous entities on names. |
| **Profile Bio / Chat Description** | `wrapRichText(about, {whitelistedDomains})` → standard URL sanitization pipeline. | `renderText(description, ['br', 'links'/'tg_links', 'emoji'])` → JSX SafeLink components. | Both safe. URLs go through respective sanitization. |
| **Poll Question/Answer Text** | `TranslatableMessageTsx` → `wrapRichText` → `DocumentFragment`. | `renderTextWithEntities({text, entities})` → JSX mode. | Both safe. Standard rendering pipeline. |
| **Inline Bot Results** | Title: `wrapEmojiText`. Description: `wrapRichText({noLinks: true})`. Both via `setInnerHTML` (safe). | `ArticleResult.tsx` etc. — no `renderText`/`SafeLink` on result titles (plain text rendering). | Both safe. |
| **Keyboard Button URLs** | Synthetic `messageEntityTextUrl` → `wrapRichText` pipeline → same sanitization. | `<SafeLink url={ensureProtocol(url)}>` on button URLs. | Both safe. Same pipeline as message URLs. |
| **Deep Link: share/msg_url** | Creates `INTERNAL_LINK_TYPE.SHARE` → `processShareLink` → draft message composition. No navigation. | `openChatWithDraft({text: formatShareText(url, text)})`. No navigation. | Both safe. No open redirect. |
| **Deep Link: tg://resolve** | Routes to username/phone lookup actions. `startapp` creates web app link. All app-internal. | Routes to `openChatByUsername` / `openChatByPhoneNumber`. All app-internal. | Both safe. No external navigation. |
| **Deep Link: tg://iv** | Decodes URL param, re-applies `normalizeUrlProtocol`. | Not explicitly handled (falls through to general tg:// switch). | Both safe. |
| **Deep Link: oauth** | `processOauthLink` restricted to same-origin contexts. | `linkContext?.type !== 'inner'` guard — rejects non-inner contexts. | Both safe. |
| **Masked URL Confirmation** | `confirmOpenUrl` popup shows full URL, click re-applies `normalizeUrlProtocol` via `safeWindowOpen`. | `SafeLinkModal` shows full URL, opens via `window.open(ensureProtocol(url))`. | Both safe. URL re-sanitized at open time. |
| **Homograph/Suspicious URL Detection** | `isSuspiciousUrl`: checks userinfo abuse, punycode hostname, mixed-script labels (UTS #39). | `isSuspiciousUrl`: checks userinfo credentials, punycode domain, mixed-script labels via full Unicode Script_Extensions. | Both robust. Web A has more thorough script detection. |
| **DOM Clobbering Prevention** | `hasAnchorHandler`: `isAnchorHandlerName` regex (`^[A-Za-z_$][\w$]*$`) + `Object.prototype.hasOwnProperty.call(window, name)` + `typeof === 'function'`. | N/A — no window function registration for click handlers. | Web K safe. Web A uses React event system (inherently safe). |
| **innerHTML Usage** | `setInnerHTML` is safe wrapper. `htmlToDocumentFragment` uses `template.innerHTML` but only for string input (wrapRichText returns DocumentFragment). | Limited to: KaTeX output, SVG clip paths, clipboard operations, Teact internals, composer template parsing. All controlled. | Both safe. No attacker-controlled innerHTML. |

---

## AREAS NOT FULLY AUDITED

| Area | Reason | Risk Assessment |
|------|--------|----------------|
| Sticker set name rendering | Not traced end-to-end | Low — likely uses same emoji-only text rendering |
| Gift/premium feature descriptions | Not traced end-to-end | Low — UI chrome text, not user-controlled content |
| Channel/group description rendering in Web A's specific UI panel | Covered via `ChatExtra.tsx` which uses `renderText` (safe JSX) | Covered |
| Dynamic cross-account verification | No live builds tested | Static analysis shows no exploitable paths |

---

## METHODOLOGY

1. **Static code audit**: Traced all data flows from MTProto entity types through rendering pipelines to DOM output
2. **Protocol sanitization**: Compared blocklist (Web K) vs whitelist (Web A) approaches
3. **innerHTML audit**: Searched all `innerHTML` assignments and `dangerouslySetInnerHTML` usage
4. **Deep link routing**: Traced all `tg://` and `t.me/` link handlers for open redirect potential
5. **Profile metadata**: Traced display name, bio, description, poll text through their specific rendering paths
6. **Masked URL bypass**: Checked confirmation dialog flows for re-sanitization at open time

## CONCLUSION

Both Telegram web clients demonstrate mature, defense-in-depth security architecture for profile/metadata rendering and deep-link handling. Web A's whitelist-based protocol filtering is strictly stronger than Web K's blocklist approach, though both effectively block practical exploitation. No cross-account XSS, open redirect, or navigation-to-attacker-destination vulnerabilities were found that meet the bug bounty triage bar (must execute in victim's session, not self-XSS/DoS).
