import pystarcode

# parse file
seq_list = []
with open('iPCR_rep1_filtered.txt','r') as f :
    for line in f :
        seq_list.append(line.strip('\n'))

# invoke starcode
clusteralg = "components"
canonical_counts, d = pystarcode.starcode(seq_list,2,clusteralg=clusteralg)

# print counts output
# for key, l in canonical_counts.iteritems() :
    # print '%s -> %d'%(key,l)

# print output
# for key, l in d.iteritems() :
    # print '%s -> %s'%(key,l)

with open('test-%s.txt.true'%(clusteralg), 'r') as f :
    for line in f :
        canonical, counts = line.split('\t')
        counts = int(counts)
        if canonical_counts[canonical] != counts :
            print "Discrepancy between counts of %s"%(canonical)
            break
