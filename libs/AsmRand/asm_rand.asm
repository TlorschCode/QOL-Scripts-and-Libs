%define arg1 rcx
%define arg2 rdx
%define arg3 r8
%define arg4 r9
%define arg5 r10
%define arg6 r11
%define state0 [rcx]
%define state1 [rcx + 8]

%macro store_args 1-6
    %if %0 >= 1
        MOV arg1, %1
    %endif
    %if %0 >= 2
        MOV arg2, %2
    %endif
    %if %0 >= 3
        MOV arg3,  %3
    %endif
    %if %0 >= 4
        MOV arg4,  %4
    %endif
    %if %0 >= 5
        MOV arg5,  %5
    %endif
    %if %0 >= 6
        MOV arg6,  %6
    %endif
%endmacro

section .data
    invalid_inputs_msg: DB "Invalid argument(s). Arg min cannot be greater than arg max for RNG range specification. Min: ",0
    overflow_msg: DB "OVERFLOW. no_overflow label in asm_rand.asm was reached",0



section .text

extern _throw_bad_arg_error



;| MARK: Gen uNorm
global gen_urandint
gen_urandint: ; arg1 is RNGstate, arg2 is min, arg3 is max
    ; RESERVED: r10, r11 (for gen_rand64)
    CMP arg2,arg3
    JG call_bad_arg_order
    JE ret_min

    PUSH rbx

    MOV rbx,rdx ; store min in r10

    SUB r8,rdx ; max - min (range)
    ADD r8,1 ; make inclusive of max
    MOV rax,r8 ; put range into rax
    NEG rax ; (-range)
    DIV r8 ; (-range) % range, rdx holds remainder
    MOV r9,rdx ; r9 holds threshold
    ;# Unavailable:
    ; r9 is threshold
    ; r8 is range
    ; rbx is min
    ; rcx is RNGstate
    ;# Available:
    ; rax
    ; rdx
    ; r10
    ; r11

    .retry_rand: ;# NEEDS: rax, rdx, r11, r10, r9
        MOV rdx,r10

        ;| vvv GEN_RAND64 vvv
        ; # NEEDS: rax, r11, r10, (rcx read only)
        ; where state0 is [rcx] and state1 is [rcx + 8]
        MOV rax,state0
        ADD rax,state1
        ROL rax,17
        ADD rax,state0 ; now rax holds `result`
        
        MOV r11,state1

        ; s1 ^= s0;
        XOR r11,state0
        ; s0 = rotl(s0, 49) ^ s1 ^ (s1 << 21);
        ROL qword state0,49
        XOR qword state0,r11
        MOV r10,r11
        SHL r10,21
        XOR qword state0,r10

        ; s1 = rotl(s1, 28);
        ROL r11,28

        MOV state1,r11
        ;| ^^^ GEN_RAND64 ^^^ 

        MOV rdx,rax ; store result for multiplication
        MULX rax,r10,r8 ; rdx (rand64) * r8 (range), hi → rax, low → r9
        CMP r10,r9 ; compare low 64 bits against threshold
        JL .retry_rand
    ADD rax,rbx ; add min
    POP rbx
    ret


