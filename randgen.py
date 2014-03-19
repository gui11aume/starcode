import random

bases = "ACTGN"
brcdlen = 20
meantau = 3
numbrcd = 100000
maxmatch = 5000
nummatch = 1000
perror = meantau/float(brcdlen)


# Generate random barcodes
brcdlist = [(''.join([random.choice(bases) for b in range(brcdlen)])) for i in range(numbrcd)]

# Introduce some matches
for i in range(nummatch):
    j = random.randrange(numbrcd)
    # Mean tau is a Bernoulli distribution, where E[nerr] = brcdlen * Pr(err), hence Pr(err) = E[nerr] / brcdlen = meantau / brcdlen
    brcdlist.extend([(''.join( [ (brcdlist[j][b],random.choice(bases))[random.random() < perror] for b in range(brcdlen) ] ) ) for match in range(random.randrange(maxmatch)) ] )

for barcode in brcdlist:
    print barcode
