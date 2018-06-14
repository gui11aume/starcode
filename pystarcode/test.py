import pystarcode

# parse file
seq_list = []
with open('small.txt','r') as f :
    for line in f :
        seq_list.append(line.strip('\n'))

# invoke starcode
counts, d = pystarcode.starcode(seq_list,2,5)

# print counts output
for key, l in counts.iteritems() :
    print '%s -> %d'%(key,l)

# print output
for key, l in d.iteritems() :
    print '%s -> %s'%(key,l)
