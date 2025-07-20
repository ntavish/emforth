: if immediate
    ' 0branch ,    \ compile 0branch instruction
    here           \ push here to the stack, this is where the offset will be stored
    0 ,            \ compile dummy offset, it will overwrite the current here
;

: then immediate
    dup
	here swap -	\ calculate the offset from the address saved on the stack
	swap !		    \ store the offset in the back-filled location
;

: else immediate
	' branch ,	\ definite branch to just over the false-part
	here		\ save location of the offset on the stack
	0 ,		    \ compile a dummy offset
	swap		\ now back-fill the original (if) offset
	dup		    \ same as for then word above
	here swap -
	swap !
;

: print-if-true
    if
        65 emit  \ A
        13 emit 10 emit
    else
        66 emit  \ B
        13 emit 10 emit
    then
        67 emit 13 emit 10 emit  \ C
;

see print-if-true

.s
1 1 = print-if-true
.s
1 0 = print-if-true
.s
