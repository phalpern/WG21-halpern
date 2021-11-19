#if defined(RA_SWITCH)
# include "resource_adaptor_switch.h"
#elif defined(RA_LINEAR)
# include "resource_adaptor_linear.h"
#elif defined (RA_BINARY_SEARCH)
# include "resource_adaptor_binsearch.h"
#else
# error "RA_SWITCH, RA_LINEAR, or RA_BINARY_SEARCH must be #defined"
#endif