global gen_urandintHQ
gen_urandintHQ: ; rcx is an RNGstate struct, rdx is min, and r8 is max
    ; RESERVED: r10, r11 (for gen_rand64)
    CMP arg2,arg3
    JG call_bad_arg_order
    JE ret_min

    PUSH rbx

    MOV rbx,rdx ; store min in r10

    SUB r8,rdx ; max - min (range)
    ADD r8,1 ; make inclusive of max
    MOV rax,r8 ; put range into rax
    NEG rax ; (-range)
    DIV r8 ; (-range) % range, rdx holds remainder
    MOV r9,rdx ; r9 holds threshold
    ;# Unavailable:
    ; r9 is threshold
    ; r8 is range
    ; rbx is min
    ; rcx is RNGstate
    ;# Available:
    ; rax
    ; rdx
    ; r10
    ; r11

    .retry_rand: ;# NEEDS: rax, rdx, r11, r10, r9
        MOV rdx,r10

        ;| vvv GEN_RAND64HQ vvv
        ;# NEEDS: rax, r11, r10, (rcx read only)
        MOV rax,state0
        IMUL rax,rax,5
        ROL rax,7
        IMUL rax,rax,9

        MOV r11,state1
        
        ; s1 ^= s0;
        XOR r11,qword state0
        ; s0 = rotl(s0, 49) ^ s1 ^ (s1 << 21);
        ROL qword state0,49
        XOR qword state0,r11
        MOV r10,r11
        SHL r10,21
        XOR qword state0,r10

        ; s1 = rotl(s1, 28);
        ROL r11,28

        MOV state1,r11
        ;| GEN_RAND64HQ

        MOV rdx,rax ; store result for multiplication
        MULX rax,r10,r8 ; rdx (rand64) * r8 (range), hi → rax, low → r9
        CMP r10,r9 ; Compare low 64 bits against threshold
        JL .retry_rand
    ADD rax,rbx ; add min
    POP rbx
    ret

    ; FORMULA:
    ; uint64_t bounded_rand(uint64_t range) {
    ;     uint64_t threshold = (-range) % range; // precompute

    ;     while (1) {
    ;         uint64_t x = rng();           // 64-bit random
    ;         128bit prod = (128bit)x * range;
    ;         uint64_t hi = prod >> 64;
    ;         uint64_t lo = (uint64_t)prod; // low 64 bits
    ;         if (lo >= threshold)
    ;             return hi;                // result in [0, range]
    ;         // else repeat
    ;     }
    ; }


;| MARK: Gen Norm
global gen_randint
gen_randint: ; arg1 is RNGstate, arg2 is min, arg3 is max
    ; RESERVED: r10, r11 (for gen_rand64)
    CMP arg2,arg3
    JG call_bad_arg_order
    JE ret_min

    PUSH rbx

    MOV rbx,rdx ; store min in rbx

    SUB r8,rdx ; max - min (range)
    ADD r8,1 ; make inclusive of max
    MOV rax,r8 ; put range into rax
    NEG rax ; (-range)
    XOR rdx,rdx
    DIV r8 ; (-range) % range, rdx holds remainder
    MOV r9,rdx ; r9 holds threshold
    ;# Unavailable:
    ; r9 is threshold
    ; r8 is range
    ; rbx is min
    ; rcx is RNGstate
    ;# Available:
    ; rax
    ; rdx
    ; r10
    ; r11

    .retry_rand: ;# NEEDS: rax, rdx, r11, r10, r9
        MOV rdx,r10

        ;| vvv GEN_RAND64 vvv
        ; # NEEDS: rax, r11, r10, (rcx read only)
        ; where state0 is [rcx] and state1 is [rcx + 8]
        MOV rax,state0
        ADD rax,state1
        ROL rax,17
        ADD rax,state0 ; now rax holds `result`
        
        MOV r11,state1

        ; s1 ^= s0;
        XOR r11,state0
        ; s0 = rotl(s0, 49) ^ s1 ^ (s1 << 21);
        ROL qword state0,49
        XOR qword state0,r11
        MOV r10,r11
        SHL r10,21
        XOR qword state0,r10

        ; s1 = rotl(s1, 28);
        ROL r11,28

        MOV state1,r11
        ;| ^^^ GEN_RAND64 ^^^ 

        MOV rdx,rax ; store result for multiplication
        MULX rax,r10,r8 ; rdx (rand64) * r8 (range), hi → rax, low → r10
        CMP r10,r9 ; compare low 64 bits against threshold
        JB .retry_rand
    ADD rax,rbx ; add min
    POP rbx
    ret

