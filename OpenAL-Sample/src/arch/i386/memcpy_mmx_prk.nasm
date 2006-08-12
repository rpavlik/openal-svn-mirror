;***************************************************************************
;*   Copyright (C) 2005 by Prakash Punnoor                                 *
;*   prakash@punnoor.de                                                    *
;*                                                                         *
;*   This program is free software; you can redistribute it and/or modify  *
;*   it under the terms of the GNU Library General Public License as       *
;*   published by the Free Software Foundation; either version 2 of the    *
;*   License, or (at your option) any later version.                       *
;*                                                                         *
;*   This program is distributed in the hope that it will be useful,       *
;*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
;*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
;*   GNU General Public License for more details.                          *
;*                                                                         *
;*   You should have received a copy of the GNU Library General Public     *
;*   License along with this program; if not, write to the                 *
;*   Free Software Foundation, Inc.,                                       *
;*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
;***************************************************************************

; void _alMMXmemcpy(void* dst, void* src, unsigned int n);
; pretty straight-forward implementation
; by design broken for n<8, so check that before calling
; x86 32 bit only!

%ifdef HAVE_NASM_VISIBILITY
global __alMMXmemcpy:function hidden
global _alMMXmemcpy:function hidden
%else
global __alMMXmemcpy
global _alMMXmemcpy
%endif

__alMMXmemcpy:
_alMMXmemcpy:

; Save the registers affected
pushf
push edi
push esi

cld

mov edi, [esp + 16] ;char* dst
mov esi, [esp + 20] ;char* src
mov edx, [esp + 24] ;int n

; align dest
mov ecx, edi
and ecx, 7	;MMX align - 1
sub ecx, 8	;MMX align
neg ecx		;eax has pre copy bytes

sub edx, ecx	;less to copy after this
; pre copy
; copying first dwords and then
; remaining bytes wasn't faster
rep movsb

; calc MMX copy length
mov ecx, edx
and ecx, 63	;post copy bytes
shr edx, 6	;MMX copy iterations
cmp edx, 0

jz .loopend
; MMX copy
.loopstart
movq mm0, [esi]
movq mm1, [esi + 8]
movq mm2, [esi + 16]
movq mm3, [esi + 24]
movq mm4, [esi + 32]
movq mm5, [esi + 40]
movq mm6, [esi + 48]
movq mm7, [esi + 56]
movq [edi], mm0
movq [edi + 8], mm1
movq [edi + 16], mm2
movq [edi + 24], mm3
movq [edi + 32], mm4
movq [edi + 40], mm5
movq [edi + 48], mm6
movq [edi + 56], mm7
add esi, 64
add edi, 64
dec edx
jnz .loopstart
emms
.loopend

; post copy
rep movsb

; Restore registers
pop esi
pop edi
popf
ret

; prevent executable stack
%ifidn __OUTPUT_FORMAT__,elf
section .note.GNU-stack noalloc noexec nowrite progbits
%endif

%ifidn __YASM_OBJFMT__,elf
section ".note.GNU-stack" noalloc noexec nowrite progbits
%endif

%ifidn __YASM_OBJFMT__,elf32
section ".note.GNU-stack" noalloc noexec nowrite progbits
%endif
