#if _KERNEL_MODE
#include <Ldk/synchapi.h>
#else
#include <synchapi.h>
#endif

void SynchApiCompileTest()
{
    CreateEventA(NULL, FALSE, FALSE, NULL);
    CreateEventW(NULL, FALSE, FALSE, NULL);
}