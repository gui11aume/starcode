int Tarjan_find
(
   int item,
   int rep[]
)
{
   int i;
   int temp;
   for (i = item; rep[i] > 0; i = rep[i]);
   while (rep[item] > 0) {
      temp = item;
      item = rep[item];
      rep[temp] = i;
   }
   return i;
}

void Tarjan_union
(
   int item1,
   int item2,
   int rep[]
)
{
   int i = find(item1, rep);
   int j = find(item2, rep);
   if (i != j) {
      if (rep[j] > rep[i]) {
         rep[i] += (rep[j] - 1);
         rep[j] = i;
      } else {
         rep[j] += (rep[i] - 1);
         rep[i] = j;
      }
   }
}
