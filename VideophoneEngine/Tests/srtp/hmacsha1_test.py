#!/usr/bin/env python

def hexToBytes(hex_str):
    """ Convert to bytes
    TODO: better way to do this in python?
    """

    num_bytes = len(hex_str) / 2

    ret = ""

    for i in range(0, num_bytes):
        byte = hex_str[i * 2:i * 2 + 2]
        #print byte
        ret += chr(int(byte, 16))

    #print ret

    return ret

def bytesToHex(byte_str):
    """ Convert to hex
    TODO: better way to do this in python?
    """

    ret = ""

    for c in byte_str:
        # NOTE: didn't use "hex()" since it puts a 0x prefix
        ret += format(ord(c), 'x')

    return ret

def roundTripTest(hex_str):
    byte_str = hexToBytes(hex_str)
    hex_str2 = bytesToHex(byte_str)

    if hex_str == hex_str2:
        print("Yay, they are the same")
    else:
        print("Nada...")

def sign_request(key, input_raw):
    """ Works! """
    from hashlib import sha1
    import hmac

    hashed = hmac.new(key, input_raw, sha1)

    # The signature
    return hashed.digest()

def testHmacSha1(key, input_str, expected_hex):
    tag = sign_request(key, input_str)
    #print len(tag)
    ret = expected_hex == bytesToHex(tag)

    if ret:
        print "Pass!"
    else:
        print "Fail..."

# Pass!
testHmacSha1("Jefe", "what do ya want for nothing?", "effcdf6ae5eb2fa2d27416d5f184df9c259a7c79")
