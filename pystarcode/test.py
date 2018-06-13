import pystarcode

seq_list = []
with open('small.txt','r') as f :
    for line in f :
        seq_list.append(line.strip('\n'))
d = pystarcode.starcode(seq_list,2,5)
for key, l in d.iteritems() :
    print key,l
