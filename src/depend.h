#ifndef DEPEND
#define DEPEND

#include "system.h"

/*
 * The code here mainly involves an interface for creating graphs etc., but
 * most of it is implicit: we just add dependencies/runs and undo them again
 * later.
 */

void dependInit (const System sys);
void dependPrint ();
void dependDone (const System sys);

/*
 * The push code returns true or false: if false, the operation fails because
 * it there is now a cycle in the graph, and there is no need to pop the
 * result.
 */
void dependPushRun (const System sys);
void dependPopRun ();
int dependPushEvent (const int r1, const int e1, const int r2, const int e2);
void dependPopEvent ();

/*
 * Test/set
 */

int getNode (const int n1, const int n2);
void setNode (const int n1, const int n2);
int isDependEvent (const int r1, const int e1, const int r2, const int e2);
void setDependEvent (const int r1, const int e1, const int r2, const int e2);

/*
 * Outside helpers
 */
int hasCycle ();
int eventNode (const int r, const int e);
int nodeCount (void);

#endif