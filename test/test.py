import pystarcode

# parse file
seq_list = []
with open('example_seqs.txt','r') as f :
    for line in f :
        seq_list.append(line.strip('\n'))

# invoke starcode
clusteralg = "mp"
canonical_counts, d = pystarcode.starcode(seq_list,2,clusteralg=clusteralg)

# print counts output
for key, l in canonical_counts.iteritems() :
    print '%s -> %d'%(key,l)

# print output
for key, l in d.iteritems() :
    print '%s -> %s'%(key,l)
