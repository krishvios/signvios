#!/usr/bin/tclsh
# Finds uses of goto


set tokenValue ""
set tokenName1 ""

foreach f [getSourceFileNames] {
    foreach t [getTokens $f 1 0 -1 -1 {}] {
        set tokenValue [lindex $t 0]
        set lineNumber [lindex $t 1]
		set tokenName [lindex $t 3]

		if {$tokenName == "goto"} {
			report $f $lineNumber "goto used"
		}
    }
}
