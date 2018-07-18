#include "unittest.h"
#include "starcode.c"

static const char untranslate[7] = "NACGT N";

void
test_starcode_1
// Basic tests of useq.
(void)
{
   // Regular initialization without info.
   useq_t *u = new_useq(1, "some sequence", NULL);
   test_assert_critical(u != NULL);
   test_assert(u->count == 1);
   // Call to 'new_useq()' capitalizes the sequence.
   test_assert(strcmp(u->seq, "SOME SEQUENCE") == 0);
   test_assert(u->info == NULL);
   test_assert(u->matches == NULL);
   test_assert(u->canonical == NULL);
   destroy_useq(u);

   // Regular initialization with info.
   u = new_useq(1, "some sequence", "some info");
   test_assert_critical(u != NULL);
   test_assert(u->count == 1);
   // Call to 'new_useq()' capitalizes the sequence.
   test_assert(strcmp(u->seq, "SOME SEQUENCE") == 0);
   test_assert(strcmp(u->info, "some info") == 0);
   test_assert(u->matches == NULL);
   test_assert(u->canonical == NULL);
   destroy_useq(u);

   // Initialize with negative value and \0 string.
   u = new_useq(-1, "", NULL);
   test_assert_critical(u != NULL);
   test_assert(u->count == -1);
   test_assert(strcmp(u->seq, "") == 0);
   test_assert(u->info == NULL);
   test_assert(u->matches == NULL);
   test_assert(u->canonical == NULL);
   destroy_useq(u);

   // Initialize with NULL string.
   u = new_useq(0, NULL, NULL);
   test_assert(u == NULL);

}


void
test_starcode_2
(void)
// Test addmatch().
{

   useq_t *u1 = new_useq(12983, "string 1", NULL);
   test_assert_critical(u1 != NULL);
   test_assert(u1->matches == NULL);
   useq_t *u2 = new_useq(-20838, "string 2", NULL);
   test_assert_critical(u2 != NULL);
   test_assert(u2->matches == NULL);

   // Add match to u1.
   addmatch(u1, u2, 1, 2);

   test_assert(u2->matches == NULL);
   test_assert(u1->matches != NULL);

   // Check failure.
   test_assert(addmatch(u1, u2, 3, 2) == 1);

   // Add match again to u1.
   addmatch(u1, u2, 1, 2);

   test_assert(u2->matches == NULL);
   test_assert_critical(u1->matches != NULL);
   test_assert_critical(u1->matches[1] != NULL);
   test_assert(u1->matches[1]->nitems == 2);

   destroy_useq(u1);
   destroy_useq(u2);

}


void
test_starcode_3
(void)
// Test 'addmatch', 'transfer_counts_and_update_canonicals'.
{
   useq_t *u1 = new_useq(1, "B}d2)$ChPyDC=xZ D-C", NULL);
   useq_t *u2 = new_useq(2, "RCD67vQc80:~@FV`?o%D", NULL);

   // Add match to 'u1'.
   addmatch(u1, u2, 1, 1);

   test_assert(u1->count == 1);
   test_assert(u2->count == 2);

   // This should not transfer counts but update canonical.
   transfer_counts_and_update_canonicals(u2);

   test_assert(u1->count == 1);
   test_assert(u2->count == 2);
   test_assert(u1->canonical == NULL);
   test_assert(u2->canonical == u2);

   // This should transfer the counts from 'u1' to 'u2'.
   transfer_counts_and_update_canonicals(u1);

   test_assert(u1->count == 0);
   test_assert(u2->count == 3);
   test_assert(u1->canonical == u2);
   test_assert(u2->canonical == u2);

   destroy_useq(u1);
   destroy_useq(u2);

   useq_t *u3 = new_useq(1, "{Lu[T}FOCMs}L_zx", NULL);
   useq_t *u4 = new_useq(2, "|kAV|Ch|RZ]h~WjCoDpX", NULL);
   useq_t *u5 = new_useq(2, "}lzHolUky", NULL);
   useq_t *u6 = new_useq(3, "][asdf31!3R]", NULL);
   useq_t *u7 = new_useq(1, "kjkdfdHK!}33-34", NULL);

   // Add matches to 'u3'.
   addmatch(u3, u4, 1, 1);
   addmatch(u3, u5, 1, 1);
   addmatch(u5, u6, 1, 1);
   addmatch(u7, u3, 1, 1);

   test_assert(u3->count == 1);
   test_assert(u4->count == 2);
   test_assert(u5->count == 2);
   test_assert(u6->count == 3);

   // u7 points to u3, which is ambiguous.
   transfer_counts_and_update_canonicals(u7);


   test_assert(u3->canonical == NULL);
   test_assert(u3->count == 1);
   test_assert(u3->sphere_d == 1);
   
   test_assert(u7->canonical == NULL);
   test_assert(u7->count == 1);
   test_assert(u7->sphere_d == 1);
   
   test_assert(u5->canonical == u6);
   test_assert(u5->count == 0);
   test_assert(u5->sphere_d == 0);
   
   test_assert(u6->canonical == u6);
   test_assert(u6->count == 5);
   test_assert(u6->sphere_d == 0);

   test_assert(u4->canonical == u4);
   test_assert(u4->count == 2);
   test_assert(u4->sphere_d == 0);

   // Resolve ambiguous canonicals.
   mp_resolve_ambiguous(u5);
   test_assert(u3->canonical == NULL);
   test_assert(u5->canonical == u6);

   // u3 canonical must be u4, because is a true canonical.
   mp_resolve_ambiguous(u7);

   test_assert(u3->canonical == u4);
   test_assert(u7->canonical == u4);
   test_assert(u4->canonical == u4);
   test_assert(u5->canonical == u6);
   test_assert(u6->canonical == u6);
   test_assert(u3->count == 0);
   test_assert(u7->count == 0);
   test_assert(u4->count == 4);
   test_assert(u5->count == 0);
   test_assert(u6->count == 5);

   destroy_useq(u3);
   destroy_useq(u4);
   destroy_useq(u5);
   destroy_useq(u6);
   destroy_useq(u7);
}


