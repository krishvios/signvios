#!/usr/bin/env python

# Here are some sample crypto attributes from linphone
"""
a=crypto:1 AES_CM_128_HMAC_SHA1_80 inline:v4yQP2QdOxtMJru5c6oNBueCmS0ynYvn9lJrgRim
a=crypto:2 AES_CM_128_HMAC_SHA1_32 inline:NWBoGOD4KwCZ6MsvvONt9cmHid7CYmdTxYrWmCrI
a=crypto:3 AES_256_CM_HMAC_SHA1_80 inline:odW5eLlz6vaWC//mA4qT1BXnlUm4qRFFwOLVgMBm7BFfyNfEwa8ocYP7fbF3TA==
a=crypto:4 AES_256_CM_HMAC_SHA1_32 inline:MczXTGXfwK9cAuipjRW0utHoS0jGUf4zvlyw0RcjaKHr6868XeVG8zDKgTlUTA==
"""

import base64

encoded_128 = "v4yQP2QdOxtMJru5c6oNBueCmS0ynYvn9lJrgRim"

decoded_128 = base64.b64decode(encoded_128)

print "Encoded length: %d" % len(encoded_128)
print "Decoded length: %d" % len(decoded_128) # = 30
# 16 bytes aes key
# 14 bytes salt

encoded_256 = "odW5eLlz6vaWC//mA4qT1BXnlUm4qRFFwOLVgMBm7BFfyNfEwa8ocYP7fbF3TA=="
decoded_256 = base64.b64decode(encoded_256)

print "Encoded length: %d" % len(encoded_256)
print "Decoded length: %d" % len(decoded_256) # = 46
# 32 bytes aes key
# 14 bytes salt
