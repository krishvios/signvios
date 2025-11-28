#!/usr/bin/tclsh
# Scope operater does not have spaces after.

set state "other"
foreach f [getSourceFileNames] {
    foreach t [getTokens $f 1 0 -1 -1 {}] {
        set tokenValue [lindex $t 0]
        set tokenName [lindex $t 3]
        if {$state == "scope"} {
            if {$tokenName == "space"} {
                report $f $lineNumber "whitepace after scope operator"
            }
            set state "other"
        } else {
            if {$tokenName == "colon_colon"} {
                set state "scope"
                set lineNumber [lindex $t 1]
                set keywordValue [lindex $t 0]
            }
        }
    }
}
