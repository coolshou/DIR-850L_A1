# written by bouble_hung

rtcp

# Base on RFC3550, for RTCP, it's version field(first 2 bits) shall be 2.
# It's padding fileld maybe 0 or 1
# then, it's RC field(5 bits) maybe 0-31
# Then, follow by  byte of payload type from 200-204
^[\x80-\xbf][\xc8-\xcc]..*


