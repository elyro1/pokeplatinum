#include "types.h"
#include "gfl/tcb.h"

// Tasks in a list sorted by priority, run in order each update, from a fixed pool in a caller's buffer. The ROM embeds
// no name for this file; it is named after Game Freak's library's

struct TCB {
    TCBManager *manager;
    TCB *prev;
    TCB *next;
    u32 priority;
    void *data;
    TCBFunc func;
    // Added during an update after the task that was running, so not run until the next update
    BOOL addedDuringUpdate;
};

struct TCBManager {
    u32 count;
    // The tasks taken from ready
    u32 used;
    // The head of the circular list of running tasks
    TCB head;
    TCB **ready;
    TCB *tasks;
    BOOL adding;
    TCB *current;
    TCB *next;
};

static void GFL_TCBMgrInitTasks(TCBManager *manager);
static TCB *GFL_TCBMgrGetReadyTask(TCBManager *manager);
static BOOL GFL_TCBMgrNotifyReadyTask(TCBManager *manager, TCB *tcb);
static void GFL_TCBMgrSetup(TCBManager *manager);
static TCB *GFL_TCBMgrAddTaskCore(TCBManager *manager, TCBFunc func, void *data, u32 priority);

static void GFL_TCBMgrInitTasks(TCBManager *manager) {
    u32 i;

    for (i = 0; i < manager->count; i++) {
        TCB *tcb = &manager->tasks[i];

        tcb->manager = manager;
        tcb->next = &manager->head;
        tcb->prev = &manager->head;
        tcb->priority = 0;
        tcb->data = NULL;
        tcb->func = NULL;
        manager->ready[i] = &manager->tasks[i];
    }
    manager->used = 0;
}

static TCB *GFL_TCBMgrGetReadyTask(TCBManager *manager) {
    TCB *tcb;

    if (manager->used == manager->count) {
        return NULL;
    }
    tcb = manager->ready[manager->used];
    manager->used++;
    return tcb;
}

static BOOL GFL_TCBMgrNotifyReadyTask(TCBManager *manager, TCB *tcb) {
    if (manager->used == 0) {
        return FALSE;
    }
    tcb->manager = manager;
    tcb->next = &manager->head;
    tcb->prev = &manager->head;
    tcb->priority = 0;
    tcb->data = NULL;
    tcb->func = NULL;
    manager->ready[--manager->used] = tcb;
    return TRUE;
}

u32 GFL_TCBMgrCalcAllocSize(u32 count) {
    return count * (sizeof(TCB *) + sizeof(TCB)) + sizeof(TCBManager);
}

TCBManager *GFL_TCBMgrCreate(u32 count, void *buffer) {
    TCBManager *manager = buffer;
    TCB **ready = (TCB **)(manager + 1);
    TCB *tasks = (TCB *)(ready + count);

    manager->ready = ready;
    manager->tasks = tasks;
    manager->count = count;
    manager->used = 0;
    manager->adding = FALSE;
    GFL_TCBMgrSetup(manager);
    return manager;
}

static void GFL_TCBMgrSetup(TCBManager *manager) {
    GFL_TCBMgrInitTasks(manager);
    manager->head.manager = manager;
    manager->head.next = &manager->head;
    manager->head.prev = &manager->head;
    manager->head.priority = 0;
    manager->head.data = NULL;
    manager->head.func = NULL;
    manager->current = &manager->head;
}

void GFL_TCBMgrUpdate(TCBManager *manager) {
    if (manager->adding) {
        return;
    }
    manager->current = manager->head.next;
    while (manager->current != &manager->head) {
        manager->next = manager->current->next;
        if (!manager->current->addedDuringUpdate) {
            if (manager->current->func != NULL) {
                manager->current->func(manager->current, manager->current->data);
            }
        } else {
            manager->current->addedDuringUpdate = FALSE;
        }
        manager->current = manager->next;
    }
    manager->current->func = NULL;
}

void func_0203a610(TCBManager *manager) {
}

TCB *GFL_TCBMgrAddTask(TCBManager *manager, TCBFunc func, void *data, u32 priority) {
    TCB *tcb;

    manager->adding = TRUE;
    tcb = GFL_TCBMgrAddTaskCore(manager, func, data, priority);
    manager->adding = FALSE;
    return tcb;
}

static TCB *GFL_TCBMgrAddTaskCore(TCBManager *manager, TCBFunc func, void *data, u32 priority) {
    TCB *it;
    TCB *tcb = GFL_TCBMgrGetReadyTask(manager);

    if (tcb == NULL) {
        return NULL;
    }
    tcb->priority = priority;
    tcb->data = data;
    tcb->func = func;
    if (manager->current->func != NULL) {
        if (manager->current->priority <= priority) {
            tcb->addedDuringUpdate = TRUE;
        } else {
            tcb->addedDuringUpdate = FALSE;
        }
    } else {
        tcb->addedDuringUpdate = FALSE;
    }
    for (it = manager->head.next; it != &manager->head; it = it->next) {
        if (it->priority > tcb->priority) {
            tcb->prev = it->prev;
            tcb->next = it;
            it->prev->next = tcb;
            it->prev = tcb;
            if (it == manager->next) {
                manager->next = tcb;
            }
            return tcb;
        }
    }
    if (manager->next == &manager->head) {
        manager->next = tcb;
    }
    tcb->prev = manager->head.prev;
    tcb->next = &manager->head;
    manager->head.prev->next = tcb;
    manager->head.prev = tcb;
    return tcb;
}

BOOL GFL_TCBRemove(TCB *tcb) {
    TCBManager *manager = tcb->manager;

    if (manager->next == tcb) {
        manager->next = tcb->next;
    }
    tcb->prev->next = tcb->next;
    tcb->next->prev = tcb->prev;
    return GFL_TCBMgrNotifyReadyTask(tcb->manager, tcb);
}

void GFL_TCBSetCallbackFunc(TCB *tcb, TCBFunc func) {
    tcb->func = func;
}

void *GFL_TCBGetData(TCB *tcb) {
    return tcb->data;
}