void
test_starcode_4
(void)
// Test 'canonical_order'.
{
   useq_t *u1 = new_useq(1, "ABCD", NULL);
   useq_t *u2 = new_useq(2, "EFGH", NULL);
   test_assert_critical(u1 != NULL);
   test_assert_critical(u2 != NULL);

   // 'u1' and 'u2' have no canonical, so the
   // comparison is alphabetical.
   test_assert(canonical_order(&u1, &u2) < 0);
   test_assert(canonical_order(&u2, &u1) > 0);

   // Add match to 'u1', and update canonicals.
   addmatch(u1, u2, 1, 1);
   test_assert_critical(u1->matches != NULL);
   transfer_counts_and_update_canonicals(u1);
   test_assert(u1->count == 0);
   test_assert(u2->count == 3);

   // Now 'u1' and 'u2' have the same canonical ('u2')
   // so the comparison is again alphabetical.
   test_assert(canonical_order(&u1, &u2) < 0);
   test_assert(canonical_order(&u2, &u1) > 0);

   useq_t *u3 = new_useq(1, "CDEF", NULL);
   useq_t *u4 = new_useq(2, "GHIJ", NULL);
   test_assert_critical(u3 != NULL);
   test_assert_critical(u4 != NULL);

   // Comparisons with 'u1' or with 'u2' give the same
   // results because they have the same canonical ('u2').
   test_assert(canonical_order(&u1, &u3) == -1);
   test_assert(canonical_order(&u2, &u3) == -1);
   test_assert(canonical_order(&u3, &u1) == 1);
   test_assert(canonical_order(&u3, &u2) == 1);
   test_assert(canonical_order(&u1, &u4) == -1);
   test_assert(canonical_order(&u2, &u4) == -1);
   test_assert(canonical_order(&u4, &u1) == 1);
   test_assert(canonical_order(&u4, &u2) == 1);
   // Comparisons between 'u3' and 'u4' are alphabetical.
   test_assert(canonical_order(&u3, &u4) < 0);
   test_assert(canonical_order(&u4, &u3) > 0);

   // Add match to 'u3', and update canonicals.
   addmatch(u3, u4, 1, 1);
   test_assert_critical(u3->matches != NULL);
   transfer_counts_and_update_canonicals(u3);
   test_assert(u3->count == 0);
   test_assert(u4->count == 3);

   // Now canonicals ('u2' and 'u4') have the same counts
   // so comparisons are always alphabetical.
   test_assert(canonical_order(&u1, &u3) < 0);
   test_assert(canonical_order(&u2, &u3) < 0);
   test_assert(canonical_order(&u3, &u1) > 0);
   test_assert(canonical_order(&u3, &u2) > 0);
   test_assert(canonical_order(&u1, &u4) < 0);
   test_assert(canonical_order(&u2, &u4) < 0);
   test_assert(canonical_order(&u4, &u1) > 0);
   test_assert(canonical_order(&u4, &u2) > 0);
   test_assert(canonical_order(&u3, &u4) < 0);
   test_assert(canonical_order(&u4, &u3) > 0);

   useq_t *u5 = new_useq(1, "CDEF", NULL);
   useq_t *u6 = new_useq(3, "GHIJ", NULL);
   test_assert_critical(u5 != NULL);
   test_assert_critical(u6 != NULL);

   // Comparisons between canonicals.
   test_assert(canonical_order(&u1, &u5) == -1);
   test_assert(canonical_order(&u2, &u5) == -1);
   test_assert(canonical_order(&u5, &u1) == 1);
   test_assert(canonical_order(&u5, &u2) == 1);
   test_assert(canonical_order(&u1, &u6) == -1);
   test_assert(canonical_order(&u2, &u6) == -1);
   test_assert(canonical_order(&u6, &u1) == 1);
   test_assert(canonical_order(&u6, &u2) == 1);
   test_assert(canonical_order(&u3, &u5) == -1);
   test_assert(canonical_order(&u4, &u5) == -1);
   test_assert(canonical_order(&u5, &u3) == 1);
   test_assert(canonical_order(&u5, &u4) == 1);
   test_assert(canonical_order(&u3, &u6) == -1);
   test_assert(canonical_order(&u4, &u6) == -1);
   test_assert(canonical_order(&u6, &u3) == 1);
   test_assert(canonical_order(&u6, &u4) == 1);
   // Alphabetical comparisons.
   test_assert(canonical_order(&u5, &u6) < 0);
   test_assert(canonical_order(&u6, &u5) > 0);

   // Add match to 'u5', and update canonicals.
   addmatch(u5, u6, 1, 1);
   test_assert_critical(u5->matches != NULL);
   transfer_counts_and_update_canonicals(u5);
   test_assert(u5->count == 0);
   test_assert(u6->count == 4);

   // Comparisons between canonicals ('u2', 'u4', 'u6').
   test_assert(canonical_order(&u1, &u5) == 1);
   test_assert(canonical_order(&u2, &u5) == 1);
   test_assert(canonical_order(&u5, &u1) == -1);
   test_assert(canonical_order(&u5, &u2) == -1);
   test_assert(canonical_order(&u1, &u6) == 1);
   test_assert(canonical_order(&u2, &u6) == 1);
   test_assert(canonical_order(&u6, &u1) == -1);
   test_assert(canonical_order(&u6, &u2) == -1);
   test_assert(canonical_order(&u3, &u5) == 1);
   test_assert(canonical_order(&u4, &u5) == 1);
   test_assert(canonical_order(&u5, &u3) == -1);
   test_assert(canonical_order(&u5, &u4) == -1);
   test_assert(canonical_order(&u3, &u6) == 1);
   test_assert(canonical_order(&u4, &u6) == 1);
   test_assert(canonical_order(&u6, &u3) == -1);
   test_assert(canonical_order(&u6, &u4) == -1);
   // Alphabetical.
   test_assert(canonical_order(&u5, &u6) < 0);
   test_assert(canonical_order(&u6, &u5) > 0);

   useq_t *useq_array[6] = {u1,u2,u3,u4,u5,u6};
   useq_t *sorted[6] = {u5,u6,u1,u2,u3,u4};
   qsort(useq_array, 6, sizeof(useq_t *), canonical_order);
   for (int i = 0 ; i < 6 ; i++) {
      test_assert(useq_array[i] == sorted[i]);
   }

   destroy_useq(u1);
   destroy_useq(u2);
   destroy_useq(u3);
   destroy_useq(u4);
   destroy_useq(u5);
   destroy_useq(u6);

}


