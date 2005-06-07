" Vim compiler file
" Compiler:     splint/lclint (C source code checker)
" Maintainer:   Ralf Wildenhues <Ralf.Wildenhues@gmx.de>
" Last Change:  $Date: 2005-03-22 07:47:37 $
" $Revision: 1.1 $

if exists("current_compiler")
  finish
endif
let current_compiler = "splint"

" adapt this if you want to check more than one file at a time.
" put command line options in .splintrc or ~/.splintrc
setlocal makeprg=splint\ %

" Note: when using the new array bounds checking flags:  Each warning
" usually has several lines and several references to source code mostly
" within one or two lines (see sample warning below).  The easiest way
" not to mess up file name detection and not to jump to all positions is
" to add something like
"	-linelen 500 +boundscompacterrormessages
" to your .splintrc and 'set cmdheight=4' or more.
" TODO: reliable way to distinguish file names and constraints.
"
" sample warning (generic):
"
"foo.c:1006:12: Clauses exit with var referencing local storage in one
"                      case, fresh storage in other case
"   foo.c:1003:2: Fresh storage var allocated
"
" sample warning (bounds checking):
"
"bounds.c: (in function updateEnv)
"bounds.c:10:5: Possible out-of-bounds store:
"    strcpy(str, tmp)
"    Unable to resolve constraint:
"    requires maxSet(str @ bounds.c:10:13) >= maxRead(getenv("MYENV") @
"    bounds.c:6:9)
"     needed to satisfy precondition:
"    requires maxSet(str @ bounds.c:10:13) >= maxRead(tmp @ bounds.c:10:18)
"     derived from strcpy precondition: requires maxSet(<parameter 1>) >=
"    maxRead(<parameter 2>)
"  A memory write may write to an address beyond the allocated buffer. (Use
"  -boundswrite to inhibit warning)

setlocal errorformat=%OLCLint*m,
	\%OSplint*m,
	\%*[\ ]%f:%l:%c:\ %m,
	\%*[\ ]%f:%l:\ %m,
	\%*[^\"]\"%f\"%*\\D%l:\ %m,
	\\"%f\"%*\\D%l:\ %m,
	\%A%f:%l:%c:\ %m,
	\%A%f:%l:%m,
	\\"%f\"\\,
	\\ line\ %l%*\\D%c%*[^\ ]\ %m,
	\%D%*\\a[%*\\d]:\ Entering\ directory\ `%f',
	\%X%*\\a[%*\\d]:\ Leaving\ directory\ `%f',
	\%DMaking\ %*\\a\ in\ %f,
	\%C\ \ %m
