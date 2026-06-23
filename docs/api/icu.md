# ICU virtual module

LDK registers a small `ICU.DLL` pseudo-module for code that imports a narrow
ICU calendar/time-zone surface. The current implementation exists primarily to
support MSVC STL time-zone code paths that look up ICU entry points through the
LDK loader.

This is not a full ICU implementation.

## Loader-visible exports

The virtual module exports:

- `ucal_close`
- `ucal_get`
- `ucal_getCanonicalTimeZoneID`
- `ucal_getDefaultTimeZone`
- `ucal_getTimeZoneDisplayName`
- `ucal_getTimeZoneTransitionDate`
- `ucal_getTZDataVersion`
- `ucal_inDaylightTime`
- `ucal_open`
- `ucal_openTimeZoneIDEnumeration`
- `ucal_setMillis`
- `uenum_close`
- `uenum_count`
- `uenum_unext`

The module is registered as `ICU.DLL` with the path
`\SystemRoot\system32\ICU.DLL`. `LoadLibraryExW(L"icu.dll", ...)` can therefore
resolve it through normal LDK loader paths without reading a real ICU image from
disk.

## Time-zone subset

The built-in zone table is intentionally tiny:

- `Etc/UTC`
- `UTC` as an alias for `Etc/UTC`
- `GMT` as an alias for `Etc/UTC`
- `Asia/Seoul`, fixed at UTC+09:00
- `Asia/Tokyo`, fixed at UTC+09:00
- `Asia/Shanghai`, fixed at UTC+08:00
- `America/New_York`, fixed at UTC-05:00
- `Europe/Berlin`, fixed at UTC+01:00
- `Europe/Moscow`, fixed at UTC+03:00

The implementation returns canonical IDs, standard display names, short standard
names, raw offsets, and enumeration over the table.

## Limitations

LDK does not ship IANA tzdb data or general ICU services. Current limitations
include:

- No daylight-saving transitions. `ucal_inDaylightTime` always returns false,
  and `UCAL_DST_OFFSET` is always zero.
- `ucal_getTimeZoneTransitionDate` reports no transition.
- `ucal_setMillis` accepts the call but does not affect later offset answers.
- Locale, calendar, collation, formatting, normalization, and conversion ICU
  APIs are not implemented.
- Unknown time-zone IDs fail `ucal_open`; canonical-ID lookup falls back to
  `Etc/UTC` and marks the ID as non-system.

Callers that need real civil-time behavior must not treat this as a substitute
for ICU proper.

## Tests

`IcuTest` loads `icu.dll`, verifies that the required exports resolve, checks
time-zone enumeration, and validates fixed offsets for the built-in UTC, Korean,
Japanese, Chinese, US Eastern, German, and Russian zones.