void
test_starcode_5
(void)
// Test 'count_order()'.
{

   useq_t *u1 = new_useq(1, "L@[ohztp{2@V(u(x7fLt&x80", NULL);
   useq_t *u2 = new_useq(2, "$Ee6xkB+.Q;Nk)|w[KQ;", NULL);
   test_assert(count_order(&u1, &u2) == 1);
   test_assert(count_order(&u2, &u1) == -1);
   test_assert(count_order(&u1, &u1) == 0);
   test_assert(count_order(&u2, &u2) == 0);

   destroy_useq(u1);
   destroy_useq(u2);

   for (int i = 0 ; i < 1000 ; i++) {
      char seq1[21] = {0};
      char seq2[21] = {0};
      for (int j = 0 ; j < 20 ; j++) {
         seq1[j] = untranslate[(int)(5 * drand48())];
         seq2[j] = untranslate[(int)(5 * drand48())];
      }
      int randint = (int)(4096 * drand48());
      u1 = new_useq(randint, seq1, NULL);
      u2 = new_useq(randint + 1, seq2, NULL);
      test_assert(count_order(&u1, &u2) == 1);
      test_assert(count_order(&u2, &u1) == -1);
      test_assert(count_order(&u1, &u1) == 0);
      test_assert(count_order(&u2, &u2) == 0);

      destroy_useq(u1);
      destroy_useq(u2);
   }

   // Case 1 (no repeat).
   char *sequences_1[10] = {
      "IRrLv<'*3S?UU<JF4S<,", "tcKvz5JTm!h*X0mSTg",
      "tW:0K&Mvtax<PP/qY6er", "hcU+f!=`.Xs6[a,C7XpN",
      ":3ILp'w?)f]4(a;mf%A9", "RlEF',$6[}ouJQyWqqT#",
      "U Ct`3w8(#KAE+z;vh,",  "[S^jXvNS VP' cwg~_iq",
      ".*/@*Q/]]}32kNB#`qqv", "#`Hwp(&,z|bN~07CSID'",
   };
   // Call to 'new_useq()' will also capitalize the letters.
   const char *sorted_1[10] = {
      "#`HWP(&,Z|BN~07CSID'", ".*/@*Q/]]}32KNB#`QQV", 
      "[S^JXVNS VP' CWG~_IQ", "U CT`3W8(#KAE+Z;VH,", 
      "RLEF',$6[}OUJQYWQQT#", ":3ILP'W?)F]4(A;MF%A9", 
      "HCU+F!=`.XS6[A,C7XPN", "TW:0K&MVTAX<PP/QY6ER", 
      "TCKVZ5JTM!H*X0MSTG",   "IRRLV<'*3S?UU<JF4S<,", 
   };

   useq_t *to_sort_1[10];
   for (int i = 0 ; i < 10 ; i++) {
      to_sort_1[i] = new_useq(i, sequences_1[i], NULL);
   }

   qsort(to_sort_1, 10, sizeof(useq_t *), count_order);
   for (int i = 0 ; i < 10 ; i++) {
      test_assert(strcmp(to_sort_1[i]->seq, sorted_1[i]) == 0);
      test_assert(to_sort_1[i]->count == 9-i);
      destroy_useq(to_sort_1[i]);
   }

   // Case 2 (repeats).
   char *sequences_2[6] = {
      "repeat", "repeat", "repeat", "abc", "abc", "xyz"
   };
   int counts[6] = {1,1,2,3,4,4};
   // Call to 'new_useq()' will also capitalize the letters.
   char *sorted_2[6] = {
      "ABC", "XYZ", "ABC", "REPEAT", "REPEAT", "REPEAT",
   };
   int sorted_counts[6] = {4,4,3,2,1,1};

   useq_t *to_sort_2[6];
   for (int i = 0 ; i < 6 ; i++) {
      to_sort_2[i] = new_useq(counts[i], sequences_2[i], NULL);
   }

   qsort(to_sort_2, 6, sizeof(useq_t *), count_order);
   for (int i = 0 ; i < 6 ; i++) {
      test_assert(strcmp(to_sort_2[i]->seq, sorted_2[i]) == 0);
      test_assert(to_sort_2[i]->count == sorted_counts[i]);
      destroy_useq(to_sort_2[i]);
   }

}


