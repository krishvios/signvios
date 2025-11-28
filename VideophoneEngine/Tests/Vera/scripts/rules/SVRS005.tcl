#!/usr/bin/tclsh
# Checks for proper spacing around operators

set spaceOperators {
	andand
	andassign
	assign
	divide
	equal
	divideassign
	lessequal
	minusassign
	notequal
	or
	orassign
	oror
	percent
	percentassign
	plus
	plusassign
	question_mark
	shiftleft
	shiftleftassign
	shiftright
	shiftrightassign
	starassign
	xor
	xorassign
}

set nospaceOperators {
	arrow
	arrowstar
	dot
	dotstar
}

set otherOperators {
	star
	and
	minus
	less
	greater
}

set state "other"
set tokenValue1 ""
set tokenName1 ""
set tokenValue2 ""
set tokenName2 ""
set tokenValue3 ""
set tokenName3 ""

set mode "None"

foreach f [getSourceFileNames] {
    foreach t [getTokens $f 1 0 -1 -1 {}] {
        set tokenValue1 [lindex $t 0]
        set tokenName1 [lindex $t 3]
        set lineNumber [lindex $t 1]

		if {$tokenName1 == "operator"} {
			set mode "ParsingOperator"
		} else {
			
    		set operator [lsearch $spaceOperators $tokenName2]
			set nospaceOperator [lsearch $nospaceOperators $tokenName2]
				        
			if {$mode == "ParsingOperator"} {
				#
				# We are currently parsing an operator method.  Treat it specially.
				# Allow for no space preceding the operator but there must be a single space after the operator
				#
				set otherOperator [lsearch $otherOperators $tokenName2]
			
				if {$operator != -1 || $nospaceOperator != -1 || $otherOperator != -1 } {
					set mode "None"
					
					if {$tokenName1 != "space"} {
						report $f $lineNumber "no whitepsace after operator: \'$tokenValue2\'"
					}
				}
			} else {
        		if {$operator != -1} {
					
					if {($tokenName3 != "space" || ($tokenName1 != "space" && $tokenName1 != "newline")) } {
    					report $f $lineNumber "no whitepsace around operator: \'$tokenValue2\'"
					}
        		} else {
					if {$nospaceOperator != -1} {
    					if {$tokenName3 == "space" || $tokenName1 == "space" || $tokenName1 == "newline"} {
    						report $f $lineNumber "no whitepsace allowed around operator: \'$tokenValue2\'"
    					}
					}
        		}
			}
		}

		set tokenValue3 $tokenValue2
		set tokenName3 $tokenName2

		set tokenValue2 $tokenValue1
		set tokenName2 $tokenName1
    }
}
