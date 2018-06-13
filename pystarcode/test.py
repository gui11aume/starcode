import pystarcode

seq_list = []
with open('iPCR_rep1_filtered.txt','r') as f :
    for line in f :
        seq_list.append(line.strip('\n'))
d = pystarcode.starcode(seq_list,2,5)
# print d