global gen_randintHQ
gen_randintHQ: ; arg1 is RNGstate, arg2 is min, arg3 is max
    ; RESERVED: r10, r11 (for gen_rand64)
    CMP arg2,arg3
    JG call_bad_arg_order
    JE ret_min

    PUSH rbx

    MOV rbx,rdx ; store min in rbx

    SUB r8,rdx ; max - min (range)
    ADD r8,1 ; make inclusive of max
    MOV rax,r8 ; put range into rax
    NEG rax ; (-range)
    XOR rdx,rdx
    DIV r8 ; (-range) % range, rdx holds remainder
    MOV r9,rdx ; r9 holds threshold
    ;# Unavailable:
    ; r9 is threshold
    ; r8 is range
    ; rbx is min
    ; rcx is RNGstate
    ;# Available:
    ; rax
    ; rdx
    ; r10
    ; r11

    .retry_rand: ;# NEEDS: rax, rdx, r11, r10, r9
        MOV rdx,r10

        ;| vvv GEN_RAND64HQ vvv
        ; # NEEDS: rax, r11, r10, (rcx read only)
        MOV rax,state0
        IMUL rax,rax,5
        ROL rax,7
        IMUL rax,rax,9

        MOV r11,state1
        
        ; s1 ^= s0;
        XOR r11,qword state0
        ; s0 = rotl(s0, 49) ^ s1 ^ (s1 << 21);
        ROL qword state0,49
        XOR qword state0,r11
        MOV r10,r11
        SHL r10,21
        XOR qword state0,r10

        ; s1 = rotl(s1, 28);
        ROL r11,28

        MOV state1,r11
        ;| ^^^ GEN_RAND64HQ ^^^ 

        MOV rdx,rax ; store result for multiplication
        MULX rax,r10,r8 ; rdx (rand64) * r8 (range), hi → rax, low → r10
        CMP r10,r9 ; compare low 64 bits against threshold
        JB .retry_rand
    ADD rax,rbx ; add min
    POP rbx
    ret


;| MARK: Gen Seed
global gen_seed
gen_seed:
    CMP arg1,arg2
    JG call_bad_arg_order
    JE ret_min

    ;| Seed Biasless
    ; range = max - min + 1;
    SUB arg2,arg1
    ADD rdx,1 ; rdx now holds range
    MOV r9,arg1 ; save min for later (add to rax at end)
    ; threshold = (-range) % range
    MOV rax,rdx ; rax now stores range
    MOV rcx,rdx ; rcx now stores range as well
    NEG rax ; (-range)
    XOR rdx,rdx ; clear higher 64 bits after rax
    DIV rcx ; now rdx stores the remainder of (-range) / range
    MOV r10,rdx ; now r10 holds threshold
    ; at this point, rcx stores range,
    ;                r10 stores threshold, and
    ;                rax and rdx are expendable
    ; gen seed
    .regen_seed:
        .try_seed:
            RDSEED rdx
            JNC .try_seed ; retry if it failed
        MULX rax, r8, rcx ; rax is hi, r8 is low
        CMP r8,r10
        JL .regen_seed ; if not low >= threshold, then retry, otherwise continue to the `ret` below
    ADD rax,r9
    ret

    ; FORMULA:
    ; uint64_t bounded_rand(uint64_t range) {
    ;     uint64_t threshold = (-range) % range; // precompute

    ;     while (1) {
    ;         uint64_t x = rng();           // 64-bit random
    ;         128bit prod = (128bit)x * range;
    ;         uint64_t hi = prod >> 64;
    ;         uint64_t lo = (uint64_t)prod; // low 64 bits
    ;         if (lo >= threshold)
    ;             return hi;                // result in [0, range]
    ;         // else repeat
    ;     }
    ; }



;| MARK: Gen 64
global gen_seed64
gen_seed64:
    .try_seed:
        RDSEED rax
        JNC .try_seed ; carry flag is clear, so it did not succeed
    ret

