# Jellyfin v12.4.0 Security Audit Report

**Target:** Jellyfin Media Server v12.4.0 (Docker: `jellyfin/jellyfin:latest`)
**Date:** 2026-11-25
**Scope:** Authorized defensive security research on own local Docker instance
**Methodology:** Source code review + dynamic verification on live instance

---

## Executive Summary

This audit identified **3 confirmed, dynamically-verified vulnerabilities** in Jellyfin v12.4.0 that expose user data and server internals to any authenticated non-admin user. All findings were verified on a live Docker instance with multi-user test environment. No path traversal, XSS, or SSRF vulnerabilities exploitable by non-admin users were found.

---

## Novelty Assessment

| Finding | Known CVE | Status |
|---------|-----------|--------|
| Finding 1 (IDOR /Users/{id}) | CVE-2022-35909 partially overlaps (Jellyfin <10.8.0 access control on /Users). The per-user ownership check on GET was not added in that fix. | **Regression / incomplete fix** — novel in v12.4.0 |
| Finding 2 (User list /Users) | CVE-2022-35909 covered this for <10.8.0 | **Regression** — still present in v12.4.0 |
| Finding 3a (/System/Info paths) | No known CVE | **Novel** |
| Finding 3b (/System/Configuration) | No known CVE | **Novel** |

---

## Finding 1: Cross-User IDOR on User Profile Endpoint

**Severity:** Medium (CVSS 6.5)
**CWE:** CWE-639 (Authorization Bypass Through User-Controlled Key)
**Endpoint:** `GET /Users/{userId}`
**Auth Required:** Any authenticated user (DefaultAuthorization policy)

### Description

Any authenticated user can read the full profile of any other user, including the administrator, by supplying their userId in the URL path. The endpoint uses `[Authorize(Policy = Policies.IgnoreParentalControl)]` which requires authentication but performs no ownership check on the target userId.

### Root Cause

**File:** `Jellyfin.Api/Controllers/UserController.cs` lines 127-142

The `GetUserById` method retrieves any user by GUID without comparing the requesting user's ID against the target user's ID. While write operations (`UpdateUser` at line 351) correctly call `RequestHelpers.AssertCanUpdateUser()` for ownership verification, the read operation omits this check.

### Dynamic Proof

```
# userA (non-admin) reads userB's full profile:
GET /Users/{userB_ID}
Authorization: MediaBrowser Token="<userA_token>"

Response 200:
{
  "Name": "userB",
  "Policy": {
    "IsAdministrator": false,
    "AuthenticationProviderId": "Jellyfin.Server.Implementations.Users.DefaultAuthenticationProvider",
    "EnableAllFolders": false,
    "EnabledFolders": ["a656b907eb3a73532e40e44b968d0225"]
  },
  "HasPassword": true
}

# userA also reads admin profile:
GET /Users/{admin_ID} → 200, exposes IsAdministrator=true, AuthenticationProviderId
```

### Data Exposed

Per user: Name, Id, ServerId, HasPassword, HasConfiguredPassword, HasConfiguredEasyPassword, EnableAutoLogin, Policy (full object including IsAdministrator, AuthenticationProviderId, PasswordResetProviderId, EnableAllFolders, EnabledFolders list, all permission flags), Configuration, LastLoginDate, LastActivityDate.

### Impact

- Attacker enumerates all user IDs (via Finding 2), then reads every user's complete profile
- Leaks which users are administrators (privilege escalation reconnaissance)
- Leaks authentication provider configuration
- Leaks library access restrictions (which folders each user can see)
- Facilitates targeted attacks against admin accounts

### Suggested Fix

Add ownership check to `GetUserById`:
```csharp
[HttpGet("{userId}")]
[Authorize(Policy = Policies.IgnoreParentalControl)]
public ActionResult<UserDto> GetUserById([FromRoute, Required] Guid userId)
{
    RequestHelpers.AssertCanUpdateUser(_authContext, HttpContext, userId, true);
    // ... existing code
}
```
Or create a read-only equivalent that verifies `authenticatedUserId == userId || isAdministrator`.

---

## Finding 2: User Enumeration via Unrestricted User List

**Severity:** Medium (CVSS 5.3)
**CWE:** CWE-200 (Exposure of Sensitive Information)
**Endpoint:** `GET /Users`
**Auth Required:** Any authenticated user (DefaultAuthorization policy)

