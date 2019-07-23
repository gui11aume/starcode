## Starcode: Sequence clustering based on all-pairs search ##
[![Build Status](https://travis-ci.org/gui11aume/starcode.svg?branch=master)](https://travis-ci.org/gui11aume/starcode) [![Coverage Status](https://coveralls.io/repos/gui11aume/starcode/badge.svg?branch=master&service=github)](https://coveralls.io/github/gui11aume/starcode?branch=master)
---
## Contents: ##
    1. What is starcode?
    2. Source file list.
    3. Compilation and installation.
    4. Running starcode.
    5. Running starcode-umi.
    6. File formats.
    7. License.
	8. Citation.

---
## I. What is starcode?   ##


Starcode is a DNA sequence clustering software. Starcode clustering is
based on all pairs search within a specified
[Levenshtein distance](https://en.wikipedia.org/wiki/Levenshtein_distance)
(allowing insertions and deletions),
followed by a clustering algorithm: Message Passing, Spheres or Connected
Components. Typically, a file containing a set of DNA sequences is passed
as input, jointly with the desired clustering distance and algorihtm.
Starcode returns the canonical sequence of the cluster, the cluster size,
the set of different sequences that compose the cluster and the input line
numbers of the cluster components.

Starcode has many applications in the field of biology, such as DNA/RNA
motif recovery, barcode/UMI clustering, sequencing error recovery, etc.


II. Source file list
--------------------

* **starcode-umi**           Starcode script to cluster UMI-tagged sequences.
* **main-starcode.c**        Starcode main file (parameter parsing).
* **starcode.c**             Main starcode algorithm.
* **trie.c**                 Trie search and construction functions.
* **view.c**                 Graphical representation of starcode output.
* **Makefile**               Make instruction file.


III. Compilation and installation
---------------------------------

To install starcode, clone this git repository (or manually download the 
latest release [starcode v1.3](https://github.com/gui11aume/starcode/releases/tag/1.3)):

 > git clone https://github.com/gui11aume/starcode

the files should be downloaded in a folder named 'starcode'. Use make to
compile (Mac users require 'xcode', available at the Mac Appstore):

 > make -C starcode

a binary file 'starcode' will be created. You can optionally make a
symbolic link to run starcode from any directory:

 > sudo ln -s starcode/starcode /usr/bin/starcode


IV. Running starcode
--------------------

Starcode runs on Linux and Mac. It has not been tested on Windows.

### Usage:

  > starcode [options] {[-i] INPUT_FILE | -1 PAIRED_END_FILE1 -2 PAIRED_END_FILE2} [-o OUTPUT_FILE]
  
### Starcode defaults (please read this):

  By default, Starcode uses clustering parameters that are meaningful on many problems. Yet, the
  output may not look exactly like you expect. This may be for the following reasons:
    
  1. The clustering method is Message Passing. This means that clusters are built bottom-up by merging
  small clusters into bigger ones. The process is recursive, so **sequences in a cluster may not be
  neighbors**, _i.e._, they may not be within the specified Levenshtein distance. If this must be the
  case, use sphere clustering instead (see option **-s** or **--spheres** below).
  
  1. The clustering ratio is 5. This means that a cluster can absorb a smaller one only if it is at
  least five times bigger. A practical implication is that **clusters of similar size are not merged**.
  You can choose another threshold for merging clusters (see option **-r** or **--cluster-ratio** below).
      
### Search options:
  
  **-d or --distance** *distance*

     Defines the maximum Levenshtein distance for clustering.
     When not set it is automatically computed as:
     min(8, 2 + [median seq length]/30)
	 
### Clustering algorithm:
  
  **-r or --cluster-ratio** *ratio*

     (Message passing only) Specifies the minimum sequence count ratio to cluster two matching
	 sequences, i.e. two matching sequences A and B will be clustered together only if
	 count(A) > ratio * count(B).
	 Sparse datasets may need to set -r to small values (minimum is 1.0) to trigger clustering.
     Default is 5.0.
	 
  **-s or --spheres**

     Use sphere clustering algorithm instead of message passing (MP). Spheres is more greedy than MP:
	 sorted by size, centroids absorb all their matches.
	 
  **-c or --connected-comp**

     Clusters are defined by the connected components.

### Output format:
	 
  **--non-redundant**
  
     Removes redundant sequences from the output. Only the canonical sequence of each cluster is
	 returned.

  **--print-clusters**
  
     Adds a third column to the starcode output, containing the sequences that compose each cluster.
	 By default, the output contains only the centroid and the counts.

  **--seq-id**
     
     Shows the input sequence order (1-based) of the cluster components.
	 
### Input files:
- Single-file mode:

  **-i or --input** *file*

     Specifies input file.

- Paired-end fastq files:
   
  **-1** *file1* **-2** *file2*

     Specifies two paired-end FASTQ files for paired-end clustering mode.

Standard input is used when neither **-i** nor **-1/-2** are set.

### Output files:

  **-o or --output** *file*

     Specifies output file. When not set, standard output is used instead.
	 
  **--output1** *file1* **--output2** *file2*
  
	 (Paired-end mode with --non-redundant option only). Specifies the output file names of the
	  processed paired-end files.
	  
Standard output is used when **-o** is not set.

When --output1/2 is not specified in paired-end --non-redundant mode, the output file
names are the input file names with a "-starcode" suffix.
	 
### Other options:

  **-t or --threads** *threads*

     Defines the maximum number of parallel threads.
     Default is 1.


  **-q or --quiet**

     Non verbose. By default, starcode prints verbose information to
     the standard error channel.
	 
  **-v or --version**

     Prints version information.


  **-h or --help**

     Prints usage information.
	 
V. Running starcode-umi
-----------------------

Starcode-umi is a python script that uses `starcode` to cluster UMI-tagged sequences.
UMI-tagged sequences are assumed to contain a unique molecular identifier at the beginning
of the read followed by some other (longer) sequence. Starcode-umi performs a double round
of clustering and merging to find the best possible clusters of UMI and sequence pairs.


### Usage:

  > starcode-umi [options] --umi-len *N* input_file1 [input_file2]
  
### Required arguments:

  **--umi-len** *number*

     Defines the length of the UMI tags. Adding some extra nucleotides may improve the clustering
	 performance.
	 
  **--starcode-path** *path*
  
	  Path to `starcode` binary file. Default is `./starcode`.
	  
### Clustering options:

  **--umi-d** *distance*
  
     Match distance (Levenshtein) for the UMI region.

  **--seq-d** *distance*
  
     Match distance (Levenshtein) for the sequence region.
	 
  **--umi-cluster** *clustering algorithm*
  
     Clustering algorithm to be used in the UMI region. ('mp' for message passing, 's' for spheres,
	 'cc' for connected components). Default is message passing.

  **--seq-cluster** *clustering algorithm*
  
     Clustering algorithm to be used in the seq region. ('mp' for message passing, 's' for spheres,
	 'cc' for connected components). Default is message passing.
	 
  **--umi-cluster-ratio** *clustering algorithm*
  
     (Only for message passing in UMI). Minimum clustering ratio (same as -r option in starcode).
	 
  **--seq-cluster-ratio** *clustering algorithm*
  
     (Only for message passing in seq). Minimum clustering ratio (same as -r option in starcode).
	 
  **--seq-trim** *trim*
  
      Use only *trim* nucleotides of the sequence for clustering. Starcode becomes memory inefficient
	  with very long sequences, this parameter defines the maximum length of the sequence that will
	  be used for clustering. Set it to 0 to use the full sequence. Default is 50.

### Output options:

  **--seq-id**
     
     Shows the input sequence order (1-based) of the cluster components.

### Other options:

  **--umi-threads** *threads*

     Defines the maximum number of parallel threads to be used in the UMI process.
     Default is 1.

  **--seq-threads** *threads*

     Defines the maximum number of parallel threads to be used in the sequence process.
     Default is 1.


VI. File formats
---------------

### VI.I. Supported input file formats: ###

####  VI.I.I. Plain text: ####

  Consists of a file containing one sequence per line. Only the standard
  DNA-base characters are supported ('A', 'C', 'G', 'T'). The sequences
  may not contain empty spaces at the beginning or the end of the string,
  as these will be counted as alignment characters. The file may not
  contain empty lines as these will be considered as zero-length sequences.
  The sequences do not need to be sorted and may be repeated.
  
  Example:

    TTACTATCGATCATCATCGACTGACTACG
    ACTGCATCGACTAGCTACGACTACGCTACCATCAG
    TTACTATCGATCATCATCGACTGACTAGC
    ACTACGACTACGACTCAGCTCACTATCAGC
    GCATCGACCGCTACTACGCATACTACGACATC


####  VI.I.II. Plain text with sequence count: ####

  If the count of the sequences is known, it may be specified in the input
  file using the following format:

  > [SEQUENCE]\t[COUNT]\n

  Where '\t' denotes the TAB character and '\n' the NEWLINE character.
  The sequences do not need to be sorted and may be repeated as well. If
  a repeated sequence is found, their counts will be addded together. As
  before, the sequences may not contain any additional characters and the
  file may not contain empty lines.

  Example:

    TATCGACTCTATCTATCGCTGATGCGTAC       200
    CGAGCCGCCGGCACGTCACGACGCATCAA       1
    TAGCACCTACGCATCTCGACTATCACG         234
    CGAGCCGCCGGCACGTCACGACGCATCAA       17
    TGACTCTATCAGCTAC                    39


####  VI.I.III. FASTA/FASTQ ####

  Starcode supports FASTA and FASTQ files as well. Note, however, that
  starcode does not use the quality factors and the only relevant
  information is the sequence itself. The FASTA/FASTQ labels will not
  be used to identify the sequences in the output file. The sequences do
  not need to be sorted and may be repeated.

  Example FASTA:

    > FASTA sequence 1 label
    ATGCATCGATCACTCATCAGCTACAG
    > FASTA sequence 2 label
    TATCGACTATCTACGACTACATCA
    > FASTA sequence 3 label
    ATCATCACTCTAGCAGCGTACTCGCA
    > FASTA sequence 4 label
    ATGCATCGATTACTCATCAGCTACAG

  Example FASTQ:

    @ FASTQ sequence 1 label
    CATCGAGCAGCTATGCAGCTACGAGT
    +
    -$#'%-#.&)%#)"".)--'*()$)%
    @ FASTQ sequence 2 label
    TACTGCTGATATTCAGCTCACACC
    +
    ,*#%+#&*$-#,''+*)'&.,).,


### VI.II. Output formats: ###

#### VI.II.I Standard output format: ####

  Starcode prints a line for each detected cluster with the following
  format:

  > [CANONICAL SEQUENCE]\t[CLUSTER SIZE]\t[CLUSTER SEQUENCES]\n
  
  Where '\t' denotes the TAB character and '\n' the NEWLINE character.
  'CANONICAL SEQUENCE' is the sequence of the cluster that has more
  counts, 'CLUSTER SIZE' is the aggregated count of all the sequences
  that form the cluster, and 'CLUSTER SEQUENCES' is a list of all the
  cluster sequences separated by commas and in arbitrary order. The
  lines are printed sorted by 'CLUSTER SIZE' in descending order.

  For instance, an execution with the following input and clustering
  distance of 3 (-d3):

    TAGCTAGACGTA   250
    TAGCTAGCCGTA   10
    TAAGCTAGGGGT   16
    ACGCGAGCGGAA   155
    ACTTTAGCGGAA   1

  would produce the following output:

    TAGCTAGACGTA    260       TAGCTAGACGTA,TAGCTAGCCGTA
    ACGCGAGCGGAA    156       ACGCGAGCGGAA,ACTTTAGCGGAA
    TAAGCTAGGGGT    16        TAAGCTAGGGGT

  The same example executed with a more restrictive distance -d2 would
  produce the following output:

    TAGCTAGACGTA    260       TAGCTAGACGTA,TAGCTAGCCGTA
    ACGCGAGCGGAA    155       ACGCGAGCGGAA
    TAAGCTAGGGGT    16        TAAGCTAGGGGT
    ACTTTAGCGGAA    1         ACTTTAGCGGAA

#### VI.II.II Non-redundant output format: ####

  In non-redundant output mode, starcode only prints the canonical
  sequence of each cluster, one per line. Following the example from
  the previous section, the output with distance 3 (-d3) would be:

      TAGCTAGACGTA
      ACGCGAGCGGAA
    
  whereas for -d2:

      TAGCTAGACGTA
      ACGCGAGCGGAA
      TAAGCTAGGGGT
      ACTTTAGCGGAA


VII. License
-----------

Starcode is licensed under the GNU General Public License, version 3
(GPLv3), for more information read the LICENSE file or refer to:

  http://www.gnu.org/licenses/


VIII. Citation
--------------

If you use our software, please cite:

Zorita E, Cusco P, Filion GJ. 2015. [Starcode: sequence clustering based on all-pairs search.](http://bioinformatics.oxfordjournals.org/content/31/12/1913.abstract) Bioinformatics 31 (12): 1913-1919.
