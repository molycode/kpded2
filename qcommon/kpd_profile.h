// Profiling hooks: Tracy zones in a KPD_ENABLE_TRACY build, nothing otherwise

#ifdef KPD_TRACY

#include <tracy/TracyC.h>

#define KPD_ZONE(ctx, name)	TracyCZoneN (ctx, name, 1)
#define KPD_ZONE_END(ctx)	TracyCZoneEnd (ctx)
#define KPD_FRAME()			TracyCFrameMark

#else

#define KPD_ZONE(ctx, name)
#define KPD_ZONE_END(ctx)
#define KPD_FRAME()

#endif // KPD_TRACY
