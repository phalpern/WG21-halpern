#include "stdafx.h"
#include "task_run.h"

__declspec(thread) structured_task_groupEx* tls_current_task_group = nullptr;
