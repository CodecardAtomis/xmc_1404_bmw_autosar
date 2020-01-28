
    PRESERVE8

  IMPORT  kern_curr_run_task
  IMPORT  kern_next_task_to_run
  IMPORT  kern_int_counter
  IMPORT  kern_context_switch_request
  IMPORT  kern_system_state


  EXPORT  kern_switch_context_exit
  EXPORT  kern_switch_context
  EXPORT  kern_iswitch_context
  EXPORT  kern_cpu_save_sr
  EXPORT  kern_cpu_restore_sr
  EXPORT  kern_start_exe
  EXPORT  kern_chk_irq_disabled
  EXPORT  PendSV_Handler
  EXPORT  kern_int_exit
  EXPORT  kern_arm_disable_interrupts
  EXPORT  kern_arm_enable_interrupts



ICSR_ADDR        EQU     0xE000ED04
PENDSVSET        EQU     0x10000000
PR_12_15_ADDR    EQU     0xE000ED20
PENDS_VPRIORITY  EQU     0x00FF0000

;----------------------------------------------------------------------------
;
;----------------------------------------------------------------------------
       SECTION    .text:CODE(2)
       THUMB
       REQUIRE8
       PRESERVE8


;----------------------------------------------------------------------------
; Interrups not yet enabled
;----------------------------------------------------------------------------
kern_start_exe:

       ldr      r1, =PR_12_15_ADDR        
       ldr      r0, [r1]
       ldr      r2, =PENDS_VPRIORITY
       orrs     r0, r0, r2  
       str      r0, [r1]

       ldr      r1,=kern_system_state       ;-- Indicate that system has started
       movs     r0,#1                       ;-- 1 -> KERN_SYS_STATE_RUNNING
       strb     r0,[r1]

  ;---------------

       ldr      r0, =kern_context_switch_request  ; set the context switch
       movs     r1, #1
       str      r1, [r0]

       ldr      r1, =kern_curr_run_task     
       ldr      r2, [r1]
       ldr      r0, [r2]                  ;-- in r0 - new task SP

        ADDS    R0,R0,#16               // set pointer to R8-R11
        LDMIA   R0!,{R4-R7}             /* Restore new Context (R8-R11) */
        MOV     R8,R4
        MOV     R9,R5
        MOV     R10,R6
        MOV     R11,R7

        MSR     PSP,R0                  /* Write PSP */
        
        SUBS    R0,R0,#32               /* Adjust Start Address */
        LDMIA   R0!,{R4-R7}             /* Restore new Context (R4-R7) */
        
        //SUBS    R0,R0,#16
        
       //ldm      r0, {r4,r5}                     ;{r4-r11}
       //LDMIA   R0,{R4-R10,R11}           /* Read R0-R3,R12 from stack */       
       
       //msr      PSP, r0
       movs     r0, #4
       mov      r1, lr
       orrs     r0, r0, r1             ;-- Force to new process PSP
       mov      lr, r0
       
kern_switch_context_exit:

       ldr    r1, =ICSR_ADDR            ;-- Trigger PendSV exception
       ldr    r0, =PENDSVSET
       str    r0, [r1]

       cpsie  I                         ;-- Enable core interrupts

  ;--- Never reach this

       b  .
    ;  bx lr

;----------------------------------------------------------------------------
kern_iswitch_context:
PendSV_Handler:

       cpsid  I                         ;-- Disable core int

  ;--- The context switching - to do or not

       ldr      r0, =kern_context_switch_request
       ldr      r1, [r0]
       cmp      r1, #0                    ;-- if there is no context
       beq      exit_context_switch       ;-- switching - return
       movs     r1, #0
       str      r1, [r0]

  ;----------------------------------------

       mrs      r0, PSP                   ;-- in PSP - process(task) stack pointer
       //stmdb    r0!, {r4-r11}
       
       SUBS    R0,R0,#32               /* Adjust Start Address */
       
       STMIA    R0!,{R4-R7}             /* Save old context (R4-R7) */
       MOV      R4,R8
       MOV      R5,R9
       MOV      R6,R10
       MOV      R7,R11
       STMIA    R0!,{R4-R7}             /* Save old context (R8-R11) */
       
       SUBS     R0,R0,#32     // su situo veike
       
       ldr    r3, =kern_curr_run_task
       mov    r1,  r3
       ldr    r1, [r1]
       str    r0, [r1]                  ;-- save own SP in TCB

       ldr    r1, =kern_next_task_to_run
       ldr    r2, [r1]
       
       cmp    r2, #0
       beq    next_task_null
       
       str    r2, [r3]                  ;-- in r3 - =tn_curr_run_task
       ldr    r0, [r2]                  ;-- in r0 - new task SP

next_task_null:

       //ldmia  r0!, {r4-r11}
       //SUBS     R0,R0,#16
       ADDS     R0,R0,#16
       LDMIA    R0!,{R4-R7}             /* Restore new Context (R8-R11) */
       MOV      R8,R4
       MOV      R9,R5
       MOV      R10,R6
       MOV      R11,R7 
       
       msr      PSP, r0
       
       SUBS     R0,R0,#32
       LDMIA    R0!,{R4-R7} 
       //ADDS     R0,R0,#16
       
       //msr      PSP, r0

        movs     r0, #4
        mov      r1, lr
        orrs     r0, r0, r1             ;-- Force to new process PSP
        mov      lr, r0        

  ;----------------------------------------

exit_context_switch:

       cpsie  I                         ;-- enable core int

       bx     lr

;-----------------------------------------------------------------------------
kern_switch_context:

     ;--- set the context switch request

       ldr      r0, =kern_context_switch_request
       movs     r1, #1
       str      r1, [r0]

     ;---  Just Invoke PendSV exception

       ldr      r1, =ICSR_ADDR
       ldr      r0, =PENDSVSET
       str      r0, [r1]

       bx       lr

;-----------------------------------------------------------------------------
kern_int_exit:

       ldr    r1, =ICSR_ADDR            ;-- Invoke PendSV exception
       ldr    r0, =PENDSVSET
       str    r0, [r1]

       bx     lr

;-----------------------------------------------------------------------------
kern_cpu_save_sr:

       mrs    r0, PRIMASK
       cpsid  I
       bx     lr

;-----------------------------------------------------------------------------
kern_cpu_restore_sr:

       msr    PRIMASK, r0
       bx     lr

;-----------------------------------------------------------------------------
kern_chk_irq_disabled:

       mrs    r0,PRIMASK
       bx     lr

; -------------------------------------------------------------------------
kern_arm_disable_interrupts

     cpsid  I
     bx     lr


; -------------------------------------------------------------------------
kern_arm_enable_interrupts

     cpsie I
     bx   lr
;----------------------------------------------------------------------------

      END
