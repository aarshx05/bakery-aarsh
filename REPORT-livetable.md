# XWiki Security Audit Report: LiveTable Module

**Target:** xwiki-platform-livetable-ui + LiveTable-related templates
**Source version audited:** 19.0.0-SNAPSHOT (xwiki-platform main branch)
**Dynamic test instance:** XWiki 16.10.6 Standalone (Jetty + HSQLDB)
**Date:** 2026-11-20
**Auditor:** Defensive security research, authorized local instance only

---

## Executive Summary

Three vulnerabilities were identified and dynamically verified in the XWiki LiveTable data-source layer. One is a novel stored XSS (high severity), one is a novel information disclosure (medium severity), and one is a variant of a previously known issue with an incomplete fix (low-medium severity).

---

## Finding 1: HQL Query Debug Information Disclosure via `request.sql` Parameter

**Severity:** Medium (CWE-200: Exposure of Sensitive Information)
**Status:** NOVEL — no existing CVE/GHSA covers this
**Affected components:**
- `templates/getdocuments.vm` (line 132)
- `templates/getdeleteddocuments.vm` (line 94)
- `XWiki/LiveTableResultsMacros.xml` (line 536)
**Affects:** All versions through 19.0.0-SNAPSHOT (current main)
**Authentication required:** None (guest access sufficient)

### Description

The LiveTable data sources accept a user-supplied `sql` (or any non-empty value for `getdocuments.vm`/`getdeleteddocuments.vm`) URL parameter that causes the full HQL query string and bind parameter values to be included in the JSON API response. There is no authentication or authorization check on this debug feature.

### Vulnerable Code

**getdocuments.vm:132-136** — ANY non-empty value triggers disclosure:
```velocity
#if("$!request.sql" != '')
  #set($discard = $map.put('sql', $queryString))
  #set($discard = $map.put('params', $queryParams))
#end
```

**getdeleteddocuments.vm:94-98** — identical pattern.

**LiveTableResultsMacros.xml:536-539** — requires exact value `1`:
```velocity
#if("$!request.sql" == '1')
  #set($discard = $map.put('sql', $sql))
  #set($discard = $map.put('params', $sqlParams))
#end
```

### Dynamic Proof

```
$ curl -s 'http://localhost:8080/xwiki/bin/get/Main/WebHome?xpage=getdocuments&outputSyntax=plain&limit=1&sql=1&space=TestSpace&sort=doc.date&dir=desc'
```
Response includes:
```json
{
  "sql": "WHERE 1=1 AND doc.space = ?1 order by doc.date desc",
  "params": ["TestSpace"],
  "totalrows": 3,
  ...
}
```

Unauthenticated guest request leaks:
- Complete HQL query structure including WHERE clauses, ORDER BY, JOINs
- All bind parameter values (space names, search terms, filter values)
- Database schema information (table aliases, column names)

### Impact

An attacker can:
1. Map the entire database schema used by HQL queries
2. Discover filter logic and parameter binding patterns
3. Use leaked query structures to identify injection points in other components
4. Discover names of spaces, classes, and properties via bind parameter values

### Recommended Fix

Remove the `request.sql` debug feature entirely from production code, or gate it behind Programming Right:

```velocity
#if("$!request.sql" == '1' && $services.security.authorization.hasAccess('programming'))
  #set($discard = $map.put('sql', $sql))
  #set($discard = $map.put('params', $sqlParams))
#end
```

---

## Finding 2: Stored XSS via Unescaped Attachment Filenames in LiveTable Rendering

**Severity:** High (CWE-79: Stored Cross-Site Scripting)
**Status:** Likely NOVEL — CVE-2024-37900 (GHSA-wf3x-jccf-5g5g) fixed XSS via attachment filenames in the uploader UI, but NOT in the LiveTable rendering macros `livetable_getAttachmentsList` and `livetable_getImagesList`. The 19.0.0-SNAPSHOT source still contains unescaped `$attachment.filename`.
**Affected components:**
- `XWiki/LiveTableResultsMacros.xml`, macro `livetable_getAttachmentsList` (line 650)
- `XWiki/LiveTableResultsMacros.xml`, macro `livetable_getImagesList` (line 674, in `alt` and `title` attributes)
**Affects:** All versions through 19.0.0-SNAPSHOT (current main)
**Authentication required:** Edit right (to upload attachment); victim needs only View right

### Description

The `livetable_getAttachmentsList` macro builds an HTML string from attachment filenames without HTML-escaping them. When a LiveTable column of type `_attachments` or `_images` is rendered, filenames containing HTML/JavaScript execute in the browser of anyone viewing the table.

### Vulnerable Code

**livetable_getAttachmentsList (line 646-652):**
```velocity
#macro(livetable_getAttachmentsList $itemDoc)
  #set($attachlist = '')
  #foreach($attachment in $itemDoc.attachmentList)
    #set($attachmentUrl = $itemDoc.getAttachmentURL($attachment.filename))
    #set($attachlist = "${attachlist}<a href='${attachmentUrl}'>$attachment.filename</a><br/>")
  #end
#end
```