void
test_starcode_6
(void)
// Test 'pad_useq()' and 'unpad_useq()'
{

   gstack_t * useqS = new_gstack();
   test_assert_critical(useqS != NULL);

   useq_t *u1 = new_useq(1, "L@[ohztp{2@V(u(x7fLt&x80", NULL);
   useq_t *u2 = new_useq(2, "$Ee6xkB+.Q;Nk)|w[KQ;", NULL);
   test_assert_critical(u1 != NULL);
   test_assert_critical(u2 != NULL);

   push(u1, &useqS);
   push(u2, &useqS);
   test_assert(useqS->nitems == 2);

   int med;
   pad_useq(useqS, &med);
   // The call to 'new_useq()' will capitalize the sequence.
   test_assert(strcmp(u2->seq, "    $EE6XKB+.Q;NK)|W[KQ;") == 0);
   test_assert(med == 20);

   useq_t *u3 = new_useq(23, "0sdfd:'!'@{1$Ee6xkB+.Q;[Nk)|w[KQ;", NULL);
   test_assert_critical(u3 != NULL);
   push(u3, &useqS);
   test_assert(useqS->nitems == 3);

   pad_useq(useqS, &med);
   // The call to 'new_useq()' will capitalize the sequence.
   test_assert(strcmp(u1->seq, "         L@[OHZTP{2@V(U(X7FLT&X80") == 0);
   test_assert(strcmp(u2->seq, "             $EE6XKB+.Q;NK)|W[KQ;") == 0);
   test_assert(med == 24);

   unpad_useq(useqS);
   // The call to 'new_useq()' will capitalize the sequence.
   test_assert(strcmp(u1->seq, "L@[OHZTP{2@V(U(X7FLT&X80") == 0);
   test_assert(strcmp(u2->seq, "$EE6XKB+.Q;NK)|W[KQ;") == 0);
   test_assert(strcmp(u3->seq, "0SDFD:'!'@{1$EE6XKB+.Q;[NK)|W[KQ;") == 0);

   destroy_useq(u1);
   destroy_useq(u2);
   destroy_useq(u3);
   free(useqS);

}


void
test_starcode_7
(void)
// Test 'new_lookup()'
{

   int expected_klen[][4] = {
      {4,4,4,5},
      {4,4,5,5},
      {4,5,5,5},
      {5,5,5,5},
   };

   for (int i = 0 ; i < 4 ; i++) {
      lookup_t * lut = new_lookup(20+i, 20+i, 3);
      test_assert_critical(lut != NULL);
      test_assert(lut->kmers == 3+1);
      test_assert(lut->slen == 20+i);
      test_assert_critical(lut->klen != NULL);
      for (int j = 0 ; j < 4 ; j++) {
         test_assert(lut->klen[j] == expected_klen[i][j]);
      }
      destroy_lookup(lut);
   }

   for (int i = 0 ; i < 10 ; i++) {
      lookup_t * lut = new_lookup(59+i, 59+i, 3);
      test_assert_critical(lut != NULL);
      test_assert(lut->kmers == 3+1);
      test_assert(lut->slen == 59+i);
      test_assert_critical(lut->klen != NULL);
      for (int j = 0 ; j < 4 ; j++) {
         test_assert(lut->klen[j] == MAX_K_FOR_LOOKUP);
      }
      destroy_lookup(lut);
   }

}


void
test_starcode_8
(void)
// Test 'seq2id().'
{

   srand48(123);

   test_assert(seq2id("AAAAA", 4) == 0);
   test_assert(seq2id("AAAAC", 4) == 0);
   test_assert(seq2id("AAAAG", 4) == 0);
   test_assert(seq2id("AAAAT", 4) == 0);
   test_assert(seq2id("AAACA", 4) == 1);
   test_assert(seq2id("AAAGA", 4) == 2);
   test_assert(seq2id("AAATA", 4) == 3);
   test_assert(seq2id("AACAA", 4) == 4);
   test_assert(seq2id("AAGAA", 4) == 8);
   test_assert(seq2id("AATAA", 4) == 12);
   test_assert(seq2id("ACAAA", 4) == 16);
   test_assert(seq2id("AGAAA", 4) == 32);
   test_assert(seq2id("ATAAA", 4) == 48);
   test_assert(seq2id("CAAAA", 4) == 64);
   test_assert(seq2id("GAAAA", 4) == 128);
   test_assert(seq2id("TAAAA", 4) == 192);

   // Test 10,000 random cases (with no N).
   for (int i = 0 ; i < 10000 ; i++) {
      char seq[21] = {0};
      int id = 0;
      for (int j = 0 ; j < 20 ; j++) {
         int r = (int) drand48()*4;
         id += (r << 2*(19-j));
         seq[j] = untranslate[r+1];
      }
      test_assert(seq2id(seq, 20) == id);
   }

   // Test failure.
   test_assert(seq2id("AAAAN", 4) == 0);
   test_assert(seq2id("NAAAA", 4) == -1);

}


