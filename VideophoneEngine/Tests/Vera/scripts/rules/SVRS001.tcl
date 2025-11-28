#!/usr/bin/tclsh
# Don't use space characters to indent

foreach f [getSourceFileNames] {
    set lineNumber 1
    foreach line [getAllLines $f] {

		# A space followed by zero or more tabs and spaces followed by one or more non-whitepsace characters
		# Exception: a '*' following a single space which is a pattern found with C style comments.
		#
        if { [regexp {^[ ][ \t]*[^ \t]+} $line] && ![regexp {^[ ]\*} $line] } {
            report $f $lineNumber "space used to indent"
        }

        incr lineNumber
    }
}