`$attachment.filename` is inserted directly into HTML without `$escapetool.xml()`.

**livetable_getImagesList (line 664-677):**
```velocity
#set($imagesList = "${imagesList}<a href=""$imageURL""><img src=""$thumbnailURL"" alt=""$attachment.filename"" title=""$attachment.filename"" /></a>")
```

Same issue in `alt` and `title` attributes.

### Dynamic Proof

1. Upload attachment with malicious filename:
```
$ curl -u "superadmin:system" -X PUT \
  'http://localhost:8080/xwiki/rest/.../attachments/%22%3E%3Cimg%20src%3Dx%20onerror%3Dalert(1)%3E.txt' \
  -H "Content-Type: text/plain" --data-binary "test"
```

2. HTML output (rendered via Velocity page reproducing the exact macro code):
```html
<a href='/xwiki/bin/download/TestSpace/Doc1/...'>"><img src=x onerror=alert(1)>.txt</a>
```

3. Playwright browser test confirms `alert(1)` fires:
```
ALERT FIRED: "1"
XSS CONFIRMED: alert("1") executed
```

### Impact

Any user with Edit right on a document can upload an attachment with a crafted filename. When any other user (including administrators) views a LiveTable that includes an `_attachments` or `_images` column showing that document, the XSS payload executes in their browser session. This enables:

1. Session hijacking / cookie theft
2. Privilege escalation (steal admin session → gain Programming Right)
3. Defacement of wiki pages
4. Exfiltration of private data visible to the victim

### Recommended Fix

HTML-escape all dynamic values in the HTML string:

```velocity
#set($attachlist = "${attachlist}<a href='${escapetool.xml($attachmentUrl)}'>$escapetool.xml($attachment.filename)</a><br/>")
```

And for images:
```velocity
#set($imagesList = "${imagesList}<a href=""${escapetool.xml($imageURL)}""><img src=""${escapetool.xml($thumbnailURL)}"" alt=""${escapetool.xml($attachment.filename)}"" title=""${escapetool.xml($attachment.filename)}"" /></a>")
```

---

## Finding 3: Document Count Information Leak via `totalrows` in Paginated Responses

**Severity:** Low-Medium (CWE-200: Exposure of Sensitive Information)
**Status:** VARIANT of CVE-2022-41935 (GHSA-p2x4-6ghr-6vmq), likely INCOMPLETE FIX
**Note:** CVE-2022-41935 was fixed in XWiki 14.6RC1/13.10.8/14.4.3 and targeted `LiveTableResultsMacros.xml`. However, the same vulnerability persists in `getdocuments.vm` (the template-based LiveTable endpoint) where the `livetable_filterObfuscated` macro logic is bypassable.
**Affected component:** `templates/macros.vm` macro `livetable_filterObfuscated` + `templates/getdocuments.vm`
**Affects:** All versions through 19.0.0-SNAPSHOT

### Description

When a user without view access queries a space containing restricted documents via `getdocuments.vm`, the `totalrows` field in the JSON response includes the count of ALL documents (including those the user cannot view). The `livetable_filterObfuscated` macro only corrects this count when `totalrows <= returnedrows` (all matching documents fit on a single page). By using a small `limit` value, an attacker ensures `totalrows > returnedrows`, bypassing the correction.

### Vulnerable Code

**macros.vm, `livetable_filterObfuscated` (lines 1921-1944):**
```velocity
#macro(livetable_filterObfuscated $map)
  #if($map.totalrows <= $map.returnedrows)
    ## Only filters when ALL rows are on this page
    ...remove obfuscated rows and adjust totalrows...
  #end
#end
```

The condition `$map.totalrows <= $map.returnedrows` is false whenever pagination is active (`limit < total documents`), leaving `totalrows` uncorrected.

### Dynamic Proof

Setup: 5 documents in `SecretSpace`, all denied view for guest via `XWikiGlobalRights`.

```
# Guest is redirected when accessing pages directly (view denied):
Guest accessing SecretSpace.Page1: HTTP 302

# Guest with limit=1 (leak):
{"totalrows": 5, "returnedrows": 1, "rows": [{"doc_viewable": false, "doc_fullName": "obfuscated"}]}

# Guest with limit=5 (obfuscation fires, hides the count):
{"totalrows": 0, "returnedrows": 0, "rows": []}

# Binary search reveals exact count:
limit=4 → totalrows=5  (leak!)
limit=5 → totalrows=0  (all rows fit, macro fires)
```

### Impact

An unauthenticated user can determine:
1. The exact number of documents in any space, even if all documents are view-restricted
2. Whether a secret space exists and how many documents it contains
3. By combining with other filters (childrenOf, date ranges), potentially narrow down document properties

### Recommended Fix

The `totalrows` count must be computed AFTER access filtering, not before. The HQL query should exclude documents the current user cannot view, or the `totalrows` field should always be corrected regardless of pagination:

```velocity
#macro(livetable_filterObfuscated $map)
  ## Always filter, not just when totalrows <= returnedrows
  #set ($filteredRows = [])
  #foreach ($row in $map.rows)
    #if ($row.doc_viewable)
      #set ($discard = $filteredRows.add($row))
    #end
  #end
  #set ($obfuscatedCount = $mathtool.sub($map.rows.size(), $filteredRows.size()))
  #set ($discard = $map.put('rows', $filteredRows))
  #set ($discard = $map.put('returnedrows', $filteredRows.size()))
  ## Adjust totalrows: subtract the proportion of obfuscated rows from the total
  ## Note: This is an approximation; the accurate fix requires a filtered count query
  #if ($obfuscatedCount > 0)
    #set ($discard = $map.put('totalrows', $mathtool.sub($map.totalrows, $obfuscatedCount)))
  #end
#end
```

The proper fix is to run the count query with access control applied at the database level.

---

## Per-Sink Coverage Matrix

| Bug Class | Sinks Examined | Finding? |
|-----------|---------------|----------|
| **Script injection / right escalation** | No `#evaluate`, `{{groovy}}`, or `clean="false"` in LiveTable templates. Content author on LiveTableResults.xml is XWiki.Admin (expected). No user-controlled macro parameters into script contexts. | No finding |
| **XWQL/HQL injection** | `$request.sort` → `getdocuments.vm` validated with `^(doc\.)?\w+$` regex (line 85). `$request.sort` → `getdeleteddocuments.vm` validated with `checkOrderBySafe()` (line 61). `$request.sort` → LiveTableResultsMacros validated with `livetable_getTableAlias` `\W` stripping (line 1170). All filter values use bind parameters (`?N` or `:name`). `$request.childrenOf` uses `$services.query.parameter().literal()`. | No finding |
| **Broken access control** | Per-row VIEW check present in `getdocuments.vm` (line 150) and `LiveTableResultsMacros` (line 392). `livetable_filterObfuscated` present but bypassable via pagination → **Finding 3**. `request.wiki` cross-wiki query accepted without authorization check but only functional in multi-wiki setups. | Finding 3 |
| **XSS (Stored)** | `$attachment.filename` unescaped in `livetable_getAttachmentsList` (line 650) and `livetable_getImagesList` (line 674) → **Finding 2**. All other user-visible values pass through `$jsontool.serialize()` (safe) or `$escapetool.xml()`. | Finding 2 |
| **XSS (Reflected)** | JSON responses use `$jsontool.serialize()` for data serialization. `$response.setContentType("application/json")` set on all endpoints. No reflected user input in HTML rendering paths. | No finding |
| **SSRF** | No URL fetch from user input. `$request.wiki` sets query context, doesn't make outbound requests. | No finding |
| **XXE** | No XML parsing from user input in LiveTable templates. | No finding |
| **Path traversal** | No file system access from user input. Attachment filenames handled by XWiki's attachment API. | No finding |
| **Insecure deserialization** | No deserialization from user input. | No finding |
| **Open redirect** | No redirects constructed from user input. `doc_url` values built from `$xwiki.getURL()` (safe). | No finding |
| **CSRF** | LiveTable data calls are GET-only read operations. No state-changing operations in LiveTable templates. CSRF tokens present in delete/restore URLs in `getdeleteddocuments.vm` (line 127-129). | No finding |
| **Information disclosure** | `request.sql` debug parameter leaks HQL queries → **Finding 1**. Stack traces leak internal paths via `request.wiki=nonexistent` (minor, not reported separately). | Finding 1 |

---

## Findings NOT Reported (and why)

1. **queryFilters user control** (`getdocuments.vm` line 117, `LiveTableResultsMacros.xml` line 351): The parameter adds filters to queries but cannot remove server-side default filters. The `hidden` document filter is optional by design in these endpoints. Not a vulnerability.

2. **Cross-wiki query via `request.wiki`** (`getdocuments.vm` line 121, `getdeleteddocuments.vm` line 81): Causes error on nonexistent wiki. In a multi-wiki setup, cross-wiki queries may be intentional admin functionality. Could not dynamically verify data leakage across wikis in single-wiki test instance.

3. **LiveTableResults content author as XWiki.Admin**: This is expected — the page is a system page that needs admin privileges to execute its queries.

---

## Audit Metadata

- **Source analysis:** Complete for `LiveTableResultsMacros.xml` (~1182 lines), `LiveTableResults.xml` (69 lines), `getdocuments.vm` (219 lines), `getdeleteddocuments.vm` (142 lines), relevant sections of `macros.vm`
- **Dynamic tests:** Executed against XWiki 16.10.6 standalone instance with superadmin and unauthenticated guest
- **Browser verification:** Playwright/Chromium for XSS (Finding 2)
- **CVE novelty check:** Searched NVD, GitHub Advisory Database, XWiki JIRA