### Description

Any authenticated user can list all users on the server, including administrators. The response includes each user's Id, Name, HasPassword, Policy (including IsAdministrator), and other profile data. This enables user enumeration and is a prerequisite for exploiting Finding 1.

### Root Cause

**File:** `Jellyfin.Api/Controllers/UserController.cs` lines 91-96

```csharp
[HttpGet]
[Authorize]
public ActionResult<IReadOnlyList<UserDto>> GetUsers(...)
```

The endpoint uses `[Authorize]` (default policy) with no additional restriction. Any authenticated user retrieves the complete user list.

### Dynamic Proof

```
# userA (non-admin) lists all users:
GET /Users
Authorization: MediaBrowser Token="<userA_token>"

Response 200: [
  {"Name":"admin", "Policy":{"IsAdministrator":true}, ...},
  {"Name":"userA", "Policy":{"IsAdministrator":false}, ...},
  {"Name":"userB", "Policy":{"IsAdministrator":false}, ...}
]
```

### Impact

- Any authenticated user discovers all usernames and user IDs
- Identifies administrator accounts for targeted attacks
- Combined with Finding 1, enables full profile enumeration of all users
- Facilitates brute-force attacks against specific user accounts

### Suggested Fix

Restrict `GetUsers` to admin only, or filter the response so non-admin users see only their own entry (or a minimal subset needed for client functionality like session display names).

---

## Finding 3: Server Configuration and Internal Paths Disclosed to Non-Admin Users

**Severity:** Low-Medium (CVSS 4.3)
**CWE:** CWE-200 (Exposure of Sensitive Information)
**Endpoints:**
- `GET /System/Info` — 26 keys including internal filesystem paths
- `GET /System/Configuration` — 53 server configuration keys

### Description

Any authenticated user can read detailed server system information and full server configuration. The `/System/Info` endpoint exposes internal filesystem paths (ProgramDataPath, CachePath, LogPath, TranscodingTempPath, InternalMetadataPath, WebPath) and the server's internal IP address. The `/System/Configuration` endpoint exposes 53 configuration keys including security-relevant settings.

### Root Cause

**File:** `Jellyfin.Api/Controllers/SystemController.cs` line 68
```csharp
[HttpGet("Info")]
[Authorize(Policy = Policies.FirstTimeSetupOrIgnoreParentalControl)]
```

**File:** `Jellyfin.Api/Controllers/ConfigurationController.cs` lines 40-45
```csharp
[HttpGet("Configuration")]
// Class-level: [Authorize] (default policy = any authenticated user)
public ActionResult<ServerConfigurationDto> GetConfiguration()
```

Both read endpoints use permissive policies accessible to any authenticated user. The corresponding write endpoints (`POST /System/Configuration` and `POST /System/Info/Storage`) correctly require `RequiresElevation` (admin), creating a read/write authorization asymmetry.

### Dynamic Proof

```
# Non-admin user reads /System/Info:
GET /System/Info → 200
Sensitive keys exposed:
  ProgramDataPath: /config
  CachePath: /cache
  LogPath: /config/log
  TranscodingTempPath: /cache/transcodes
  InternalMetadataPath: /config/metadata
  WebPath: /jellyfin/jellyfin-web
  LocalAddress: http://172.17.0.2:8096

# Non-admin reads /System/Configuration → 200, 53 keys including:
  IsStartupWizardCompleted: True
  EnableLegacyAuthorization: False
  QuickConnectAvailable: True
  RemoteClientBitrateLimit: 0
```

### Impact

- Internal filesystem layout aids crafting path traversal or file inclusion attacks
- Internal IP address aids network reconnaissance and SSRF targeting
- Configuration settings reveal security posture (legacy auth, brute force settings, etc.)
- May assist in exploiting other vulnerabilities or planning further attacks

### Suggested Fix

Restrict `/System/Info` and `/System/Configuration` to admin users only, or create filtered DTOs that expose only the subset of information non-admin users need (e.g., server name and version for display purposes).

---

## Findings Investigated But Not Reported (No Exploitable Impact)

