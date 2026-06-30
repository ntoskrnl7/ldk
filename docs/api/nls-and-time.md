# NLS, strings, and time formatting

LDK implements a pragmatic subset of Kernel32 NLS and string conversion APIs.
The code is intended to satisfy loader/runtime dependencies and selected STL
paths, not to provide complete Windows globalization behavior.

## Code pages and conversion

`LdkpInitializeNls` creates a process-local cache for code-page tables and
queries the system locale. Non-algorithmic code pages are loaded from NLS table
files such as `\SystemRoot\System32\C_<codepage>.NLS` and cached until
termination.

Supported conversion paths include:

- `MultiByteToWideChar`
- `WideCharToMultiByte`
- `GetACP`
- `GetOEMCP`
- `IsValidCodePage`
- `GetCPInfo`
- `GetCPInfoExA/W`

The conversion APIs handle ACP, OEMCP, UTF-8, and loaded custom code pages.
UTF-8 flag validation is implemented for the supported flag set. Unsupported
or invalid code-page requests fail through normal Win32 last-error paths.

## Locale data

LDK provides locale-name and LCID helpers:

- `IsValidLocale`
- `LCIDToLocaleName`
- `LocaleNameToLCID`
- `GetUserDefaultLCID`
- `GetUserDefaultLocaleName`
- `EnumSystemLocalesW`
- `EnumSystemLocalesEx`
- `GetLocaleInfoW`
- `GetLocaleInfoEx`

Locale-name mapping covers a broad static LCID/name table, while detailed locale
formatting data is intentionally small. The current detailed data table covers:

- `en-US`
- `de-DE`
- `en-GB`
- `ko-KR`
- `zh-CN`
- `ja-JP`
- `ru-RU`

Other valid locale names may map to an LCID but fall back to the default detail
record for many `GetLocaleInfo*` values.

The East Asian and Russian detail records include localized country/language
codes, native display/language/country names, currency symbols, AM/PM markers
where applicable, typical script tags (`Hang;`, `Hans;`, `Jpan;`, and `Cyrl;`),
OpenType language tags, date/time patterns, and localized day/month names used
by `GetDateFormatW`.

## Date and time formatting

`GetDateFormatW`, `GetDateFormatEx`, `GetTimeFormatW`, and `GetTimeFormatEx`
format dates and times using the internal locale data table. The `Ex` variants
resolve locale names through the same locale-name mapping used by
`GetLocaleInfoEx`. The formatter supports the common year, month, day, hour,
minute, second, and AM/PM pattern tokens used by the current tests and runtime
paths. It is not a complete NLS format-engine clone.

`FileTimeToSystemTime`, `SystemTimeToFileTime`, `GetTimeZoneInformation`, and
`SystemTimeToTzSpecificLocalTime` are backed by RTL time conversion helpers and
kernel time-zone state.

## Character classification

`GetStringTypeW` supports the `CT_CTYPE1`, `CT_CTYPE2`, and `CT_CTYPE3`
classes. ASCII classification is explicit. The implementation also recognizes
representative Hangul, CJK ideograph, Hiragana, Katakana, and Cyrillic ranges
for direction and `CT_CTYPE3` script flags. This is still not a full Unicode
category database.

## Tests

`NlsTest` covers representative locale-name/LCID round trips, locale
enumeration, `GetLocaleInfoEx`, `GetDateFormatW/Ex`, `GetTimeFormatW/Ex`, and
script classification for `ko-KR`, `zh-CN`, `ja-JP`, and `ru-RU`. Additional
code-page conversions and format tokens should still get direct tests when
expanded.
