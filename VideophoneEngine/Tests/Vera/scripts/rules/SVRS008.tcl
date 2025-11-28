#!/usr/bin/tclsh
# Identifies incorrectly named classes, structs and enums.


set tokenValue ""
set tokenName1 ""
set currentObject {"" "" ""}		;# type name state
set objectList "$currentObject"
set scope 0

foreach f [getSourceFileNames] {
    foreach t [getTokens $f 1 0 -1 -1 {}] {
        set tokenValue [lindex $t 0]
        set lineNumber [lindex $t 1]
		set tokenName [lindex $t 3]

		switch $tokenName {
			
			"leftbrace" {
				
				switch [lindex $currentObject 2] {
					"start" {
						
						set objectType [lindex $currentObject 0]
						set objectName [lindex $currentObject 1]
						set currentObject [list "$objectType" "$objectName" "body"]
						lset objectList $scope $currentObject
						
						#report $f $lineNumber "start of body: ${currentObject}"
						
						switch $objectType {
							
							"enum" {
    							if {$objectName != "" && ![regexp {^(Esti[A-Z].+|E[A-Z].+)} $objectName]} {
    								report $f $lineNumber "found enum entry not following naming convention: \'${objectName}\'"
    							}
    						}
							
							"struct" {
    							if {$objectName != "" && ![regexp {^(Ssti[A-Z].+|S[A-Z].+)} $objectName] } {
    								report $f $lineNumber "found struct not following naming convention: \'${objectName}\'"
    							}
							}
							
							"class" {
    							if {$objectName != "" && ![regexp {^(Csti[A-Z].+|C[A-Z].+|Isti[A-Z].+)} $objectName] } {
    								report $f $lineNumber "found class not following naming convention: \'${objectName}\'"
    							}
							}
						}
					}
					
					"body" {
						#
						# start of unnamed block
						#
						#report $f $lineNumber "start of anonymous block"
						set scope [expr {$scope + 1}]
						set currentObject [list "anonymous" "" "body"]
						lset objectList $scope $currentObject
					}
					
					"end" {
						#
						# This could be an initialization list
						#
    					#report $f $lineNumber "found starting anonymous block at end of object"
    					set scope [expr {$scope + 1}]
    					set currentObject [list "anonymous" "" "body"]
    					lset objectList $scope $currentObject
    					#report $f $lineNumber "no object state: ${currentObject}"
					}
					
					"" {
						#
						# functions, if, while, for, do, etc
						#
#						#report $f $lineNumber "error: left brace in global space?"
						#report $f $lineNumber "found starting anonymous block"
						set scope [expr {$scope + 1}]
						set currentObject [list "anonymous" "" "body"]
						lset objectList $scope $currentObject
						#report $f $lineNumber "no object state: ${currentObject}"
					}
				}
				
				#report $f $lineNumber "leftbrace: ${currentObject}"
				#set scope [expr {$scope + 1}]
				#report $f $lineNumber "Scope: ${scope}"
			}
			
			"rightbrace" {
								
				#report $f $lineNumber "rightbrace: ${currentObject}"
				
				if {[lindex $currentObject 2] == "body"} {
					
					set objectType [lindex $currentObject 0]
															
					if {$objectType == "anonymous"} {
						set scope [expr {$scope - 1}]
						set currentObject [lindex $objectList $scope]
					} else {
						set objectName [lindex $currentObject 1]
						set currentObject [list $objectType $objectName "end"]
						lset objectList $scope $currentObject
					}
					
					#report $f $lineNumber "body end: ${currentObject}"
										
				} else {
					report $f $lineNumber "unbalanced braces "
				}
			}
			
			"semicolon" {
				#set currentObject [lindex $objectList $scope]
				
				if {[lindex $currentObject 2] == "start" || [lindex $currentObject 2] == "end"} {
					
					set scope [expr {$scope - 1}]
					set currentObject [lindex $objectList $scope]
					#report $f $lineNumber "semicolon: ${currentObject}"
				}
			}
			
			"rightparen" {
    			if {[lindex $currentObject 2] == "start" || [lindex $currentObject 2] == "end"} {
    				
    				set scope [expr {$scope - 1}]
    				set currentObject [lindex $objectList $scope]
    				#report $f $lineNumber "semicolon: ${currentObject}"
    			}
			}
				
			"comma" {
				if {[lindex $currentObject 2] == "start" || [lindex $currentObject 2] == "end"} {
					
					set scope [expr {$scope - 1}]
					set currentObject [lindex $objectList $scope]
					#report $f $lineNumber "semicolon: ${currentObject}"
				}
			}
			
			"return" {
#    			if {$scope != 1} {
 #   				report $f $lineNumber "Return not at end of function"
  #  			}
			}
			
			"identifier" {
				if {[lindex $currentObject 0] != "anonymous" && [lindex $currentObject 1] == "" && [lindex $currentObject 2] == "start"} {
    				set objectType [lindex $currentObject 0]
    				set objectState [lindex $currentObject 2]
    				set currentObject [list $objectType $tokenValue $objectState]
    				lset objectList $scope $currentObject
					#report $f $lineNumber "Found identifier \'${tokenValue}\': ${currentObject}"
				}
			}
			
			"class" {
				set scope [expr {$scope + 1}]
				set currentObject [list "class" "" "start"]
				lset objectList $scope $currentObject
				#report $f $lineNumber "class: ${currentObject}"
			}
			
    		"struct" {
				set scope [expr {$scope + 1}]
				set currentObject [list "struct" "" "start"]
				lset objectList $scope $currentObject
				#report $f $lineNumber "struct: ${currentObject}"
			}

			"enum" {
				set scope [expr {$scope + 1}]
				set currentObject [list "enum" "" "start"]
				lset objectList $scope $currentObject
				#report $f $lineNumber "enum: ${currentObject}"
			}			
		}
    }
}
