#include "stdafx.h"
#include "task_run.h"

__declspec(thread) int tls_current_task_group_depth = 0;
