#ifndef LSF_VIEWS_H
#define LSF_VIEWS_H
#include "util.h"

/* Each view fills `out` with colored lines.  The caller owns `out`
 * and must have la_init'd it. */
void collect_jobs(LineArray *out);
void collect_nodes(LineArray *out);
void collect_info(LineArray *out, const char *info_ids);
void collect_rsrc(LineArray *out);

#endif
