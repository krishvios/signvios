#!/usr/bin/tclsh
# Checks for lack of statement after a delete that sets the pointer to NULL


set tokenValue ""
set tokenName1 ""
set state "None"
set pointer ""
set pointer2 ""
set errormsg "statement to reassign pointer does not follow delete"

#states: None -> pointer1 -> pointer2 -> assignment

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
    					
    					"delete" {
    						set state "pointer1"
    					}
    				}
    			}
    
    			"pointer1" {
    				switch $tokenName {
    					
    					"semicolon" {
            				if {$pointer == ""} {
            					#
            					# Found semicolon before seeing an identifier?
            					# I don't think we should get here.
            					#
            					set state "None"
            				} else {
            					set state "pointer2"
            				}
    					}
    					
						"rightbracket" {
							if {$pointer != ""} {
								# must the bracket for an array delete
								append pointer $tokenValue
							}
						}

						"leftbracket" {
							if {$pointer != ""} {
								# must the bracket for an array delete
								append pointer $tokenValue
							}
						}
						
    					default {
							
							append pointer $tokenValue
    						#set pointer $tokenValue
							#report $f $lineNumber "found identifier: ${pointer}"
    					}
        			}
    			}
    			
        		"pointer2" {
    				
    				switch $tokenName {
    					
        				"assign" {
							if {$pointer != $pointer2} {
	        					report $f $lineNumber $errormsg
							}

							set state "None"
							set pointer ""
							set pointer2 ""
           				}

						"comma" {
        					report $f $lineNumber $errormsg
							set state "None"
							set pointer ""
							set pointer2 ""
						}

						"rightbrace" {
        					report $f $lineNumber $errormsg
							set state "None"
							set pointer ""
							set pointer2 ""
						}

						"semicolon" {
        					report $f $lineNumber $errormsg
							set state "None"
							set pointer ""
							set pointer2 ""
						}

    					default {
							append pointer2 $tokenValue
    					}
    				}
        		}
    		}
		}
    }
}