### SSRF (Area 3)
- LiveTV tuner/listing endpoints accept attacker-controlled URLs and make server-side requests → **admin-only** (`RequiresElevation`), intended behavior
- `/Images/Remote` → 404 in v12 (endpoint removed or renamed)
- `/Items/RemoteSearch/*` → Returns results but no SSRF callback observed
- No SSRF vectors exploitable by non-admin users

### Path Traversal (Area 4)
- Static analysis of 8 controllers (ImageController, SubtitleController, HlsSegmentController, BackupController, etc.) found **all use safe patterns**: GUID→database lookups, `PathHelper.IsContainedIn()` checks, `Path.GetFullPath` canonicalization + `StartsWith` containment checks
- Dynamic tests with encoded traversal sequences returned 404 (blocked)

### XSS (Area 5)
- Library name with `<script>` tags: angle brackets stripped on storage (stored as plain text with brackets removed)
- User display name with XSS payload: rejected with 400 (validation)
- Search terms: reflected in JSON responses, not HTML (Content-Type: application/json)
- API-first architecture means all responses are JSON, making reflected XSS in API responses non-exploitable without a secondary injection point

### Auth & Session (Area 6)
- **ApiKey in URL query string**: Session tokens accepted via `?ApiKey=` parameter. While this works and tokens would appear in proxy logs and Referer headers, this is documented behavior and common in media server APIs. Classified as a **design concern**, not a vulnerability.
- **Brute force**: `LoginAttemptsBeforeLockout` defaults to -1 (no lockout). This is a configurable setting, not a code defect.
- **Quick Connect**: Secret and code exposed in initiate response. This is by design — the protocol requires the client to display the code for the user to authorize.
- Sessions endpoint shows only the requesting user's own sessions (not other users')
- Devices endpoint returns 403 for non-admin users

---

## Coverage Matrix

| Area | Endpoints Tested | Bug Classes | Result |
|------|-----------------|-------------|--------|
| 1. Authorization | /Users, /Users/{id}, /Users/New, /Users/{id}/Policy, /Users/{id} DELETE, /System/Info, /System/Configuration, /Sessions, /Devices | Missing auth, auth bypass | 3 findings |
| 2. IDOR | /Users/{id} GET/POST/DELETE, /Users/{id}/Policy, /Users/{id}/Password | Cross-user read/write | 1 finding (read-only IDOR) |
| 3. SSRF | /Items/{id}/RemoteImages/Download, /Items/RemoteSearch/*, /LiveTv/TunerHosts, /LiveTv/ListingProviders, /Images/Remote | Blind/full SSRF | 0 (admin-only) |
| 4. Path Traversal | /Images/General/*, /Videos/hls/*, /FallbackFont/Fonts/*, ImageController, HlsSegmentController | ../ encoding bypass | 0 |
| 5. XSS | Library names, user names, search terms, API responses | Stored/reflected XSS | 0 |
| 6. Auth & Session | /Users/AuthenticateByName, /QuickConnect/*, ?ApiKey= param, brute force | Token leak, session flaws | 0 (design concerns only) |
| 7. Info Disclosure | /System/Info/Public, /System/Info, /System/Configuration, /System/Logs, /Devices, /Sessions | Sensitive data exposure | 2 findings (included in Finding 3) |

---

## GitHub Security Advisory Format

### Advisory 1: Cross-User IDOR on GET /Users/{userId}

- **Package:** jellyfin/jellyfin
- **Affected versions:** <= 12.4.0
- **Patched versions:** (none)
- **Severity:** Medium
- **CWE:** CWE-639
- **CVSS:** CVSS:3.1/AV:N/AC:L/PR:L/UI:N/S:U/C:H/I:N/A:N (6.5)

### Advisory 2: User Enumeration via GET /Users

- **Package:** jellyfin/jellyfin
- **Affected versions:** <= 12.4.0
- **Patched versions:** (none)
- **Severity:** Medium
- **CWE:** CWE-200
- **CVSS:** CVSS:3.1/AV:N/AC:L/PR:L/UI:N/S:U/C:L/I:N/A:N (4.3)

### Advisory 3: Server Configuration Disclosure to Non-Admin Users

- **Package:** jellyfin/jellyfin
- **Affected versions:** <= 12.4.0
- **Patched versions:** (none)
- **Severity:** Medium
- **CWE:** CWE-200
- **CVSS:** CVSS:3.1/AV:N/AC:L/PR:L/UI:N/S:U/C:L/I:N/A:N (4.3)
