#!/usr/bin/tclsh
# Checks for correct hungarian notation


set tokenValue ""
set tokenName1 ""
set state "None"
set pointer ""
set errormsg "statement to reassign pointer does not follow delete"
set variableName ""
set variableType ""
set pointerLevel 0
set unsigned 0

#states: None -> pointer1 -> pointer2 -> assignment

proc InspectVariableName {f lineNumber variableType pointerLevel variableName} {

	if {$pointerLevel > 0} {
		switch $variableType {
			"int" {
				if {![regexp {^(m_)?pn[A-Z].*} $variableName]} {
					report $f $lineNumber "Possible hungarian notation violation: int *${variableName}"
				}
			}

			"unsigned int" {
				if {![regexp {^(m_)?pun[A-Z].*} $variableName]} {
					report $f $lineNumber "Possible hungarian notation violation: int *${variableName}"
				}
			}
		}

	} else {
		switch $variableType {
			"int" {
				if {$variableName != "i" && ![regexp {^(m_)?n[A-Z].*} $variableName]} {
					report $f $lineNumber "Possible hungarian notation violation: int ${variableName}"
				}
			}

			"unsigned int" {
				if {$variableName != "i" && ![regexp {^(m_)?un[A-Z].*} $variableName]} {
					report $f $lineNumber "Possible hungarian notation violation: int ${variableName}"
				}
			}
		}
	}
}


foreach f [getSourceFileNames] {
    foreach t [getTokens $f 1 0 -1 -1 {}] {
        set tokenValue [lindex $t 0]
        set lineNumber [lindex $t 1]
		set tokenName [lindex $t 3]

		#
		# Process all tokens but whitespace
		#
		if {$tokenName != "space" && $tokenName != "newline" && $tokenName != "cppcomment" && $tokenName != "ccomment"} {

    		switch $state {
    			
    			"None" {
    				switch $tokenName {
    					
						"unsigned" {
							set unsigned 1
						}

    					"int" {
    						set state "var"
							if {$unsigned == 1} {
								set variableType "unsigned int"
							} else {
								set variableType "int"
							}
    					}

						"identifier" {
							# if "unsigned" is set but there is not variableType set then assume the type is int.
							if {$unsigned == 1 && $variableType == ""} {
								set variableType "unsigned int"
								set state "termination"
								set variableName $tokenValue
							}
						}
    				}
    			}

				"var" {
					switch $tokenName {

						"identifier" {
							set state "termination"
							set variableName $tokenValue
						}

						"star" {
							set pointerLevel [expr {$pointerLevel + 1}]
						}

						default {
							set state "None"
							set pointerLevel 0
							set unsigned 0
							set variableType ""
						}
					}
				}

				"termination" {

					switch $tokenName {
						
						"comma" {

							InspectVariableName $f $lineNumber $variableType $pointerLevel $variableName

							set state "None"
							set pointerLevel 0
							set unsigned 0
							set variableType ""
						}

						"semicolon" {
							InspectVariableName $f $lineNumber $variableType $pointerLevel $variableName

							set state "None"
							set pointerLevel 0
							set unsigned 0
							set variableType ""
						}

						"equal" {
							InspectVariableName $f $lineNumber $variableType $pointerLevel $variableName

							set state "None"
							set pointerLevel 0
							set unsigned 0
							set variableType ""
						}

						default {
							set state "None"
							set pointerLevel 0
							set unsigned 0
							set variableType ""
						}
					}
				}
    		}
		}
    }
}

