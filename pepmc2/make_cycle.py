import sys

def main(cycle_len):
    print 'VARS:X,' + ','.join('Y%d'%i for i in xrange(cycle_len))
    print 'PARAMS:p,0.1,1.1'
    
    print 'EQ:X = p*rp(Y0,4.9,5.1) + -0.1*X'
    for i in xrange(cycle_len-1):
        print 'EQ:dY%d = rp(Y%d,4.9,5.1) + -0.1*Y%d' % (i, i+1, i)

    print 'EQ:dY%d = rp(X,4.9,5.1) + -0.1*Y%d' % (cycle_len - 1, cycle_len - 1)

    print 'THRES:X:0,10'
    for i in xrange(cycle_len):
        print 'THRES:Y%d:0,10' % i
    
    print 'INIT:' + ';'.join(['0,10']*(cycle_len+1))

    print "BA::X<4.9,1"
    print "BA::,1;!Y0<4.9,2"
    print "BA:F:,1;!Y0<4.9,2"

if len(sys.argv) < 2:
    print >>sys.stderr, 'Usage: %s <cycle-len>' % sys.argv[0]
else:
    main(int(sys.argv[1]))
