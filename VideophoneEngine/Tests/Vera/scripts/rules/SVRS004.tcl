#!/usr/bin/tclsh
# Verifying proper naming of enums

set state "other"
set tokenValue1 ""
set tokenName1 ""
set tokenValue2 ""
set tokenName2 ""
set tokenValue3 ""
set tokenName3 ""

set state "none"
set type "anonymous"

foreach f [getSourceFileNames] {
    foreach t [getTokens $f 1 0 -1 -1 {}] {
        set tokenValue1 [lindex $t 0]
        set tokenName1 [lindex $t 3]
        set lineNumber [lindex $t 1]

		if {$state == "none" } {
			if {$tokenName1 == "enum"} {
				set state "enum"
			}
		} else {
			
			if {$state == "enum"} {
				if {$tokenName1 == "identifier"} {
					if {$type == "anonymous"} {
						#
						# Should be the name of the enum unless this is an anonymous enum
						#
						if {![regexp {^E([A-Z].*|sti.+)} $tokenValue1] } {
							report $f $lineNumber "found enum not following naming convention: \'${tokenValue1}\'"
						}

						set type "named"
					}
				} else {
					if {$tokenName1 == "leftbrace"} {
						set state "InEnum"
					} else {
						if {$tokenName1 == "semicolon" || $tokenName1 == "comma" || $tokenName1 == "rightparen"} {
							# We reached the end of the enum definition or declaration.
							set state "none"
							set type "anonymous"
						}
					}
				}
			} else {
				if {$state == "InEnum"} {
					if {$tokenName1 == "identifier"} {

						#
						# Should be the name of the enum entry
						#
						if {![regexp {^(esti[A-Z].+|e[A-Z].+)} $tokenValue1] } {
							report $f $lineNumber "found enum entry not following naming convention: \'${tokenValue1}\'"
						}
					} else {
						if {$tokenName1 == "rightbrace"} {
							set state "enum"
						}
					}
				}
			}
		}
    }
}

