
.section ".data.header"

.extern _start
.extern _end
.extern _end_header
.extern __size__
	
.word	0xa1aced00
.word	_start
.word	__size__ /* _end - _end_header   filled at link time */
.word	0x00000030
.word	_end_header
.word	_end_header
