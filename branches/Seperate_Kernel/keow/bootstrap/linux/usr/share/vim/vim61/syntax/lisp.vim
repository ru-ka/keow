" Vim syntax file
" Language:    Lisp
" Maintainer:  Dr. Charles E. Campbell, Jr. <Charles.E.Campbell.1@gsfc.nasa.gov>
" Last Change: Dec 10, 2001
" Version:     1.11
" Latest:      http://www.erols.com/astronaut/vim/index.html#vimlinks_syntax
"
"  Thanks to F Xavier Noria for a list of 978 Common Lisp symbols
"  taken from the HyperSpec
"
"  Options:
"    lisp_instring : if it exists, then "(...") strings are highlighted
"                    as if the contents were lisp.  Useful for AutoLisp.
"                    Put    let lisp_instring=1   into your <.vimrc> if
"                    you want this option.

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

if version >= 600
 setlocal iskeyword=42,43,45,47-58,60-62,64-90,97-122,_
else
 set iskeyword=42,43,45,47-58,60-62,64-90,97-122,_
endif

" Clusters
syn cluster	lispAtomCluster	contains=lispAtomBarSymbol,lispAtomList,lispAtomNmbr0,lispComment,lispDecl,lispFunc,lispLeadWhite
syn cluster	lispListCluster	contains=lispAtom,lispAtomBarSymbol,lispAtomMark,lispBQList,lispBarSymbol,lispComment,lispConcat,lispDecl,lispFunc,lispKey,lispList,lispNumber,lispSpecial,lispSymbol,lispVar,lispLeadWhite

" Lists
syn match	lispSymbol	contained	![^()'`,"; \t]\+!
syn match	lispBarSymbol	contained	!|..\{-}|!
if exists("lisp_instring")
 syn region	lispList	matchgroup=Delimiter start="(" skip="|.\{-}|"	matchgroup=Delimiter end=")" contains=@lispListCluster,lispString,lispInString,lispInStringString
 syn region	lispBQList	matchgroup=PreProc   start="`("	skip="|.\{-}|"	matchgroup=PreProc   end=")" contains=@lispListCluster,lispString,lispInString,lispInStringString
else
 syn region	lispList	matchgroup=Delimiter start="(" skip="|.\{-}|"	matchgroup=Delimiter end=")" contains=@lispListCluster,lispString
 syn region	lispBQList	matchgroup=PreProc   start="`("	skip="|.\{-}|"	matchgroup=PreProc   end=")" contains=@lispListCluster,lispString
endif
" Atoms
syn match	lispAtomMark	"'"
syn match	lispAtom	"'("me=e-1	contains=lispAtomMark	nextgroup=lispAtomList
syn match	lispAtom	"'[^ \t()]\+"	contains=lispAtomMark
syn match	lispAtomBarSymbol	!'|..\{-}|!	contains=lispAtomMark
syn region	lispAtom	start=+'"+	skip=+\\"+ end=+"+
syn region	lispAtomList	contained	matchgroup=Special start="("	skip="|.\{-}|" matchgroup=Special end=")"	contains=@lispAtomCluster,lispString
syn match	lispAtomNmbr	contained	"\<\d\+"
syn match	lispLeadWhite	contained	"^\s\+"

" Standard Lisp Functions and Macros
syn keyword lispFunc	*	find-method	pprint-indent
syn keyword lispFunc	**	find-package	pprint-linear
syn keyword lispFunc	***	find-restart	pprint-logical-block
syn keyword lispFunc	+	find-symbol	pprint-newline
syn keyword lispFunc	++	finish-output	pprint-pop
syn keyword lispFunc	+++	first	pprint-tab
syn keyword lispFunc	-	fixnum	pprint-tabular
syn keyword lispFunc	/	flet	prin1
syn keyword lispFunc	//	float	prin1-to-string
syn keyword lispFunc	///	float-digits	princ
syn keyword lispFunc	/=	float-precision	princ-to-string
syn keyword lispFunc	1+	float-radix	print
syn keyword lispFunc	1-	float-sign	print-not-readable
syn keyword lispFunc	<	floating-point-inexact	print-not-readable-object
syn keyword lispFunc	<=	floating-point-invalid-operation	print-object
syn keyword lispFunc	=	floating-point-overflow	print-unreadable-object
syn keyword lispFunc	>	floating-point-underflow	probe-file
syn keyword lispFunc	>=	floatp	proclaim
syn keyword lispFunc	abort	floor	prog
syn keyword lispFunc	abs	fmakunbound	prog*
syn keyword lispFunc	access	force-output	prog1
syn keyword lispFunc	acons	format	prog2
syn keyword lispFunc	acos	formatter	progn
syn keyword lispFunc	acosh	fourth	program-error
syn keyword lispFunc	add-method	fresh-line	progv
syn keyword lispFunc	adjoin	fround	provide
syn keyword lispFunc	adjust-array	ftruncate	psetf
syn keyword lispFunc	adjustable-array-p	ftype	psetq
syn keyword lispFunc	allocate-instance	funcall	push
syn keyword lispFunc	alpha-char-p	function	pushnew
syn keyword lispFunc	alphanumericp	function-keywords	putprop
syn keyword lispFunc	and	function-lambda-expression	quote
syn keyword lispFunc	append	functionp	random
syn keyword lispFunc	apply	gbitp	random-state
syn keyword lispFunc	applyhook	gcd	random-state-p
syn keyword lispFunc	apropos	generic-function	rassoc
syn keyword lispFunc	apropos-list	gensym	rassoc-if
syn keyword lispFunc	aref	gentemp	rassoc-if-not
syn keyword lispFunc	arithmetic-error	get	ratio
syn keyword lispFunc	arithmetic-error-operands	get-decoded-time	rational
syn keyword lispFunc	arithmetic-error-operation	get-dispatch-macro-character	rationalize
syn keyword lispFunc	array	get-internal-real-time	rationalp
syn keyword lispFunc	array-dimension	get-internal-run-time	read
syn keyword lispFunc	array-dimension-limit	get-macro-character	read-byte
syn keyword lispFunc	array-dimensions	get-output-stream-string	read-char
syn keyword lispFunc	array-displacement	get-properties	read-char-no-hang
syn keyword lispFunc	array-element-type	get-setf-expansion	read-delimited-list
syn keyword lispFunc	array-has-fill-pointer-p	get-setf-method	read-eval-print
syn keyword lispFunc	array-in-bounds-p	get-universal-time	read-from-string
syn keyword lispFunc	array-rank	getf	read-line
syn keyword lispFunc	array-rank-limit	gethash	read-preserving-whitespace
syn keyword lispFunc	array-row-major-index	go	read-sequence
syn keyword lispFunc	array-total-size	graphic-char-p	reader-error
syn keyword lispFunc	array-total-size-limit	handler-bind	readtable
syn keyword lispFunc	arrayp	handler-case	readtable-case
syn keyword lispFunc	ash	hash-table	readtablep
syn keyword lispFunc	asin	hash-table-count	real
syn keyword lispFunc	asinh	hash-table-p	realp
syn keyword lispFunc	assert	hash-table-rehash-size	realpart
syn keyword lispFunc	assoc	hash-table-rehash-threshold	reduce
syn keyword lispFunc	assoc-if	hash-table-size	reinitialize-instance
syn keyword lispFunc	assoc-if-not	hash-table-test	rem
syn keyword lispFunc	atan	host-namestring	remf
syn keyword lispFunc	atanh	identity	remhash
syn keyword lispFunc	atom	if	remove
syn keyword lispFunc	base-char	if-exists	remove-duplicates
syn keyword lispFunc	base-string	ignorable	remove-if
syn keyword lispFunc	bignum	ignore	remove-if-not
syn keyword lispFunc	bit	ignore-errors	remove-method
syn keyword lispFunc	bit-and	imagpart	remprop
syn keyword lispFunc	bit-andc1	import	rename-file
syn keyword lispFunc	bit-andc2	in-package	rename-package
syn keyword lispFunc	bit-eqv	in-package	replace
syn keyword lispFunc	bit-ior	incf	require
syn keyword lispFunc	bit-nand	initialize-instance	rest
syn keyword lispFunc	bit-nor	inline	restart
syn keyword lispFunc	bit-not	input-stream-p	restart-bind
syn keyword lispFunc	bit-orc1	inspect	restart-case
syn keyword lispFunc	bit-orc2	int-char	restart-name
syn keyword lispFunc	bit-vector	integer	return
syn keyword lispFunc	bit-vector-p	integer-decode-float	return-from
syn keyword lispFunc	bit-xor	integer-length	revappend
syn keyword lispFunc	block	integerp	reverse
syn keyword lispFunc	boole	interactive-stream-p	room
syn keyword lispFunc	boole-1	intern	rotatef
syn keyword lispFunc	boole-2	internal-time-units-per-second	round
syn keyword lispFunc	boole-and	intersection	row-major-aref
syn keyword lispFunc	boole-andc1	invalid-method-error	rplaca
syn keyword lispFunc	boole-andc2	invoke-debugger	rplacd
syn keyword lispFunc	boole-c1	invoke-restart	safety
syn keyword lispFunc	boole-c2	invoke-restart-interactively	satisfies
syn keyword lispFunc	boole-clr	isqrt	sbit
syn keyword lispFunc	boole-eqv	keyword	scale-float
syn keyword lispFunc	boole-ior	keywordp	schar
syn keyword lispFunc	boole-nand	labels	search
syn keyword lispFunc	boole-nor	lambda	second
syn keyword lispFunc	boole-orc1	lambda-list-keywords	sequence
syn keyword lispFunc	boole-orc2	lambda-parameters-limit	serious-condition
syn keyword lispFunc	boole-set	last	set
syn keyword lispFunc	boole-xor	lcm	set-char-bit
syn keyword lispFunc	boolean	ldb	set-difference
syn keyword lispFunc	both-case-p	ldb-test	set-dispatch-macro-character
syn keyword lispFunc	boundp	ldiff	set-exclusive-or
syn keyword lispFunc	break	least-negative-double-float	set-macro-character
syn keyword lispFunc	broadcast-stream	least-negative-long-float	set-pprint-dispatch
syn keyword lispFunc	broadcast-stream-streams	least-negative-normalized-double-float	set-syntax-from-char
syn keyword lispFunc	built-in-class	least-negative-normalized-long-float	setf
syn keyword lispFunc	butlast	least-negative-normalized-short-float	setq
syn keyword lispFunc	byte	least-negative-normalized-single-float	seventh
syn keyword lispFunc	byte-position	least-negative-short-float	shadow
syn keyword lispFunc	byte-size	least-negative-single-float	shadowing-import
syn keyword lispFunc	call-arguments-limit	least-positive-double-float	shared-initialize
syn keyword lispFunc	call-method	least-positive-long-float	shiftf
syn keyword lispFunc	call-next-method	least-positive-normalized-double-float	short-float
syn keyword lispFunc	capitalize	least-positive-normalized-long-float	short-float-epsilon
syn keyword lispFunc	car	least-positive-normalized-short-float	short-float-negative-epsilon
syn keyword lispFunc	case	least-positive-normalized-single-float	short-site-name
syn keyword lispFunc	catch	least-positive-short-float	signal
syn keyword lispFunc	ccase	least-positive-single-float	signed-byte
syn keyword lispFunc	cdr	length	signum
syn keyword lispFunc	ceiling	let	simle-condition
syn keyword lispFunc	cell-error	let*	simple-array
syn keyword lispFunc	cell-error-name	lisp	simple-base-string
syn keyword lispFunc	cerror	lisp-implementation-type	simple-bit-vector
syn keyword lispFunc	change-class	lisp-implementation-version	simple-bit-vector-p
syn keyword lispFunc	char	list	simple-condition-format-arguments
syn keyword lispFunc	char-bit	list*	simple-condition-format-control
syn keyword lispFunc	char-bits	list-all-packages	simple-error
syn keyword lispFunc	char-bits-limit	list-length	simple-string
syn keyword lispFunc	char-code	listen	simple-string-p
syn keyword lispFunc	char-code-limit	listp	simple-type-error
syn keyword lispFunc	char-control-bit	load	simple-vector
syn keyword lispFunc	char-downcase	load-logical-pathname-translations	simple-vector-p
syn keyword lispFunc	char-equal	load-time-value	simple-warning
syn keyword lispFunc	char-font	locally	sin
syn keyword lispFunc	char-font-limit	log	single-flaot-epsilon
syn keyword lispFunc	char-greaterp	logand	single-float
syn keyword lispFunc	char-hyper-bit	logandc1	single-float-epsilon
syn keyword lispFunc	char-int	logandc2	single-float-negative-epsilon
syn keyword lispFunc	char-lessp	logbitp	sinh
syn keyword lispFunc	char-meta-bit	logcount	sixth
syn keyword lispFunc	char-name	logeqv	sleep
syn keyword lispFunc	char-not-equal	logical-pathname	slot-boundp
syn keyword lispFunc	char-not-greaterp	logical-pathname-translations	slot-exists-p
syn keyword lispFunc	char-not-lessp	logior	slot-makunbound
syn keyword lispFunc	char-super-bit	lognand	slot-missing
syn keyword lispFunc	char-upcase	lognor	slot-unbound
syn keyword lispFunc	char/=	lognot	slot-value
syn keyword lispFunc	char<	logorc1	software-type
syn keyword lispFunc	char<=	logorc2	software-version
syn keyword lispFunc	char=	logtest	some
syn keyword lispFunc	char>	logxor	sort
syn keyword lispFunc	char>=	long-float	space
syn keyword lispFunc	character	long-float-epsilon	special
syn keyword lispFunc	characterp	long-float-negative-epsilon	special-form-p
syn keyword lispFunc	check-type	long-site-name	special-operator-p
syn keyword lispFunc	cis	loop	speed
syn keyword lispFunc	class	loop-finish	sqrt
syn keyword lispFunc	class-name	lower-case-p	stable-sort
syn keyword lispFunc	class-of	machine-instance	standard
syn keyword lispFunc	clear-input	machine-type	standard-char
syn keyword lispFunc	clear-output	machine-version	standard-char-p
syn keyword lispFunc	close	macro-function	standard-class
syn keyword lispFunc	clrhash	macroexpand	standard-generic-function
syn keyword lispFunc	code-char	macroexpand-1	standard-method
syn keyword lispFunc	coerce	macroexpand-l	standard-object
syn keyword lispFunc	commonp	macrolet	step
syn keyword lispFunc	compilation-speed	make-array	storage-condition
syn keyword lispFunc	compile	make-array	store-value
syn keyword lispFunc	compile-file	make-broadcast-stream	stream
syn keyword lispFunc	compile-file-pathname	make-char	stream-element-type
syn keyword lispFunc	compiled-function	make-concatenated-stream	stream-error
syn keyword lispFunc	compiled-function-p	make-condition	stream-error-stream
syn keyword lispFunc	compiler-let	make-dispatch-macro-character	stream-external-format
syn keyword lispFunc	compiler-macro	make-echo-stream	streamp
syn keyword lispFunc	compiler-macro-function	make-hash-table	streamup
syn keyword lispFunc	complement	make-instance	string
syn keyword lispFunc	complex	make-instances-obsolete	string-capitalize
syn keyword lispFunc	complexp	make-list	string-char
syn keyword lispFunc	compute-applicable-methods	make-load-form	string-char-p
syn keyword lispFunc	compute-restarts	make-load-form-saving-slots	string-downcase
syn keyword lispFunc	concatenate	make-method	string-equal
syn keyword lispFunc	concatenated-stream	make-package	string-greaterp
syn keyword lispFunc	concatenated-stream-streams	make-pathname	string-left-trim
syn keyword lispFunc	cond	make-random-state	string-lessp
syn keyword lispFunc	condition	make-sequence	string-not-equal
syn keyword lispFunc	conjugate	make-string	string-not-greaterp
syn keyword lispFunc	cons	make-string-input-stream	string-not-lessp
syn keyword lispFunc	consp	make-string-output-stream	string-right-strim
syn keyword lispFunc	constantly	make-symbol	string-right-trim
syn keyword lispFunc	constantp	make-synonym-stream	string-stream
syn keyword lispFunc	continue	make-two-way-stream	string-trim
syn keyword lispFunc	control-error	makunbound	string-upcase
syn keyword lispFunc	copy-alist	map	string/=
syn keyword lispFunc	copy-list	map-into	string<
syn keyword lispFunc	copy-pprint-dispatch	mapc	string<=
syn keyword lispFunc	copy-readtable	mapcan	string=
syn keyword lispFunc	copy-seq	mapcar	string>
syn keyword lispFunc	copy-structure	mapcon	string>=
syn keyword lispFunc	copy-symbol	maphash	stringp
syn keyword lispFunc	copy-tree	mapl	structure
syn keyword lispFunc	cos	maplist	structure-class
syn keyword lispFunc	cosh	mask-field	structure-object
syn keyword lispFunc	count	max	style-warning
syn keyword lispFunc	count-if	member	sublim
syn keyword lispFunc	count-if-not	member-if	sublis
syn keyword lispFunc	ctypecase	member-if-not	subseq
syn keyword lispFunc	debug	merge	subsetp
syn keyword lispFunc	decf	merge-pathname	subst
syn keyword lispFunc	declaim	merge-pathnames	subst-if
syn keyword lispFunc	declaration	method	subst-if-not
syn keyword lispFunc	declare	method-combination	substitute
syn keyword lispFunc	decode-float	method-combination-error	substitute-if
syn keyword lispFunc	decode-universal-time	method-qualifiers	substitute-if-not
syn keyword lispFunc	defclass	min	subtypep
syn keyword lispFunc	defconstant	minusp	svref
syn keyword lispFunc	defgeneric	mismatch	sxhash
syn keyword lispFunc	define-compiler-macro	mod	symbol
syn keyword lispFunc	define-condition	most-negative-double-float	symbol-function
syn keyword lispFunc	define-method-combination	most-negative-fixnum	symbol-macrolet
syn keyword lispFunc	define-modify-macro	most-negative-long-float	symbol-name
syn keyword lispFunc	define-setf-expander	most-negative-short-float	symbol-package
syn keyword lispFunc	define-setf-method	most-negative-single-float	symbol-plist
syn keyword lispFunc	define-symbol-macro	most-positive-double-float	symbol-value
syn keyword lispFunc	defmacro	most-positive-fixnum	symbolp
syn keyword lispFunc	defmethod	most-positive-long-float	synonym-stream
syn keyword lispFunc	defpackage	most-positive-short-float	synonym-stream-symbol
syn keyword lispFunc	defparameter	most-positive-single-float	sys
syn keyword lispFunc	defsetf	muffle-warning	system
syn keyword lispFunc	defstruct	multiple-value-bind	t
syn keyword lispFunc	deftype	multiple-value-call	tagbody
syn keyword lispFunc	defun	multiple-value-list	tailp
syn keyword lispFunc	defvar	multiple-value-prog1	tan
syn keyword lispFunc	delete	multiple-value-seteq	tanh
syn keyword lispFunc	delete-duplicates	multiple-value-setq	tenth
syn keyword lispFunc	delete-file	multiple-values-limit	terpri
syn keyword lispFunc	delete-if	name-char	the
syn keyword lispFunc	delete-if-not	namestring	third
syn keyword lispFunc	delete-package	nbutlast	throw
syn keyword lispFunc	denominator	nconc	time
syn keyword lispFunc	deposit-field	next-method-p	trace
syn keyword lispFunc	describe	nil	translate-logical-pathname
syn keyword lispFunc	describe-object	nintersection	translate-pathname
syn keyword lispFunc	destructuring-bind	ninth	tree-equal
syn keyword lispFunc	digit-char	no-applicable-method	truename
syn keyword lispFunc	digit-char-p	no-next-method	truncase
syn keyword lispFunc	directory	not	truncate
syn keyword lispFunc	directory-namestring	notany	two-way-stream
syn keyword lispFunc	disassemble	notevery	two-way-stream-input-stream
syn keyword lispFunc	division-by-zero	notinline	two-way-stream-output-stream
syn keyword lispFunc	do	nreconc	type
syn keyword lispFunc	do*	nreverse	type-error
syn keyword lispFunc	do-all-symbols	nset-difference	type-error-datum
syn keyword lispFunc	do-exeternal-symbols	nset-exclusive-or	type-error-expected-type
syn keyword lispFunc	do-external-symbols	nstring	type-of
syn keyword lispFunc	do-symbols	nstring-capitalize	typecase
syn keyword lispFunc	documentation	nstring-downcase	typep
syn keyword lispFunc	dolist	nstring-upcase	unbound-slot
syn keyword lispFunc	dotimes	nsublis	unbound-slot-instance
syn keyword lispFunc	double-float	nsubst	unbound-variable
syn keyword lispFunc	double-float-epsilon	nsubst-if	undefined-function
syn keyword lispFunc	double-float-negative-epsilon	nsubst-if-not	unexport
syn keyword lispFunc	dpb	nsubstitute	unintern
syn keyword lispFunc	dribble	nsubstitute-if	union
syn keyword lispFunc	dynamic-extent	nsubstitute-if-not	unless
syn keyword lispFunc	ecase	nth	unread
syn keyword lispFunc	echo-stream	nth-value	unread-char
syn keyword lispFunc	echo-stream-input-stream	nthcdr	unsigned-byte
syn keyword lispFunc	echo-stream-output-stream	null	untrace
syn keyword lispFunc	ed	number	unuse-package
syn keyword lispFunc	eighth	numberp	unwind-protect
syn keyword lispFunc	elt	numerator	update-instance-for-different-class
syn keyword lispFunc	encode-universal-time	nunion	update-instance-for-redefined-class
syn keyword lispFunc	end-of-file	oddp	upgraded-array-element-type
syn keyword lispFunc	endp	open	upgraded-complex-part-type
syn keyword lispFunc	enough-namestring	open-stream-p	upper-case-p
syn keyword lispFunc	ensure-directories-exist	optimize	use-package
syn keyword lispFunc	ensure-generic-function	or	use-value
syn keyword lispFunc	eq	otherwise	user
syn keyword lispFunc	eql	output-stream-p	user-homedir-pathname
syn keyword lispFunc	equal	package	values
syn keyword lispFunc	equalp	package-error	values-list
syn keyword lispFunc	error	package-error-package	vector
syn keyword lispFunc	etypecase	package-name	vector-pop
syn keyword lispFunc	eval	package-nicknames	vector-push
syn keyword lispFunc	eval-when	package-shadowing-symbols	vector-push-extend
syn keyword lispFunc	evalhook	package-use-list	vectorp
syn keyword lispFunc	evenp	package-used-by-list	warn
syn keyword lispFunc	every	packagep	warning
syn keyword lispFunc	exp	pairlis	when
syn keyword lispFunc	export	parse-error	wild-pathname-p
syn keyword lispFunc	expt	parse-integer	with-accessors
syn keyword lispFunc	extended-char	parse-namestring	with-compilation-unit
syn keyword lispFunc	fboundp	pathname	with-condition-restarts
syn keyword lispFunc	fceiling	pathname-device	with-hash-table-iterator
syn keyword lispFunc	fdefinition	pathname-directory	with-input-from-string
syn keyword lispFunc	ffloor	pathname-host	with-open-file
syn keyword lispFunc	fifth	pathname-match-p	with-open-stream
syn keyword lispFunc	file-author	pathname-name	with-output-to-string
syn keyword lispFunc	file-error	pathname-type	with-package-iterator
syn keyword lispFunc	file-error-pathname	pathname-version	with-simple-restart
syn keyword lispFunc	file-length	pathnamep	with-slots
syn keyword lispFunc	file-namestring	peek-char	with-standard-io-syntax
syn keyword lispFunc	file-position	phase	write
syn keyword lispFunc	file-stream	pi	write-byte
syn keyword lispFunc	file-string-length	plusp	write-char
syn keyword lispFunc	file-write-date	pop	write-line
syn keyword lispFunc	fill	position	write-sequence
syn keyword lispFunc	fill-pointer	position-if	write-string
syn keyword lispFunc	find	position-if-not	write-to-string
syn keyword lispFunc	find-all-symbols	pprint	y-or-n-p
syn keyword lispFunc	find-class	pprint-dispatch	yes-or-no-p
syn keyword lispFunc	find-if	pprint-exit-if-list-exhausted	zerop
syn keyword lispFunc	find-if-not	pprint-fill

syn match   lispFunc	"\<c[ad]\+r\>"


" Lisp Keywords (modifiers)
syn keyword lispKey	:abort	:from-end	:overwrite
syn keyword lispKey	:adjustable	:gensym	:predicate
syn keyword lispKey	:append	:host	:preserve-whitespace
syn keyword lispKey	:array	:if-does-not-exist	:pretty
syn keyword lispKey	:base	:if-exists	:print
syn keyword lispKey	:case	:include	:print-function
syn keyword lispKey	:circle	:index	:probe
syn keyword lispKey	:conc-name	:inherited	:radix
syn keyword lispKey	:constructor	:initial-contents	:read-only
syn keyword lispKey	:copier	:initial-element	:rehash-size
syn keyword lispKey	:count	:initial-offset	:rehash-threshold
syn keyword lispKey	:create	:initial-value	:rename
syn keyword lispKey	:default	:input	:rename-and-delete
syn keyword lispKey	:defaults	:internal	:size
syn keyword lispKey	:device	:io	:start
syn keyword lispKey	:direction	:junk-allowed	:start1
syn keyword lispKey	:directory	:key	:start2
syn keyword lispKey	:displaced-index-offset	:length	:stream
syn keyword lispKey	:displaced-to	:level	:supersede
syn keyword lispKey	:element-type	:name	:test
syn keyword lispKey	:end	:named	:test-not
syn keyword lispKey	:end1	:new-version	:type
syn keyword lispKey	:end2	:nicknames	:use
syn keyword lispKey	:error	:output	:verbose
syn keyword lispKey	:escape	:output-file	:version
syn keyword lispKey	:external

" Standard Lisp Variables
syn keyword lispVar	*applyhook*	*load-pathname*	*print-pprint-dispatch*
syn keyword lispVar	*break-on-signals*	*load-print*	*print-pprint-dispatch*
syn keyword lispVar	*break-on-signals*	*load-truename*	*print-pretty*
syn keyword lispVar	*break-on-warnings*	*load-verbose*	*print-radix*
syn keyword lispVar	*compile-file-pathname*	*macroexpand-hook*	*print-readably*
syn keyword lispVar	*compile-file-pathname*	*modules*	*print-right-margin*
syn keyword lispVar	*compile-file-truename*	*package*	*print-right-margin*
syn keyword lispVar	*compile-file-truename*	*print-array*	*query-io*
syn keyword lispVar	*compile-print*	*print-base*	*random-state*
syn keyword lispVar	*compile-verbose*	*print-case*	*read-base*
syn keyword lispVar	*compile-verbose*	*print-circle*	*read-default-float-format*
syn keyword lispVar	*debug-io*	*print-escape*	*read-eval*
syn keyword lispVar	*debugger-hook*	*print-gensym*	*read-suppress*
syn keyword lispVar	*default-pathname-defaults*	*print-length*	*readtable*
syn keyword lispVar	*error-output*	*print-level*	*standard-input*
syn keyword lispVar	*evalhook*	*print-lines*	*standard-output*
syn keyword lispVar	*features*	*print-miser-width*	*terminal-io*
syn keyword lispVar	*gensym-counter*	*print-miser-width*	*trace-output*

" Strings
syn region	lispString	start=+"+ skip=+\\\\\|\\"+ end=+"+
if exists("lisp_instring")
 syn region	lispInString	keepend matchgroup=Delimiter start=+"(+rs=s+1 skip=+|.\{-}|+ matchgroup=Delimiter end=+)"+ contains=@lispListCluster,lispInStringString
 syn region	lispInStringString	start=+\\"+ skip=+\\\\+ end=+\\"+ contained
endif

" Shared with Xlisp, Declarations, Macros, Functions
syn keyword lispDecl	defmacro	do-all-symbols	labels
syn keyword lispDecl	defsetf	do-external-symbols	let
syn keyword lispDecl	deftype	do-symbols	locally
syn keyword lispDecl	defun	dotimes	macrolet
syn keyword lispDecl	do*	flet	multiple-value-bind

" Numbers: supporting integers and floating point numbers
syn match lispNumber	"-\=\(\.\d\+\|\d\+\(\.\d*\)\=\)\(e[-+]\=\d\+\)\="

syn match lispSpecial	"\*[a-zA-Z_][a-zA-Z_0-9-]*\*"
syn match lispSpecial	!#|[^()'`,"; \t]\+|#!
syn match lispSpecial	!#x[0-9a-fA-F]\+!
syn match lispSpecial	!#o[0-7]\+!
syn match lispSpecial	!#b[01]\+!
syn match lispSpecial	!#\\[ -\~]!
syn match lispSpecial	!#[':][^()'`,"; \t]\+!
syn match lispSpecial	!#([^()'`,"; \t]\+)!

syn match lispConcat	"\s\.\s"
syn match lispParenError	")"

" Comments
syn cluster lispCommentGroup	contains=lispTodo
syn match   lispComment	";.*$"	contains=@lispCommentGroup
syn case ignore
syn keyword lispTodo	contained	combak	combak:	todo	todo:
syn case match

" synchronization
syn sync lines=100

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_lisp_syntax_inits")
  if version < 508
    let did_lisp_syntax_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink lispAtomNmbr	lispNumber
  HiLink lispAtomMark	lispMark
  HiLink lispInStringString	lispString

  HiLink lispAtom	Identifier
  HiLink lispAtomBarSymbol	Special
  HiLink lispBarSymbol	Special
  HiLink lispComment	Comment
  HiLink lispConcat	Statement
  HiLink lispDecl	Statement
  HiLink lispFunc	Statement
  HiLink lispKey	Type
  HiLink lispMark	Delimiter
  HiLink lispNumber	Number
  HiLink lispParenError	Error
  HiLink lispSpecial	Type
  HiLink lispString	String
  HiLink lispTodo	Todo
  HiLink lispVar	Statement

  delcommand HiLink
endif

let b:current_syntax = "lisp"

" vim: ts=33 nowrap
