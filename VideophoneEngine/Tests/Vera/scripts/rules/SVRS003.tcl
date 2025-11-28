#!/usr/bin/tclsh
# stiMAKE_ERROR is not used directly

set state "other"
set tokenValue1 ""
set tokenName1 ""
set tokenValue2 ""
set tokenName2 ""
set tokenValue3 ""
set tokenName3 ""

foreach f [getSourceFileNames] {
    foreach t [getTokens $f 1 0 -1 -1 {}] {
        set tokenValue1 [lindex $t 0]
        set tokenName1 [lindex $t 3]
        set lineNumber [lindex $t 1]

		if {$tokenValue1 == "stiMAKE_ERROR" && ($tokenName2 == "assign" || ($tokenName3 == "assign" && $tokenName2 == "space")) } {
			report $f $lineNumber "Direct use of stiMAKE_ERROR"
		} else {
			if {$tokenValue1 != "stiRESULT_SUCCESS" && $tokenValue1 != "stiRESULT_CONVERT" && $tokenValue1 != "stiRESULT_CODE" && [regexp {^stiRESULT_.*} $tokenValue1] && ($tokenName2 == "assign" || ($tokenName3 == "assign" && $tokenName2 == "space")) } {
				report $f $lineNumber "Direct use of ${tokenValue1}"
			}
		}

		set tokenValue3 $tokenValue2
		set tokenName3 $tokenName2

		set tokenValue2 $tokenValue1
		set tokenName2 $tokenName1
    }
}
