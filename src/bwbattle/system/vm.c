#include "types.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "system/vm.h"

void VM_Reset(VM *vm) {
    vm->pc = NULL;
    vm->cmpResult = 0;
    vm->state = VM_STATE_STOPPED;
    vm->native = NULL;
    vm->stackPos = 0;
    MI_CpuFill8(vm->stack, 0, vm->param.stackSize * sizeof(uintptr_t));
    MI_CpuFill8(vm->work, 0, vm->param.workSize * sizeof(u32));
    vm->verifier = NULL;
    vm->verifierArg = NULL;
}

VM *VM_Create(HeapID heapId, const VMInitParam *param) {
    // bwbattle: the stack comes first, so its wider slots stay aligned
    u32 size = sizeof(VM) + param->stackSize * sizeof(uintptr_t) + param->workSize * sizeof(u32);
    VM *vm = GFL_HeapAllocate(heapId, size, FALSE, "vm.c", 96);

    MI_CpuFill8(vm, 0, size);
    vm->param = *param;
    vm->stack = (uintptr_t *)(vm + 1);
    vm->work = (u32 *)(vm->stack + param->stackSize);
    return vm;
}

void VM_Free(VM *vm) {
    GFL_HeapFree(vm);
}

void VM_ChangeEnv(VM *vm, void *env) {
    VM_Reset(vm);
    vm->env = env;
}

void VM_LoadScript(VM *vm, const void *script) {
    VM_Reset(vm);
    vm->pc = script;
    vm->state = VM_STATE_RUNNING;
}

void VM_Halt(VM *vm) {
    vm->state = VM_STATE_STOPPED;
    vm->pc = NULL;
}

BOOL VM_Run(VM *vm) {
    u16 cmd;

    switch (vm->state) {
    case VM_STATE_STOPPED:
        return FALSE;
    case VM_STATE_WAITING:
        if (vm->native != NULL) {
            if (vm->native(vm, vm->env) == TRUE) {
                vm->native = NULL;
                vm->state = VM_STATE_RUNNING;
            }
            return TRUE;
        }
        vm->state = VM_STATE_RUNNING;
        // fallthrough
    case VM_STATE_RUNNING:
        while (TRUE) {
            if (vm->pc == NULL) {
                vm->state = VM_STATE_STOPPED;
                return FALSE;
            }
            cmd = VM_Read16(vm);
            if (cmd >= vm->param.commandCount) {
                if (cmd < vm->param.extraCommandStart ||
                    cmd >= vm->param.extraCommandCount + vm->param.extraCommandStart ||
                    vm->param.extraCommands == NULL) {
                    vm->state = VM_STATE_STOPPED;
                    return FALSE;
                }
                if (vm->param.extraCommands[cmd - vm->param.extraCommandStart](vm, vm->env) == TRUE) {
                    break;
                }
            } else {
                if (vm->verifier != NULL && vm->verifier(vm, vm->env, vm->verifierArg, cmd) == FALSE) {
                    vm->state = VM_STATE_STOPPED;
                    return FALSE;
                }
                if (vm->param.commands[cmd](vm, vm->env) == TRUE) {
                    break;
                }
            }
        }
        break;
    default:
        vm->state = VM_STATE_STOPPED;
        break;
    }
    return TRUE;
}

void *VM_GetEnv(VM *vm) {
    return vm->env;
}

void VM_SetCallbackVerifier(VM *vm, VMVerifier verifier, void *arg) {
    vm->verifier = verifier;
    vm->verifierArg = arg;
}

u16 VM_Read16(VM *vm) {
    u16 value = VM_Read8(vm);

    value += (u16)(VM_Read8(vm) << 8);
    return value;
}

u32 VM_Read32(VM *vm) {
    u8 byte0 = VM_Read8(vm);
    u8 byte1 = VM_Read8(vm);
    u8 byte2 = VM_Read8(vm);
    u8 byte3 = VM_Read8(vm);
    u32 value = 0;

    value += byte3;
    value <<= 8;
    value += byte2;
    value <<= 8;
    value += byte1;
    value <<= 8;
    value += byte0;
    return value;
}

void VM_StackPush(VM *vm, uintptr_t value) {
    if (vm->stackPos + 1 < vm->param.stackSize) {
        vm->stack[vm->stackPos] = value;
        vm->stackPos++;
    }
}

uintptr_t VM_StackPop(VM *vm) {
    if (vm->stackPos == 0) {
        return 0;
    }
    return vm->stack[--vm->stackPos];
}

void VM_Jump(VM *vm, const u8 *pc) {
    vm->pc = pc;
}

void VM_Call(VM *vm, const u8 *pc) {
    VM_StackPush(vm, (uintptr_t)vm->pc);
    vm->pc = pc;
}

void VM_Return(VM *vm) {
    vm->pc = (const u8 *)VM_StackPop(vm);
}

void VM_SetNativeCallback(VM *vm, VMCommand native) {
    vm->state = VM_STATE_WAITING;
    vm->native = native;
}
