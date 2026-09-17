[bits 32]
section .text

global context_switch
global irq0_stub
extern timer_handler
extern current_process

; struct process has:
; offset 0:  pid (4 bytes)
; offset 4:  name (32 bytes)
; offset 36: esp (4 bytes)  <-- offset of esp inside process_t is 36

; Cooperative context switch: void context_switch(uint32_t *old_esp, uint32_t new_esp);
context_switch:
    push ebp
    push edi
    push esi
    push ebx
    push edx
    push ecx
    push eax
    pushfd

    mov edx, [esp + 36]     ; old_esp
    mov [edx], esp

    mov esp, [esp + 40]     ; new_esp

    popfd
    pop eax
    pop ecx
    pop edx
    pop ebx
    pop esi
    pop edi
    pop ebp
    sti
    ret

; Preemptive Timer Interrupt handler
irq0_stub:
    ; Push all CPU registers to form an interrupt stack frame
    push ebp
    push edi
    push esi
    push ebx
    push edx
    push ecx
    push eax
    pushfd

    ; Save outgoing process ESP
    mov eax, [current_process]
    test eax, eax
    jz .call_handler
    mov [eax + 36], esp     ; save esp at current_process->esp

.call_handler:
    call timer_handler      ; picks next task and updates current_process

    ; Restore incoming process ESP
    mov eax, [current_process]
    test eax, eax
    jz .restore
    mov esp, [eax + 36]     ; switch to current_process->esp

.restore:
    popfd
    pop eax
    pop ecx
    pop edx
    pop ebx
    pop esi
    pop edi
    pop ebp
    iret