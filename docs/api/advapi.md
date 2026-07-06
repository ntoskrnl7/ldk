# ADVAPI compatibility module

LDK registers a small loader-visible `ADVAPI32.DLL` pseudo-module for security
system-function compatibility required by MSVC UCRT and STL code.

## Random bytes

`advapi32!SystemFunction036` is exported, and `RtlGenRandom` is provided as the
public header alias used by the Windows SDK. The MSVC UCRT implements `rand_s`
through its internal `__acrt_RtlGenRandom` thunk, which late-binds
`SystemFunction036`; the STL `std::random_device` implementation then calls
`rand_s`.

LDK delegates this call to `BCryptGenRandom` with
`BCRYPT_USE_SYSTEM_PREFERRED_RNG`, matching the system-RNG intent of the Windows
API surface while keeping the loader-visible contract small.

Test coverage:

- `AdvapiCompatibilityTest` loads `advapi32.dll`, resolves
  `SystemFunction036`, and verifies that repeated calls modify the output
  buffer.
- The Win32 baseline test verifies the same normal-call behavior against
  Windows. It deliberately avoids invalid-buffer probes because native
  `SystemFunction036` may fault on invalid pointers.
