print "helloish"
import sys
print sys.path
print 5*6
def fact(i):
    if (i==0):
        return 1;
    return i*fact(i-1);
print fact(9)