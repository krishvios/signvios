#!/usr/bin/tclsh
# Checks to see if STI_BAIL is positioned at the beginning of a line


set tokenValue ""
set tokenName1 ""

foreach f [getSourceFileNames] {
    foreach t [getTokens $f 1 0 -1 -1 {}] {
        set tokenValue [lindex $t 0]
        set lineNumber [lindex $t 1]
		set columnNumber [lindex $t 2]
		set tokenName [lindex $t 3]

		if {$tokenValue == "STI_BAIL" && $columnNumber != 0} {
			report $f $lineNumber "STI_BAIL not at the beginning of the line"
		}
    }
}