void
test_starcode_9
(void)
// Test 'lut_insert()' and lut_search().
{

   srand48(123);

   lookup_t *lut = new_lookup(20, 20, 3);
   test_assert_critical(lut != NULL);

   // Insert a too short string (nothing happens).
   useq_t *u = new_useq(0, "", NULL);
   test_assert_critical(u != NULL);
   test_assert(lut_insert(lut, u) == 0);
   destroy_useq(u);

   // Insert the following k-mers: ACG|TAGC|GCTA|TAGC|GATCA
   u = new_useq(0, "ACGTAGCGCTATAGCGATCA", NULL);
   test_assert_critical(u != NULL);
   test_assert(lut_insert(lut, u) == 0);
   test_assert(lut_search(lut, u) == 1);
   destroy_useq(u);

   u = new_useq(0, "CGTAGCGCTATAGCGATCAA", NULL);
   test_assert_critical(u != NULL);
   test_assert(lut_search(lut, u) == 1);
   destroy_useq(u);

   u = new_useq(0, "AAAATAGCGCCCCCCCCCCC", NULL);
   test_assert_critical(u != NULL);
   test_assert(lut_search(lut, u) == 1);
   destroy_useq(u);

   u = new_useq(0, "CCCCCCCCCCCCCCCGATCA", NULL);
   test_assert_critical(u != NULL);
   test_assert(lut_search(lut, u) == 1);
   destroy_useq(u);

   u = new_useq(0, "CCCCCGCTACCCCCCCCCCC", NULL);
   test_assert_critical(u != NULL);
   test_assert(lut_search(lut, u) == 1);
   destroy_useq(u);

   u = new_useq(0, "TAGCAAAAAAAAAAAAAAAA", NULL);
   test_assert_critical(u != NULL);
   test_assert(lut_search(lut, u) == 1);
   destroy_useq(u);

   u = new_useq(0, "CCCCCCCCCCCCCCGATCAC", NULL);
   test_assert_critical(u != NULL);
   test_assert(lut_search(lut, u) == 0);
   destroy_useq(u);

   u = new_useq(0, "AAAAAAAAAAAAAAAAAAAA", NULL);
   test_assert_critical(u != NULL);
   test_assert(lut_search(lut, u) == 0);
   destroy_useq(u);

   destroy_lookup(lut);
   lut = NULL;

   lut = new_lookup(20, 20, 3);
   test_assert_critical(lut != NULL);

   for (int i = 0 ; i < 10000 ; i++) {
      // Create random sequences without "N".
      char seq[21] = {0};
      for (int j = 0 ; j < 20 ; j++) {
         seq[j] = untranslate[(int)(1 + 4*drand48())];
      } 
      u = new_useq(0, seq, NULL);
      test_assert(lut_insert(lut, u) == 0);
      test_assert(lut_search(lut, u) == 1);
      destroy_useq(u);
   }

   destroy_lookup(lut);

   // Insert every 4-mer.
   lut = new_lookup(19, 19, 3);
   test_assert_critical(lut != NULL);
   char seq[20] = "AAAAAAAAAAAAAAAAAAA"; //AAA|AAAA|AAAA|AAAA
   for (int i = 0 ; i < 256 ; i++) {
      for (int j = 0 ; j < 4 ; j++) {
         char nt = untranslate[1 + (int)((i >> (2*j)) & 3)];
         seq[j+3] = seq[j+7] = seq[j+11] = seq[j+15] = nt;   
      }
      u = new_useq(0, seq, NULL);
      test_assert_critical(u != NULL);
      test_assert(lut_insert(lut, u) == 0);
      destroy_useq(u);
   }

   for (int i = 0 ; i < 4 ; i++) {
      test_assert(*lut->lut[i] == 255);
   }

   destroy_lookup(lut);

   // Insert randomly.
   lut = new_lookup(64, 64, 3);
   test_assert_critical(lut != NULL);
   for (int i = 0 ; i < 4 ; i++) {
      // MAX_K_FOR_LOOKUP
      test_assert_critical(lut->klen[i] == 14);
   }
   for (int i = 0 ; i < 4096 ; i++) {
      char seq[65] = {0};
      // Create random sequences without "N".
      for (int j = 0 ; j < 64 ; j++) {
         seq[j] = untranslate[(int)(1 + 4*drand48())];
      } 
      u = new_useq(0, seq, NULL);
      test_assert_critical(u != NULL);
      test_assert(lut_insert(lut, u) == 0);
      destroy_useq(u);
   }

   const int set_bits_per_byte[256] = {
      0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,
      1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
      1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
      2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
      1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
      2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
      2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
      3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
      1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
      2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
      2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
      3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
      2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
      3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
      3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
      4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8,
   };

   int jmax = 1 << (2*14 - 3);
   for (int i = 0 ; i < 4 ; i++) {
      int total = 0;
      for (int j = 0 ; j < jmax ; j++) {
         total += set_bits_per_byte[lut->lut[i][j]];
      }
      // The probability of collision is negligible.
      test_assert(total == 4096);
   }

   destroy_lookup(lut);

}


