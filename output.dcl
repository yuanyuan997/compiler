
.TITLE output
.FILE "output.dcl"

.EXPORT _main

.IMPORT _input
.IMPORT _output

.PROC _gcd(.NOCHECK,.SIZE=0,.NODISPLAY,.ASSEMBLY=8)
  .LOCAL _u 8,4 (0,0,0)
  .LOCAL _v 12,4 (0,0,0)
.ENTRY
        pshAP   4
        derefW
        pshLit  0
        relEQ
        brFalse label0
        pshAP   0
        derefW
        popRetW
        exit
        branch label1
label0:
        pshAP   4
        derefW
        mkPar   4,0
        pshAP   0
        derefW
        pshAP   0
        derefW
        pshAP   4
        derefW
        div     noTrap
        pshAP   4
        derefW
        mul     noTrap
        sub     noTrap
        mkPar   4,4
        call    _gcd,2
        pshRetW
        popRetW
        exit
label1:
        exit
        endP
.ENDP

.PROC _main(.NOCHECK,.SIZE=8,.NODISPLAY,.ASSEMBLY=8)
  .LOCAL _x -4,4 (0,0,0)
  .LOCAL _y -8,4 (0,0,0)
.ENTRY
        call    _input,0
        pshRetW
        pshLP   -4
        assignW
        call    _input,0
        pshRetW
        pshLP   -8
        assignW
        pshLP   -4
        derefW
        mkPar   4,0
        pshLP   -8
        derefW
        mkPar   4,4
        call    _gcd,2
        pshRetW
        mkPar   4,0
        call    _output,1
        exit
        endP
.ENDP