global gen_rand64
gen_rand64: ;# USES: rax, rcx, r10, r11
    ; result = rotl(s0 + s1, 17) + s0;
    MOV rax,state0
    ADD rax,state1
    ROL rax,17
    ADD rax,state0 ; now rax holds `result`
    
    MOV r11,state1

    ; s1 ^= s0;
    XOR r11,state0
    ; s0 = rotl(s0, 49) ^ s1 ^ (s1 << 21);
    ROL qword state0,49
    XOR qword state0,r11
    MOV r10,r11
    SHL r10,21
    XOR qword state0,r10

    ; s1 = rotl(s1, 28);
    ROL r11,28

    MOV state1,r11


    ret

    ; FORMULA:
    ; uint64_t xoroshiro128plusplus() {
    ;     uint64_t result = rotl(s0 + s1, 17) + s0;
    ;     s1 ^= s0;
    ;     s0 = rotl(s0, 49) ^ s1 ^ (s1 << 21);
    ;     s1 = rotl(s1, 28);
        
    ;     return result;
    ; }


global gen_rand64HQ
gen_rand64HQ: ;# USES: rax, rcx, r10, r11
    ; result = rotl(s0 * 5, 7) * 9;
    MOV rax,state0
    IMUL rax,rax,5
    ROL rax,7
    IMUL rax,rax,9

    MOV r11,state1
    
    ; s1 ^= s0;
    XOR r11,qword state0
    ; s0 = rotl(s0, 49) ^ s1 ^ (s1 << 21);
    ROL qword state0,49
    XOR qword state0,r11
    MOV r10,r11
    SHL r10,21
    XOR qword state0,r10

    ; s1 = rotl(s1, 28);
    ROL r11,28

    MOV state1,r11

    ret
    ; FORMULA:
    ; uint64_t xoroshiro128starstar() {
    ;     uint64_t result = rotl(s0 * 5, 7) * 9;
    ;     s1 ^= s0;
    ;     s0 = rotl(s0, 49) ^ s1 ^ (s1 << 21);
    ;     s1 = rotl(s1, 28);
        
    ;     return result;
    ; }



;| MARK: Seed State
global seed_state
seed_state:
    CALL gen_seed64
    MOV qword state0,rax ; populate state0
   
   
    ;# SPLITMIX64
    ADD rax,0x9E3779B97F4A7C15 ; seed += 0x9E3779B97F4A7C15

    ; z = (z XOR (z >> 30)) * 0xBF58476D1CE4E5B9
    MOV r11,rax ; r11 = z
    SHR r11,30 ; (z >> 30)
    XOR rax,r11 ; z XOR (z >> 30)
    IMUL rax,0xBF58476D1CE4E5B9

    ; z = (z XOR (z >> 27)) * 0x94D049BB133111EB
    MOV r11,rax
    SHR r11,27 ; (z >> 27)
    XOR rax,r11 ; z XOR (z >> 27)
    IMUL rax,0x94D049BB133111EB
    
    ; return z XOR (z >> 31)
    MOV r11,rax
    SHR r11,31  ; (z >> 31)
    XOR rax,r11 ; z XOR (z >> 31)
    ; FORMULA:
    ;     seed += 0x9E3779B97F4A7C15
    ;     z = seed
    ;     z = (z XOR (z >> 30)) * 0xBF58476D1CE4E5B9
    ;     z = (z XOR (z >> 27)) * 0x94D049BB133111EB
    ;     return z XOR (z >> 31)
    ;# END SPLITMIX64


    MOV qword state1,rax ; populate state1. 8 Bytes ahead, since arg1 is a struct containing two uint64's
    ret





;| MARK: Err
call_bad_arg_order:
    CALL err__bad_arg_order
    ret
ret_min:
    MOV rax,arg2
    ret

no_overflow: ; this code should never be reached. If it is, then something has gone wrong
    LEA rcx,[rel overflow_msg]
    CALL _throw_bad_arg_error
    ret

err__bad_arg_order:
    LEA rcx,[rel invalid_inputs_msg]
    MOV r9,rax
    CALL _throw_bad_arg_error
    ret