void
test_starcode_10
(void)
// Test 'read_file()'
{

   const char * expected[] = {
   "AGGGCTTACAAGTATAGGCC",
   "TGCGCCAAGTACGATTTCCG",
   "CCTCATTATTTGTCGCAATG",
   "AGGGCTTACAAGTATAGGCC",
   "AGGGCTTACAAGTATAGGCC",
   "GGGAGCCCACAGTAAGCGAA",
   "GGGAGCCCACAGTAAGCGAA",
   "TAGCCTGGTGCGACTGTCAT",
   "TAGCCTGGTGCGACTGTCAT",
   "GGAAGCCCACAGCAAGCGAA",
   "TGCGCCAAGTACGATTTCCG",
   "GGGAGCCCACAGTAAGCGAA",
   "AGGGGTTACAAGTCTAGGCC",
   "CCTCATTATTTGTCGCAATG",
   "GGGAGCCCACAGTAAGCGAA",
   "TAGCCTGGTGCGACTGTCAT",
   "AGGGCTTACAAGTATAGGCC",
   "TGCGCCAAGTACGATTTCCG",
   "CCTCATTATTTGTCGCAATG",
   "AGGGCTTACAAGTATAGGCC",
   "TAGCCTGGTGCGACTGTCAT",
   "AGGGCTTACAAGTATAGGCC",
   "TGCGCCAAGTAAGAATTCCG",
   "GGGAGCCCACAGTAAGCGAA",
   "GGGAGCCCACAGTAAGCGAA",
   "TGCGCCAAGTACGATTTCCG",
   "CCTCATTATTTGTCGCAATG",
   "TAGCCTGGTGCGACTGTCAT",
   "TGCGCCAAGTACGATTTCCG",
   "CCTCATTATTTGTCGCAATG",
   "CCTCATTATTTGTCGCAATG",
   "CCTCATTATTTACCGCAATG",
   "TAGCCTGGTGCGACTGTCAT",
   "TGCGCCAAGTACGATTTCCG",
   "TAACCTGGTGCGACTGTTAT",
   };

   // Read raw file.
   FILE *f = fopen("test_file.txt", "r");
   gstack_t *useqS = read_file(f, NULL, 0);
   test_assert(useqS->nitems == 35);
   for (int i = 0 ; i < useqS->nitems ; i++) {
      useq_t * u = (useq_t *) useqS->items[i];
      test_assert(u->count == 1);
      test_assert(strcmp(u->seq, expected[i]) == 0);
   }

   // Clean.
   for (int i = 0 ; i < useqS->nitems ; i++) {
      destroy_useq(useqS->items[i]);
   }
   free(useqS);
   fclose(f);

   // Read fasta file.
   f = fopen("test_file.fasta", "r");
   useqS = read_file(f, NULL, 0);
   test_assert(useqS->nitems == 5);
   for (int i = 0 ; i < useqS->nitems ; i++) {
      useq_t * u = (useq_t *) useqS->items[i];
      test_assert(u->count == 1);
      test_assert(strcmp(u->seq, expected[i]) == 0);
   }

   // Clean.
   for (int i = 0 ; i < useqS->nitems ; i++) {
      destroy_useq(useqS->items[i]);
   }
   free(useqS);
   fclose(f);

   // Read fastq file.
   f = fopen("test_file1.fastq", "r");
   useqS = read_file(f, NULL, 0);
   test_assert(useqS->nitems == 5);
   for (int i = 0 ; i < useqS->nitems ; i++) {
      useq_t * u = (useq_t *) useqS->items[i];
      test_assert(u->count == 1);
      test_assert(strcmp(u->seq, expected[i]) == 0);
   }

   // Clean.
   for (int i = 0 ; i < useqS->nitems ; i++) {
      destroy_useq(useqS->items[i]);
   }
   free(useqS);
   fclose(f);

   char *PE_expected[]= {
      "AGGGCTTACAAGTATAGGCC---------AAGGGCTTACAAGTATAGGC",
      "TGCGCCAAGTACGATTTCCG---------ATGCGCCAAGTACGATTTCC",
      "CCTCATTATTTGTCGCAATG---------ACCTCATTATTTGTCGCAAT",
      "AGGGCTTACAAGTATAGGCC---------AAGGGCTTACAAGTATAGGC",
      "AGGGCTTACAAGTATAGGCC---------AAGGGCTTACAAGTATAGGC",
   };

   char *PE_fastq_headers[] = {
      "AGGGCTTACAAGTATAGGCC/AAGGGCTTACAAGTATAGGC",
      "TGCGCCAAGTACGATTTCCG/ATGCGCCAAGTACGATTTCC",
      "CCTCATTATTTGTCGCAATG/ACCTCATTATTTGTCGCAAT",
      "AGGGCTTACAAGTATAGGCC/AAGGGCTTACAAGTATAGGC",
      "AGGGCTTACAAGTATAGGCC/AAGGGCTTACAAGTATAGGC",
   };

   // Read paired-end fastq file.
   FILE *f1 = fopen("test_file1.fastq", "r");
   FILE *f2 = fopen("test_file2.fastq", "r");
   useqS = read_file(f1, f2, 0);
   test_assert(useqS->nitems == 5);
   for (int i = 0 ; i < useqS->nitems ; i++) {
      useq_t * u = (useq_t *) useqS->items[i];
      test_assert(u->count == 1);
      test_assert(strcmp(u->seq, PE_expected[i]) == 0);
      test_assert(strcmp(u->info, PE_fastq_headers[i]) == 0);
   }

   // Clean.
   for (int i = 0 ; i < useqS->nitems ; i++) {
      destroy_useq(useqS->items[i]);
   }
   free(useqS);
   fclose(f1);
   fclose(f2);



}


void
test_seqsort
(void)
{

   gstack_t * useqS = new_gstack();

   // Basic cases.
   for (int i = 0 ; i < 9 ; i++) {
      push(new_useq(1, "A", NULL), &useqS);
   }
   test_assert(useqS->nitems == 9);
   test_assert(seqsort((useq_t **) useqS->items, 9, 1) == 1);
   test_assert_critical(useqS->items[0] != NULL);
   useq_t *u = useqS->items[0];
   test_assert(strcmp(u->seq, "A") == 0);
   test_assert(u->count == 9);
   test_assert(u->canonical == NULL);
   for (int i = 1 ; i < 9 ; i++) {
      test_assert(useqS->items[i] == NULL);
   }
   destroy_useq(useqS->items[0]);

   useqS->nitems = 0;
   for (int i = 0 ; i < 9 ; i++) {
      push(new_useq(1, i % 2 ? "A":"B", NULL), &useqS);
   }
   test_assert(useqS->nitems == 9);
   test_assert(seqsort((useq_t **) useqS->items, 9, 1) == 2);
   test_assert_critical(useqS->items[0] != NULL);
   test_assert_critical(useqS->items[1] != NULL);
   for (int i = 2 ; i < 9 ; i++) {
      test_assert(useqS->items[i] == NULL);
   }
   u = useqS->items[0];
   test_assert(strcmp(u->seq, "A") == 0);
   test_assert(u->count == 4);
   test_assert(u->canonical == NULL);
   u = useqS->items[1];
   test_assert(strcmp(u->seq, "B") == 0);
   test_assert(u->count == 5);
   test_assert(u->canonical == NULL);
   destroy_useq(useqS->items[0]);
   destroy_useq(useqS->items[1]);

   // Case 1 (no repeat).
   char *sequences_1[10] = {
      "IRrLv<'*3S?UU<JF4S<,", "tcKvz5JTm!h*X0mSTg",
      "tW:0K&Mvtax<PP/qY6er", "hcU+f!=`.Xs6[a,C7XpN",
      ":3ILp'w?)f]4(a;mf%A9", "RlEF',$6[}ouJQyWqqT#",
      "U Ct`3w8(#KAE+z;vh,",  "[S^jXvNS VP' cwg~_iq",
      ".*/@*Q/]]}32kNB#`qqv", "#`Hwp(&,z|bN~07CSID'",
   };
   // The call to 'new_useq()' will capitalize the sequences.
   const char *sorted_1[10] = {
      "TCKVZ5JTM!H*X0MSTG",   "U CT`3W8(#KAE+Z;VH,",
      "#`HWP(&,Z|BN~07CSID'", ".*/@*Q/]]}32KNB#`QQV",
      ":3ILP'W?)F]4(A;MF%A9", "HCU+F!=`.XS6[A,C7XPN",
      "IRRLV<'*3S?UU<JF4S<,", "RLEF',$6[}OUJQYWQQT#",
      "TW:0K&MVTAX<PP/QY6ER", "[S^JXVNS VP' CWG~_IQ",
   };

   useq_t *to_sort_1[10];
   for (int i = 0 ; i < 10 ; i++) {
      to_sort_1[i] = new_useq(1, sequences_1[i], NULL);
   }

   test_assert(seqsort(to_sort_1, 10, 1) == 10);
   for (int i = 0 ; i < 10 ; i++) {
      test_assert(strcmp(to_sort_1[i]->seq, sorted_1[i]) == 0);
      test_assert(to_sort_1[i]->count == 1);
      destroy_useq(to_sort_1[i]);
   }

   // Case 2 (different lengths).
   char *sequences_2[10] = {
      "IRr",                  "tcKvz5JTm!h*X0mSTg",
      "tW:0K&Mvtax<PP/qY6er", "hcU+f!=`.Xs6[a,C7XpNwoi~OWe88",
      "z3ILp'w?)f]4(a;mf9",   "RlEFWqqT#",
      "U Ct`3w8(#Kz;vh,",     "aS^jXvNS VP' cwg~_iq",
      ".*/@*Q/]]}32#`",       "(&,z|bN~07CSID'",
   };
   // The call to 'new_useq()' will capitalize the sequences.
   const char *sorted_2[10] = {
      "IRR",                  "RLEFWQQT#",
      ".*/@*Q/]]}32#`",       "(&,Z|BN~07CSID'",
      "U CT`3W8(#KZ;VH,",     "TCKVZ5JTM!H*X0MSTG",
      "Z3ILP'W?)F]4(A;MF9",   "AS^JXVNS VP' CWG~_IQ",
      "TW:0K&MVTAX<PP/QY6ER", "HCU+F!=`.XS6[A,C7XPNWOI~OWE88",
   };

   useq_t *to_sort_2[10];
   for (int i = 0 ; i < 10 ; i++) {
      to_sort_2[i] = new_useq(1, sequences_2[i], NULL);
   }

   test_assert(seqsort(to_sort_2, 10, 1) == 10);
   for (int i = 0 ; i < 10 ; i++) {
      test_assert(strcmp(to_sort_2[i]->seq, sorted_2[i]) == 0);
      test_assert(to_sort_2[i]->count == 1);
      destroy_useq(to_sort_2[i]);
   }

   // Case 3 (repeats).
   char *sequences_3[6] = {
      "repeat", "repeat", "repeat", "repeat", "repeat", "xyz"
   };
   // The call to 'new_useq()' will capitalize the sequences.
   char *sorted_3[6] = {
      "XYZ", "REPEAT", NULL, NULL, NULL, NULL,
   };
   int counts[2] = {1,5};

   useq_t *to_sort_3[6];
   for (int i = 0 ; i < 6 ; i++) {
      to_sort_3[i] = new_useq(1, sequences_3[i], NULL);
   }

   test_assert(seqsort(to_sort_3, 6, 1) == 2);
   for (int i = 0 ; i < 2 ; i++) {
      test_assert(strcmp(to_sort_3[i]->seq, sorted_3[i]) == 0);
      test_assert(to_sort_3[i]->count == counts[i]);
      destroy_useq(to_sort_3[i]);
   }
   for (int i = 2 ; i < 6 ; i++) {
      test_assert(to_sort_3[i] == NULL);
   }


   // Case 4 (realistic).
   char *seq[35] = {
   "AGGGCTTACAAGTATAGGCC",
   "TGCGCCAAGTACGATTTCCG",
   "CCTCATTATTTGTCGCAATG",
   "AGGGCTTACAAGTATAGGCC",
   "AGGGCTTACAAGTATAGGCC",
   "GGGAGCCCACAGTAAGCGAA",
   "GGGAGCCCACAGTAAGCGAA",
   "TAGCCTGGTGCGACTGTCAT",
   "TAGCCTGGTGCGACTGTCAT",
   "GGAAGCCCACAGCAAGCGAA",
   "TGCGCCAAGTACGATTTCCG",
   "GGGAGCCCACAGTAAGCGAA",
   "AGGGGTTACAAGTCTAGGCC",
   "CCTCATTATTTGTCGCAATG",
   "GGGAGCCCACAGTAAGCGAA",
   "TAGCCTGGTGCGACTGTCAT",
   "AGGGCTTACAAGTATAGGCC",
   "TGCGCCAAGTACGATTTCCG",
   "CCTCATTATTTGTCGCAATG",
   "AGGGCTTACAAGTATAGGCC",
   "TAGCCTGGTGCGACTGTCAT",
   "AGGGCTTACAAGTATAGGCC",
   "TGCGCCAAGTAAGAATTCCG",
   "GGGAGCCCACAGTAAGCGAA",
   "GGGAGCCCACAGTAAGCGAA",
   "TGCGCCAAGTACGATTTCCG",
   "CCTCATTATTTGTCGCAATG",
   "TAGCCTGGTGCGACTGTCAT",
   "TGCGCCAAGTACGATTTCCG",
   "CCTCATTATTTGTCGCAATG",
   "CCTCATTATTTGTCGCAATG",
   "CCTCATTATTTACCGCAATG",
   "TAGCCTGGTGCGACTGTCAT",
   "TGCGCCAAGTACGATTTCCG",
   "TAACCTGGTGCGACTGTTAT",
   };

   char *sorted_4[10] = {
   "AGGGCTTACAAGTATAGGCC",
   "AGGGGTTACAAGTCTAGGCC",
   "CCTCATTATTTACCGCAATG",
   "CCTCATTATTTGTCGCAATG",
   "GGAAGCCCACAGCAAGCGAA",
   "GGGAGCCCACAGTAAGCGAA",
   "TAACCTGGTGCGACTGTTAT",
   "TAGCCTGGTGCGACTGTCAT",
   "TGCGCCAAGTAAGAATTCCG",
   "TGCGCCAAGTACGATTTCCG",
   };

   int counts_4[10] = {6,1,1,6,1,6,1,6,1,6};

   // Test 'seqsort()' with 1 to 8 threads.
   for (int t = 1 ; t < 9 ; t++) {

      useqS->nitems = 0;
      for (int i = 0 ; i < 35 ; i++) {
         push(new_useq(1, seq[i], NULL), &useqS);
      }

      test_assert(seqsort((useq_t **) useqS->items, 35, t) == 10);
      for (int i = 0 ; i < 10 ; i++) {
         test_assert_critical(useqS->items[i] != NULL);
         u = useqS->items[i];
         test_assert(strcmp(u->seq, sorted_4[i]) == 0);
         test_assert(u->count == counts_4[i]);
         test_assert(u->canonical == NULL);
         destroy_useq(u);
      }
      for (int i = 10 ; i < 35 ; i++) {
         test_assert(useqS->items[i] == NULL);
      }

   }

   free(useqS);

}

// Test cases for export.
const test_case_t test_cases_starcode[] = {
   {"starcode/base/1",  test_starcode_1},
   {"starcode/base/2",  test_starcode_2},
   {"starcode/base/3",  test_starcode_3},
   {"starcode/base/4",  test_starcode_4},
   {"starcode/base/5",  test_starcode_5},
   {"starcode/base/6",  test_starcode_6},
   {"starcode/base/7",  test_starcode_7},
   {"starcode/base/8",  test_starcode_8},
   {"starcode/base/9",  test_starcode_9},
   {"starcode/base/10", test_starcode_10},
   {"starcode/seqsort", test_seqsort},
   {NULL, NULL}
};
