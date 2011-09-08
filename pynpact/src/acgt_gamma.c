#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

double	SIGNIFICANCE= 0.001;
int	RANDOMIZE= 0;
#define MAX_ENTROPY 0.40

# define WRITE_SEQUENCES 0

#define BASE_DIR "/Volumes/2TB_DIsk/"
#define BASE_DIR_THRESHOLD_TABLES "/Users/luciano/ACGT_ORFS/data/"

# define ATG_POS5 45
# define ATG_POS3 12
# define GTG_POS5 45
# define GTG_POS3 12
# define TTG_POS5 45
# define TTG_POS3 12

int	MYCOPLASMA= 0;

/* Boolean RANDOMIZE forces if true a randomization (shuffling) of the sequence to be analyzed (both HHS and G-test analyses) */

// #define DOWN_G 16.919  // Value at which G-hss is terminated, corresponding to G-Prob = 0.05; d.f. = 9
#define DOWN_G 21.67  // Value at which G-hss is terminated, corresponding to G-Prob = 0.01; d.f. = 9
#define mHL 36         // Minimum hit-length

/* If REPETITIONS = 0, it reads threshold values from table and allowed values are: 0.01, 0.001, 0.0001.                 */
/* If REPETITIONS > 0 distribution is sampled and SIGNIFICANCE is meaninigful only if REPETITIONS >= (1 / SIGNIFICANCE). */
/* LEN_TRANSFORM is set to compute linear regressions of threshold scores as a function of sequence length (=0), ln(len) (=1), ln(ln(len)) (=2, default) or ln(ln(ln(len))) (=3). */

#define REPETITIONS 0
#define LEN_TRANSFORM 2		// If 

#if !REPETITIONS
float	threshold[3][23426][2];
#endif

#if REPETITIONS
float	threshold[1][1][1];
#endif

#define DOWNTHR 0.05
#define MAX_ORF_SIZE 500000
#define MAX_LINE 400
#define SHUFFLE 1
#define EPSILON 0.01
#define NUMBERS { 0,-1,1,-1,-1,-1,2,-1,-1,-1,-1,-1,-1,1,-1,-1,-1,-1,-1,3,3,-1,-1,-1,-1,-1 }  /* n is coded as c to avoid insertion of stop codons */
#define LETTERS "acgtn"
# define AA_LETTERS "KNKNTTTTRSRSIIMIQHQHPPPPRRRRLLLLEDEDAAAAGGGGVVVV*Y*YSSSSWCWCLFLF"
# define AA_NUMBERS { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63 }

int	NAME_OFFSET= 0;
char	common_name[50];
int	ttinvalid_base= 0;
int	numbers[]= NUMBERS;
char	letters[]= LETTERS;
int	aa_numbers[]= AA_NUMBERS;
char	aa_letters[]= AA_LETTERS;
char	RGB[]= "RGB";
char	strnd[]= "DC";
char	ORF[6*MAX_ORF_SIZE];
char	buff[MAX_LINE];
char	Output_name[9 * 100];

int	length[1000]= { 0 };

// If threshold probability values are read from table (REPETITIONS = 0) they are stored in a matrix threshold[i][j][2]:
// i identifies the probability value p[i] ( p[0]= 0.01; p[1]= 0.001; p[2]= 0.0001 )
// j is the address corresponding to the nucleotide composition as reported by function position()
// Thersholds are a linear function of X, where X is length (LEN_TRANSFORM= 0), ln(length) (LEN_TRANSFORM= 1),
// ln(ln(length)) (LEN_TRANSFORM= 2 (default)), or ln(ln(ln(length))) (LEN_TRANSFORM= 3);
// Threshold[i][j][0]= a and threshold[i][j][1]= b, where score(threshold) = aX + b;

int	tot_orfs[6]= { 0 };
int	**orf_num[6];

int	search_size;

double	sc[384];        // Old: sc[6][64]

FILE *output, *fp;

char Prediction[13][30];
char Annotation[5][15];

// Chi-square thresholds with d.f.=6

double PChi6[][2]=  {
		     {0.5, 5.35},
		     {0.4, 6.21},
		     {0.3, 7.23},
		     {0.2, 8.56},
		     {0.1, 10.645},
		     {0.05, 12.592},
		     {0.000001, 38.26},
		     {0.00001, 33.11},
		     {0.0001, 27.86},
		     {0.001, 22.46},
		     {0.005, 18.548},
		     {0.01, 16.812},
		     {0.025, 14.449},
		     {0.05, 12.592}
	    	   };

struct exons
        {
        char    strand;
	char	name[50];
        int     num_exons;
        int     from;		// First coding position encountered along the genome
        int     to;		// Last coding position encountered along the genome (excluding stop codon).
        int     *start;		// 5' position of each exon, numbered 5' to 3'.
        int     *newstart;	// Modified 5' position of each exon, numbered 5' to 3'.
        int     *newend;	// Modified 3' position of each exon, numbered 5' to 3'.
        int     *end;		// 3' position of each exon (excluding stop codon), numbered 5' to 3'.
	int	*fpos;		// Position in ffrom array where file-address of start of exon sequence is recorded.
	int	*type;		// Type of exon as defined in Annotation[][] from function define_characterizations().
        char    *color;		// color of exon.
        int     *frame;		// frame of exon.
        int     span;		// Length of gene from first coding position to last coding position (excluding stop codon) = to - from + 1.
        int     len;		// Sum of the lengths of all exons (including stop codon).
	double  entropy;
        } *gene;

struct HSSs {
	   int	  stop1;
	   int	  stop2;
	   char	  strand;
	   int	  frame;
	   char	  color;
	   int	  orf_num;
	   int	  hit_num;
	   int	  previous_hit;
	   int	  next_hit;
	   int	  fromp;
	   int	  top;
	   int	  len;
	   int	  type;
	   int	  atg;
	   int	  gtg;
	   int	  ttg;
	   int	  start;
	   int	  seqlen;
	   char	  *seq;
	   int	  hsuper;
	   int	  gsuper;
	   int	  exten;
           char	  hit_type;	// 'H' (hss) or 'G' (G-test).
	   float  prob;
	   double nuc[24];
	   double score;
	   double entropy;
	   double sig_len;
	   char	  global_sig;
	   double G;
	   char	  pstring[15];
	   } *hss;

struct Gstruct
{
	char strand;
        int pos[2];
        float score;
} *Ghit;


void	define_characterizations();
void	read_tables();
int	analyze_genome(int n, int tot_hss, int *on);
int	genome_composition(double *tnuc, long *ffrom, int *o, int k);
void	get_sequence(int from, int len, char strand, char *ORF);
long	annotation(int *ncds, int *nexons);
void	sort(double * array, int size);
void	build_scores(char *seg, int n, double *sc);
int	score_orf_random(char *seg, int n, int tot_hss);
int	score_orf_table(char *seg, int n, int tot_hss);
double	score(char *seq,int n,double *sc,int *from,int *to, int flag);
double entropy(char *seq, int n);
int	get_codon(char *seq);
int	maxG_test(char *seq, int len, int ori, char strand, int frame, int orfn, int tot_Ghits, int *on);
double	G_test(char *seq, int n, char Pg[], int *k);
void	complement(char *seq,int len);
void	invert_sequence(char *seq,int len);
void    write_complement(char *seq,int len, char *cseq);
void    copy_sequence(char *seq,int len, char *cseq);
void	composition(char *seq, int len, double *nuc, int offset);
void	clear_vector(double *vector, int size);
void	clear_intvector(int *vector, int size);
void	copy_vector(double *from_vector, double *to_vector, int size);
void	add_vector(int *vector, int *add_vector, int size);
void	difference_vector(double *vector, double *minus_vector, double *result_vector, int size);
void	rescue_hss(int to_hss);
void	process_hss(int from_hss, int to_hss, int ncds);
void	write_results(int from_hss, int to_hss, int ncds, int genome_size);
int 	find_Ghits(int genome_size, int tot_hss, long bytes_from_origin, int *on);
void	characterize_published_genes(int ncds, int tot_Ghits, double nuc[], long bytes_from_origin, long *ffrom);
void	write_published_exon(int j, int h, int k, int from, int to, int s1, int s2, double G, char Pg[], double nuc[], FILE *output1, FILE *output2, FILE *output3);
void	shuffle(char *seq, int n, int remove_stops);
int	position(int a, int c, int g, int t);
void	assign_gene_positions(long *ffrom, int *o, int ncds, int nexons);
void	reorder_hits(int n, int orfn);
void	renumber_orf_hits();
void	check_lists();
void	print_nucleotides(char *seq, int len, FILE *output);
void	print_amino_acids(char *seq, int len, FILE *output);

int check;

int main (int argc, char *argv[])
{
char	organism_file[256], *p;
int	i, j, k, h, genome_size, tot_hss= 0, *o, tot_Ghits= 0, ncds, nexons= 0, on= 1;
long	*ffrom, bytes_from_origin;
int     minutes, secs;
time_t	t0, t1, t2;
double	nuc[24], tnuc[5]= {0.0};
		
define_characterizations();
	if(!REPETITIONS) read_tables();

srand48(time(NULL));
t0= time(NULL);

strcpy(organism_file,BASE_DIR);
strcat(organism_file,argv[1]);

if(strstr(organism_file, "Mycoplasma") || strstr(organism_file, "Mesoplasma") || strstr(organism_file, "Ureaplasma") || strstr(organism_file, "Candidatus_Hodgkinia")) MYCOPLASMA= 1;

p= strrchr(organism_file,'/'); ++p;
strcpy(Output_name, p);
strcpy(Output_name + 100, p);
strcpy(Output_name + 2 * 100, p);
strcpy(Output_name + 3 * 100, p);
strcpy(Output_name + 4 * 100, p);
strcpy(Output_name + 5 * 100, p);
strcpy(Output_name + 6 * 100, p);
strcpy(Output_name + 7 * 100, p);
strcpy(Output_name + 8 * 100, p);
strcpy(Output_name + strlen(Output_name) - 4, ".modified");
strcpy(Output_name + 100 + strlen(Output_name + 100) - 4, ".newcds");
strcpy(Output_name +  2 * 100 + strlen(Output_name + 2 * 100) - 4, ".profiles");
strcpy(Output_name +  3 * 100 + strlen(Output_name + 3 * 100) - 4, ".verbose");
strcpy(Output_name +  4 * 100 + strlen(Output_name + 4 * 100) - 4, ".altcds");
strcpy(Output_name +  5 * 100 + strlen(Output_name + 5 * 100) - 4, ".repetitive");
strcpy(Output_name +  6 * 100 + strlen(Output_name + 6 * 100) - 4, ".confirmed");
strcpy(Output_name +  7 * 100 + strlen(Output_name + 7 * 100) - 4, ".ffn");
strcpy(Output_name +  8 * 100 + strlen(Output_name + 8 * 100) - 4, ".faa");

	if(argc == 3) SIGNIFICANCE= atof(argv[2]);
	if(argc == 4)
	{
	NAME_OFFSET= strlen(argv[3]);
	strcpy(common_name, argv[3]);
	}

hss= (struct HSSs *)malloc(sizeof(struct HSSs));
gene= (struct exons *)malloc(sizeof(struct exons));

sprintf(organism_file, "%s", organism_file);

	if((fp = fopen(organism_file, "r")) == NULL) { fprintf(stderr, "fp file %s returns NULL\n", organism_file); exit(1); }

bytes_from_origin= annotation(&ncds, &nexons);		// Reads annotated CDSs and records start-of-sequence position in the file

fprintf(stderr, "\nNum CDS: %d\nNum exons: %d\n", ncds, nexons);

ffrom= (long *)malloc(nexons * sizeof(long));
o= (int *)malloc(nexons * sizeof(int));

assign_gene_positions(ffrom, o, ncds, nexons);

genome_size= genome_composition(tnuc, ffrom, o, nexons);	// Determines sequence length and composition
search_size= genome_size;
fseek(fp, bytes_from_origin, SEEK_SET);				// Resets file pointer to start of sequence


fprintf(stdout,"Genome: ~/%s\nGenome size: %d\nUnknown bases: %d\nNumber of CDS annotated in file: %d (%d exons).",argv[1],genome_size, ttinvalid_base, ncds, nexons);
	if(p= strrchr(argv[1], '/')) ++p;
	else p= argv[1];

fprintf(stderr,"\n%s", p);
fprintf(stdout,"\nComposition: A: %.2f%%, C: %.2f%%, G: %.2f%%, T: %.2f%%.\n",tnuc[0]*100.0/tnuc[4],tnuc[1]*100.0/tnuc[4],tnuc[2]*100.0/tnuc[4],tnuc[3]*100.0/tnuc[4]);
fprintf(stdout,"\nSignificance level: %.4f\n",SIGNIFICANCE);
fprintf(stderr,"\nAnalyzing genome reading frames:     ");

/* Finds and tests orfs in all 6 frames */

	for(k= 0; k < 6; ++k)
	{
	orf_num[k]= (int **)malloc(sizeof(int *));
	orf_num[k][0]= (int *)malloc(3 * sizeof(int));
	}


tot_hss= analyze_genome(genome_size, tot_hss, &on);
fseek(fp, bytes_from_origin, SEEK_SET);

/************/
		
fprintf(stdout,"\nTotal HSS: %d\n",tot_hss);
// for(i= 0; i < tot_hss; ++i) fprintf(stderr,"%5d.\t%d\t%d\t%c%d\n",i+1,hss[i].fromp,hss[i].top,hss[i].strand,hss[i].frame);

rescue_hss(tot_hss);                                    // Rescues excluded HSSs 

renumber_orf_hits();

t1= time(NULL);
minutes= (int)(t1 - t0) / 60;
secs= ((int)(t1 - t0)) % 60;

fprintf(stdout, "\nScoring time: %d\' %d\"\n", minutes, secs);
fprintf(stderr,"\nChecking for other compositional asymmetries (G-tests):     ");

tot_Ghits= find_Ghits(genome_size, tot_hss, bytes_from_origin, &on);     // G-tests
fseek(fp, bytes_from_origin, SEEK_SET);

t2= time(NULL);
minutes= (int)(t2 - t1) / 60;
secs= ((int)(t2 - t1)) % 60;
fprintf(stdout, "\n\nG-test time: %d\' %d\"\n", minutes, secs);

fprintf(stderr,"\nCharacterizing %d published genes:     ",ncds);
characterize_published_genes(ncds, tot_Ghits, nuc, bytes_from_origin, ffrom);   // List published genes characterized by content annotation

process_hss(tot_hss, tot_Ghits, ncds);     // Characterizes genes predicted by G-test
write_results(tot_hss, tot_Ghits, ncds, genome_size);     // Writes all results

// check_lists();

fclose(fp);
free(o);
free(hss);
free(gene);


fprintf(stdout, "\nTotal hss scored: %d\n\n", tot_Ghits);
t2= time(NULL);
minutes= (int)(t2 - t0) / 60;
secs= ((int)(t2 - t0)) % 60;
fprintf(stdout, "Total run time: %d\' %d\"\n\n", minutes, secs);
}

/************ END OF MAIN ************/



/*********************************/
/**** Function get_sequence() ****/
/*********************************/

// Requires global FILE *fp

void get_sequence(int from, int len, char strand, char *ORF)
{
int	i= 0, j;
char	c;

	while(i < from - 1)
	{
	c= fgetc(fp);
		if(c >= 'A' && c <= 'Z') c += 'a' - 'A';
		if (c >= 'a' && c <= 'z') ++i;
	}

	i= 0;
	while(i < len)
	{
	c= fgetc(fp);
		if(c >= 'A' && c <= 'Z') c += 'a' - 'A';
		if (c >= 'a' && c <= 'z')
		{
                        if(numbers[c - 'a'] == -1)    // TEMPORARY FIX FOR NON-STANDARD BASE
                        {
                                if(strand == 'D') c= 'c';
                                else              c= 'g';
                        }

		ORF[i++] = numbers[c - 'a'];
		}
	}

	if(strand == 'C')
	{
		for(i= 0; i < len; ++i) ORF[i]= 3 - ORF[i];
		for(i= 0; i < len / 2; ++i) { j= ORF[i]; ORF[i]= ORF[len - 1 - i]; ORF[len - 1 - i]= j; }
	}

ftell(fp);
}

// End of function get_sequence()

/***************************************/
/**** Function genome_composition() ****/
/***************************************/

int genome_composition(double *tnuc, long *ffrom, int *o, int k)
{
char	c;
int	i= 0, j, n;

	while((c= fgetc(fp)) && !feof(fp))
	{
		if(c >= 'A' && c <= 'Z') c += 'a' - 'A';
		if (c >= 'a' && c <= 'z')
		{
			if(c != 'a' && c != 'c' && c != 'g' && c != 't')
			{
			c= 'c'; //  TEMPORARY FIX FOR INVALID BASE
			++ttinvalid_base;
			}

			while(i < k && (int)ffrom[o[i]] == tnuc[4]) { ffrom[o[i]]= ftell(fp); ++i; }

		++tnuc[numbers[c-'a']];
		++tnuc[4];
		}
	}

n= (int)(tnuc[4] + 0.5);    // 0.5 added to double tnuc[4] to avoid flooring of possible approximation by defect of double

return(n);
}

// End of function genome_composition()

/***********************************/
/**** Function analyze_genome() ****/
/***********************************/

// Requires global FILE *fp

int analyze_genome(int genome_size, int tot_hss, int *on) 
{
int	pos= 0, j, k, t, f, codon, h, th, i, len[6]= {0}, first_stop[6], second_stop[6],frame;
char	*p, c, s;

	while(pos < 1)
	{
	c= fgetc(fp);
		if(c >= 'A' && c <= 'Z') c += 'a' - 'A';
		if(c >= 'a' && c <= 'z') ++pos;
	}
	if(c != 'a' && c != 'c' && c != 'g' && c != 't') c= 'c'; //  TEMPORARY FIX FOR INVALID BASE

ORF[2*MAX_ORF_SIZE]= numbers[c - 'a']; 
ORF[3*MAX_ORF_SIZE]= 3 - numbers[c - 'a'];

	while(pos < 2)
	{
	c= fgetc(fp);
		if(c >= 'A' && c <= 'Z') c += 'a' - 'A';
		if(c >= 'a' && c <= 'z') ++pos;
	}
	if(c != 'a' && c != 'c' && c != 'g' && c != 't') c= 'c'; //  TEMPORARY FIX FOR INVALID BASE

ORF[2*MAX_ORF_SIZE + 1]= numbers[c - 'a'];
ORF[3*MAX_ORF_SIZE + 1]= 3 - numbers[c - 'a'];

ORF[0]= numbers[c - 'a'];
ORF[4*MAX_ORF_SIZE]= 3 - numbers[c - 'a'];

len[2]= len[3]= 2;
len[0]= len[4]= 1;

first_stop[0]= 1;
first_stop[1]= 2;
first_stop[2]= 0;
first_stop[3]= 0;
first_stop[4]= 1;
first_stop[5]= 2;

	while (pos < genome_size)
	{
	c= fgetc(fp);
		if(c >= 'A' && c <= 'Z') c += 'a' - 'A';
		if (c >= 'a' && c <= 'z')
		{
		++pos;
		fprintf(stderr,"\b\b\b\b%3d%%",(int)((double)pos / ((double)genome_size) * 100.0 + 0.5));

			for(k= 0; k < 3; ++k)
			{
				if(c != 'a' && c != 'c' && c != 'g' && c != 't') c= 'c'; //  TEMPORARY FIX FOR INVALID BASE
			ORF[k * MAX_ORF_SIZE + len[k]++] = numbers[c - 'a'];
			}
			for(k= 3; k < 6; ++k)
			{
				if(c != 'a' && c != 'c' && c != 'g' && c != 't') c= 'g'; //  TEMPORARY FIX FOR INVALID BASE
			ORF[k*MAX_ORF_SIZE + len[k]++] = 3 - numbers[c - 'a'];
			}

// DIRECT STRAND:

		frame= (pos - 1) % 3;

		codon= ORF[frame * MAX_ORF_SIZE + len[frame]-3] * 16 + ORF[frame * MAX_ORF_SIZE + len[frame] - 2] * 4 + ORF[frame * MAX_ORF_SIZE + len[frame] - 1];

			if(codon == 48 || codon == 50 || (codon == 56 && !MYCOPLASMA) || pos >= genome_size - 2)
			{

				if(codon == 48 || codon == 50 || (codon == 56 && !MYCOPLASMA))
				{
				len[frame] -= 3;
				second_stop[frame]= pos - 4;
				}
				else second_stop[frame]= pos - 1;

				if(len[frame] >= mHL)
				{
				orf_num[frame]= (int **)realloc(orf_num[frame],(tot_orfs[frame] + 1) * sizeof(int *));
				orf_num[frame][tot_orfs[frame]]= (int *)malloc(3 * sizeof(int));
				orf_num[frame][tot_orfs[frame]][0]= first_stop[frame];
				orf_num[frame][tot_orfs[frame]][1]= second_stop[frame];
	
					if(RANDOMIZE) shuffle(ORF + frame*MAX_ORF_SIZE, len[frame], 1);

					if(REPETITIONS) th= score_orf_random(ORF + frame * MAX_ORF_SIZE, len[frame], tot_hss);	// SCORING SEGMENT BY SIMULATIONS
					else            th= score_orf_table(ORF + frame * MAX_ORF_SIZE, len[frame], tot_hss);	// SCORING SEGMENT USING TABLE OF THRESHOLDS

					for(h= 0; h < th; ++h)
					{
					hss[tot_hss + h].strand= strnd[0];
					hss[tot_hss + h].frame= frame;
					hss[tot_hss + h].hit_type= 'H';
					hss[tot_hss + h].hsuper= 0;
					hss[tot_hss + h].gsuper= 0;
					hss[tot_hss + h].stop1= first_stop[frame];
					hss[tot_hss + h].stop2= second_stop[frame];
					hss[tot_hss + h].orf_num= tot_orfs[frame];
					hss[tot_hss + h].hit_num= th - h;
						if(h) hss[tot_hss + h].next_hit= tot_hss + h - 1;
						else  hss[tot_hss + h].next_hit= -1;
						if(h < th - 1) hss[tot_hss + h].previous_hit= tot_hss + h + 1;
						else  hss[tot_hss + h].previous_hit= -1;
					composition(ORF + frame * MAX_ORF_SIZE + hss[tot_hss + h].fromp, hss[tot_hss + h].top - hss[tot_hss + h].fromp + 1, hss[tot_hss + h].nuc, 0);
					hss[tot_hss + h].G= G_test(ORF + frame * MAX_ORF_SIZE + hss[tot_hss + h].fromp, hss[tot_hss + h].top - hss[tot_hss + h].fromp + 1, hss[tot_hss + h].pstring, &i);

					hss[tot_hss + h].fromp += hss[tot_hss + h].stop1;
						if(hss[tot_hss + h].start == 1 || hss[tot_hss + h].start == 3) hss[tot_hss + h].atg += hss[tot_hss + h].stop1;
						else if(hss[tot_hss + h].start == 2 || hss[tot_hss + h].start == 4) hss[tot_hss + h].gtg += hss[tot_hss + h].stop1;
						else if(hss[tot_hss + h].start == 5 || hss[tot_hss + h].start == 6) hss[tot_hss + h].ttg += hss[tot_hss + h].stop1;
					hss[tot_hss + h].top +=   hss[tot_hss + h].stop1;
					hss[tot_hss + h].len= hss[tot_hss + h].top - hss[tot_hss + h].fromp + 1;
					}

					if(th) orf_num[frame][tot_orfs[frame]][2]= tot_hss + th - 1;
					else   orf_num[frame][tot_orfs[frame]][2]= -1;

				++tot_orfs[frame];
				tot_hss += th;
				}
			len[frame]= 0;
		  	first_stop[frame]= second_stop[frame] + 4;
			}

// COMPLEMENTARY STRAND:

		  frame= pos % 3 + 3;

		  codon= ORF[frame*MAX_ORF_SIZE + len[frame]-3] + ORF[frame*MAX_ORF_SIZE + len[frame]-2]*4 + ORF[frame*MAX_ORF_SIZE + len[frame]-1]*16;

			if(codon == 48 || codon == 50 || (codon == 56 && !MYCOPLASMA) || pos >= genome_size - 2)
			{
				if(codon == 48 || codon == 50 || (codon == 56 && !MYCOPLASMA))
				{
				len[frame] -= 3;
				second_stop[frame]= pos - 4;
				}
				else second_stop[frame]= pos - 1;


				if(len[frame] >= mHL)
				{
				orf_num[frame]= (int **)realloc(orf_num[frame],(tot_orfs[frame] + 1) * sizeof(int *));
				orf_num[frame][tot_orfs[frame]]= (int *)malloc(3 * sizeof(int *));
				orf_num[frame][tot_orfs[frame]][0]= first_stop[frame];
				orf_num[frame][tot_orfs[frame]][1]= second_stop[frame];
	
				invert_sequence(ORF + frame * MAX_ORF_SIZE, len[frame]);   // strand = 'C'

					if(RANDOMIZE) shuffle(ORF + frame*MAX_ORF_SIZE, len[frame], 1);

					if(REPETITIONS) th= score_orf_random(ORF + frame * MAX_ORF_SIZE, len[frame], tot_hss);	// SCORING SEGMENT
					else            th= score_orf_table(ORF + frame * MAX_ORF_SIZE, len[frame], tot_hss);	// SCORING SEGMENT

					for(h= 0; h < th; ++h)
					{
					hss[tot_hss + h].strand= strnd[1];
					hss[tot_hss + h].frame= frame;
					hss[tot_hss + h].hit_type= 'H';
					hss[tot_hss + h].hsuper= 0;
					hss[tot_hss + h].gsuper= 0;
					hss[tot_hss + h].stop1= first_stop[frame];
					hss[tot_hss + h].stop2= second_stop[frame];
					hss[tot_hss + h].orf_num= tot_orfs[frame];
					hss[tot_hss + h].hit_num= th - h;
						if(h) hss[tot_hss + h].next_hit= tot_hss + h - 1;
						else  hss[tot_hss + h].next_hit= -1;
						if(h < th - 1) hss[tot_hss + h].previous_hit= tot_hss + h + 1;
						else  hss[tot_hss + h].previous_hit= -1;
					composition(ORF + frame*MAX_ORF_SIZE + hss[tot_hss + h].fromp, hss[tot_hss + h].top - hss[tot_hss + h].fromp + 1, hss[tot_hss + h].nuc, 0);
					hss[tot_hss + h].G= G_test(ORF + frame*MAX_ORF_SIZE + hss[tot_hss + h].fromp, hss[tot_hss + h].top - hss[tot_hss + h].fromp + 1,hss[tot_hss + h].pstring, &i);
						
					f= hss[tot_hss + h].fromp;
					hss[tot_hss + h].fromp = len[frame] - 1 - hss[tot_hss + h].top + hss[tot_hss + h].stop1;
					hss[tot_hss + h].top = len[frame] - 1 - f + hss[tot_hss + h].stop1;
					hss[tot_hss + h].len= hss[tot_hss + h].top - hss[tot_hss + h].fromp + 1;
						if(hss[tot_hss + h].start == 1 || hss[tot_hss + h].start == 3)
						{
						f= hss[tot_hss + h].atg;
						hss[tot_hss + h].atg = len[frame] - 1 - f + hss[tot_hss + h].stop1;
						}
						else if(hss[tot_hss + h].start == 2 || hss[tot_hss + h].start == 4)
						{
						f= hss[tot_hss + h].gtg;
						hss[tot_hss + h].gtg = len[frame] - 1 - f + hss[tot_hss + h].stop1;
						}
						else if(hss[tot_hss + h].start == 5 || hss[tot_hss + h].start == 6)
						{
						f= hss[tot_hss + h].ttg;
						hss[tot_hss + h].ttg = len[frame] - 1 - f + hss[tot_hss + h].stop1;
						}
					}

					if(th) orf_num[frame][tot_orfs[frame]][2]= tot_hss + th - 1;
					else   orf_num[frame][tot_orfs[frame]][2]= -1;
				++tot_orfs[frame];
				tot_hss += th;
				}
			len[frame]= 0;
		  	first_stop[frame]= second_stop[frame] + 4;
			}
		}
	}

fprintf(stderr,"\b\b\b\b100%%\n");
fprintf(stdout,"\nTotal # orfs (len >= %d): %d\n", mHL, tot_orfs[0] + tot_orfs[1] + tot_orfs[2] + tot_orfs[3] + tot_orfs[4] + tot_orfs[5]);

return(tot_hss);
}	

/*** End of function analyze_genome() ***/

/*************************/
/**** Function sort() ****/
/*************************/

void sort(double * array, int size)
{
double temp;
int i, j;

	for(i = 0; i < size - 1; ++i)
		for(j = i + 1; j < size; ++j)
        if(array[j] < array[i])
		{
			temp = array[i];
            array[i] = array[j];
            array[j] = temp;
		}
}

// End of function sort()

/***************************************/
/*** Function score_orf_random() ***/
/***************************************/

// Requires global double sc[6 * 64]

int score_orf_random(char *orf, int n, int tot_hss)
{
int i, j = 0, k, nuc[4]= {0}, downthr_pos, rep, sig_pos, from, to, last_zero= n, from_pos, to_pos= n - 1;
char *seq;
double r, Score = 0.0, maxscore= 0.0;
double maxrscore[REPETITIONS + 1] = {0.0};

build_scores(orf, n, sc);

rep = REPETITIONS;

	seq = (char *) malloc(n * sizeof(char));

		for(i = 0; i < n; i++) seq[i]= orf[i];

// Shuffles (removing stop codons) and scores randomized sequences:

		for(k = 0; k < rep; k++)
		{
		shuffle(seq, n, 1);
		maxrscore[k]= score(seq, n, sc, &from, &to, 1);
		}

// Sorts scores of randomized sequences:

	sort(maxrscore, rep);

	free(seq);

	sig_pos = (rep * (1.0 -SIGNIFICANCE));
	downthr_pos = (rep * (1.0 - DOWNTHR));
	
// Finds HSSs in ORF:

	for(i = n - 3; i >= 0; i -= 3)
	{
		Score += sc[16*orf[i] + 4*orf[i+1] + orf[i+2]];    //  Score += sc[0][16*orf[i] + 4*orf[i+1] + orf[i+2]];

		if(Score < 0.0 || maxscore - Score >= maxrscore[downthr_pos])
			Score = 0.0;
		
		if(Score && Score > maxscore)
		{
			maxscore = Score;
			from_pos= i;
		}

                if(i==0) Score= 0;
		
		if (Score == 0.0)
		{
			if (maxscore >= maxrscore[sig_pos] && to_pos - from_pos + 1 >= mHL)
			{
				to_pos= last_zero - 1;
                                hss= (struct HSSs *)realloc(hss,(tot_hss+j+1)*sizeof(struct HSSs));
                                hss[tot_hss+j].fromp= from_pos;
                                hss[tot_hss+j].top= to_pos;
                                hss[tot_hss+j].score= maxscore;
                                k= 0;
                                  do ++k; while((sig_pos+k) < rep && maxscore >= maxrscore[sig_pos+k]);
                                hss[tot_hss+j].prob= 1.0 - (float)(sig_pos+k-1)/((float)rep);
                                hss[tot_hss+j].type= 5;

// Checks HSS against alternative reading frames:

				from= from_pos;
				to= to_pos;

                                for(k=1;k<=5;++k)
                                {
                                	r= score(orf+from, to-from+1, sc + k*64, &from, &to, 1);    //  r= score(orf+from,to-from+1,sc[k],&from,&to);
					from += from_pos;
					to += from_pos;
					from_pos= from;
				}

                                maxscore= score(orf+from, to-from+1, sc, &from, &to, 1);    //  maxscore_array[j]= score(orf+from,to-from+1,sc[0],&from,&to);
				from += from_pos;
				to += from_pos;
				from_pos= from;
                                to_pos= to;
/**/

				if (maxscore >= maxrscore[sig_pos])
				{
				hss[tot_hss+j].fromp= from_pos;
				hss[tot_hss+j].top= to_pos;
				hss[tot_hss+j].score= maxscore;
				hss[tot_hss+j].type= 0;
				k= 0;
                                  do ++k; while((sig_pos+k) < rep && maxscore >= maxrscore[sig_pos+k]);
                                hss[tot_hss+j].prob= 1.0 - (float)(sig_pos+k-1)/((float)rep);
				}
			++j;
			}
			last_zero = i;
			to_pos= i - 1;
			maxscore= 0.0;
		}
	}

return(j);         // Returns the number of HSSs found.
}

// End of function score_orf_random()

/**************************************/
/*** Function score_orf_table() ***/
/**************************************/

// Requires global double sc[6 * 64]

int score_orf_table(char *orf, int n, int tot_hss)
{
int i, j, h, k, l, t, nuc[4]= {0}, from, to, last_zero= n, from_pos, to_pos= n - 1, nt, flag;
float	a[3], b[3], f;
char *seq;
double r, Score = 0.0, maxscore= 0.0, max_len;
double down_thr, thr[3], X;

max_len= pow(2.0, 1023.0);
	for(k= 0; k < LEN_TRANSFORM; ++k) max_len= log(max_len);

build_scores(orf, n, sc);

		for(i= 0; i < n; ++i) ++nuc[orf[i]];
nuc[3]= 50;
		for(i= 0; i < 3; ++i)
		{
		nuc[i]= (int)( (double)nuc[i] / (double)n * 50.0 + 0.5 );
		nuc[3] -= nuc[i];
		}

	i= position(nuc[0], nuc[1], nuc[2], nuc[3]);

	a[0]= threshold[0][i][0];
	b[0]= threshold[0][i][1];

	a[1]= threshold[1][i][0];
	b[1]= threshold[1][i][1];

	a[2]= threshold[2][i][0];
	b[2]= threshold[2][i][1];

		if(LEN_TRANSFORM == 0) X= (double)n; 
		else if(LEN_TRANSFORM == 1) X= log((double)n); 
		else if(LEN_TRANSFORM == 2) X= log(log((double)n)); 
		else if(LEN_TRANSFORM == 3) X= log(log(log((double)n))); 
		else { fprintf(stderr,"\n\nLEN_TRANSFORM can only be defined as 0= len, 1= ln(len), 2= ln(ln(len)), or 3= ln(ln(ln(len)))\n"); exit(1); }

	thr[0]= a[0] * X + b[0];
	thr[1]= a[1] * X + b[1];
	thr[2]= a[2] * X + b[2];

// fprintf(stderr,"\n%d %d %d %d %d   %.3f %.3f %.3f\n",n,nuc[0], nuc[1], nuc[2], nuc[3], thr[0],thr[1],thr[2]);

	down_thr= thr[0];

		if(SIGNIFICANCE == 0.01) t= 0;
		else if(SIGNIFICANCE == 0.001) t= 1;
		else if(SIGNIFICANCE == 0.0001) t= 2;
		else { fprintf(stderr,"\n\nWhen using tables significance values can only be 0.01, 0.001, or 0.0001.\n"); exit(1); }
	
// Finds HSSs in ORF:

	last_zero = n;
	Score = 0.0;
	h = 0;

	for(i = n - 3; i >= 0; i -= 3)
	{
	Score += sc[16*orf[i] + 4*orf[i+1] + orf[i+2]];    //  Score += sc[0][16*orf[i] + 4*orf[i+1] + orf[i+2]];

		if(Score < 0.0 || maxscore - Score >= down_thr)
			Score = 0.0;
		
		if(Score && Score > maxscore)
		{
			maxscore= Score;
			from_pos= i;
		}

                if(i==0) Score= 0.0;
		
		if (Score == 0.0)
		{
			if (maxscore >= thr[t] && to_pos - from_pos + 1 >= mHL && thr[t] > 0.0)
			{
				to_pos= last_zero - 1;
                                hss= (struct HSSs *)realloc(hss, (tot_hss + h + 1) * sizeof(struct HSSs));
                                hss[tot_hss + h].fromp= from_pos;
                                hss[tot_hss + h].top= to_pos;
                                hss[tot_hss + h].score= maxscore;
                                	for(k= t; k < 3 && maxscore >= thr[k]; ++k);
				--k;
                                	if(k == 0) hss[tot_hss + h].prob= 0.01;
                                	else if(k == 1) hss[tot_hss + h].prob= 0.001;
                                	else if(k == 2) hss[tot_hss + h].prob= 0.0001;
                                hss[tot_hss+h].type= 5;

// fprintf(stderr,"\nSEGMENT FOUND (len: %d; Score: %.3f). ", to_pos - from_pos + 1, maxscore);

// Checks HSS against alternative reading frames:

				from= from_pos;
				to= to_pos;

                                for(k= 1; k <= 5; ++k)
                                {
                                	r= score(orf + from, to - from + 1, sc + k*64, &from, &to, 1);    //  r= score(orf+from,to-from+1,sc[k],&from,&to);
					from += from_pos;
					to += from_pos;
					from_pos= from;
				}

                                maxscore= score(orf + from, to - from + 1, sc, &from, &to, 1);    //  maxscore_array[j]= score(orf+from,to-from+1,sc[0],&from,&to);
				from += from_pos;
				to += from_pos;
				from_pos= from;
                                to_pos= to;

				hss[tot_hss + h].entropy= entropy(orf + from_pos, to_pos - from_pos + 1);

				hss[tot_hss + h].fromp= from_pos;
				hss[tot_hss + h].top= to_pos;
				hss[tot_hss + h].score= maxscore;

				flag= 0;

					for(k= hss[tot_hss + h].fromp; k >= 0 && k >=hss[tot_hss + h].fromp - ATG_POS5; k -= 3)
					{
					nt= 16 * orf[k] + 4 * orf[k + 1] + orf[k + 2];
						if(nt == 14) { hss[tot_hss + h].atg= k; k= 0; flag= 1; }
					}

					if(!flag)
						for(k= hss[tot_hss + h].fromp; k < hss[tot_hss + h].top && k <= hss[tot_hss + h].fromp + ATG_POS3; k += 3)
						{
						nt= 16 * orf[k] + 4 * orf[k + 1] + orf[k + 2];
							if(nt == 14) { hss[tot_hss + h].atg= k; k= hss[tot_hss + h].top; flag= 1; }
						}

					if(!flag)
						for(k= hss[tot_hss + h].fromp; k >= 0 && k >= hss[tot_hss + h].fromp - GTG_POS5; k -= 3)
						{
						nt= 16 * orf[k] + 4 * orf[k + 1] + orf[k + 2];
							if(nt == 46) { hss[tot_hss + h].gtg= k; k= 0; flag= 2; }
						}

					if(!flag)
						for(k= hss[tot_hss + h].fromp; k < hss[tot_hss + h].top && k <= hss[tot_hss + h].fromp + GTG_POS3; k += 3)
						{
						nt= 16 * orf[k] + 4 * orf[k + 1] + orf[k + 2];
							if(nt == 46) { hss[tot_hss + h].gtg= k; k= hss[tot_hss + h].top; flag= 2; }
						}

					if(!flag)
						for(k= hss[tot_hss + h].fromp - ATG_POS5 - 3; k >= 0; k -= 3)
						{
						nt= 16 * orf[k] + 4 * orf[k + 1] + orf[k + 2];
							if(nt == 14) { hss[tot_hss + h].atg= k; k= 0; flag= 3; }
							else if(nt == 46) { hss[tot_hss + h].gtg= k; k= 0; flag= 4; }
						}

					if(!flag)
						for(k= hss[tot_hss + h].fromp; k >= 0 && k >= hss[tot_hss + h].fromp - TTG_POS5; k -= 3)
						{
						nt= 16 * orf[k] + 4 * orf[k + 1] + orf[k + 2];
							if(nt == 62) { hss[tot_hss + h].ttg= k; k= 0; flag= 5; }
						}

					if(!flag)
						for(k= hss[tot_hss + h].fromp; k < hss[tot_hss + h].top && k <= hss[tot_hss + h].fromp + TTG_POS3; k += 3)
						{
						nt= 16 * orf[k] + 4 * orf[k + 1] + orf[k + 2];
							if(nt == 62) { hss[tot_hss + h].ttg= k; k= hss[tot_hss + h].top; flag= 5; }
						}

					if(!flag)
						for(k= hss[tot_hss + h].fromp - TTG_POS5 - 3; k >= 0; k -= 3)
						{
						nt= 16 * orf[k] + 4 * orf[k + 1] + orf[k + 2];
							if(nt == 62) { hss[tot_hss + h].ttg= k; k= 0; flag= 6; }
						}

				hss[tot_hss + h].start= flag;


					if(WRITE_SEQUENCES)
					{
						if(flag == 1 || flag == 3)
						{
						nt= n - hss[tot_hss + h].atg + 2;
						hss[tot_hss + h].seqlen= nt - 1;
						hss[tot_hss + h].seq= (char *)malloc(nt * sizeof(char));
							for(j= 0; j < nt - 1; ++j) hss[tot_hss + h].seq[j]= orf[hss[tot_hss + h].atg + j];
						}
						else if(flag == 2 || flag == 4)
						{
						nt= n - hss[tot_hss + h].gtg + 2;
						hss[tot_hss + h].seqlen= nt - 1;
						hss[tot_hss + h].seq= (char *)malloc(nt * sizeof(char));
						strncpy(hss[tot_hss + h].seq, orf + hss[tot_hss + h].gtg, nt - 1);
						}
						else
						{
						nt= n - hss[tot_hss + h].fromp + 2;
						hss[tot_hss + h].seqlen= nt - 1;
						hss[tot_hss + h].seq= (char *)malloc(nt * sizeof(char));
						strncpy(hss[tot_hss + h].seq, orf + hss[tot_hss + h].fromp, n);
						}
					}

				hss[tot_hss + h].sig_len= (maxscore - (double)b[0]) / ((double)a[0]); 
					if(hss[tot_hss + h].sig_len < max_len)
						for(k= 0; k < LEN_TRANSFORM; ++k) hss[tot_hss + h].sig_len= exp(hss[tot_hss + h].sig_len);
					else hss[tot_hss + h].sig_len= pow(2.0, 1023.0);

                                	for(k= t; k < 3 && maxscore >= thr[k]; ++k);
				--k;
                                	if(k == 0) hss[tot_hss + h].prob= 0.01;
                                	else if(k == 1) hss[tot_hss + h].prob= 0.001;
                                	else if(k == 2) hss[tot_hss + h].prob= 0.0001;


				if (to - from + 1 >= mHL)
				{
					if (maxscore >= thr[t] && thr[t] > 0.0) { hss[tot_hss + h].type= 0; }
				++h;
				}
			}
			last_zero = i;
			to_pos= i - 1;
			maxscore= 0.0;
		}
	}

return(h);         // Returns the number of HSSs found.
}

// End of function score_orf_table()

/**************************/
/**** Function entropy() **/
/**************************/

double entropy(char *seq, int n)
{
	int i, j, k, l, codon, total_codon= 0;
	double codon_arr[64]= {0.0}, entropy= 0.0, D;

	if(MYCOPLASMA) D= 62.0;
	else           D= 61.0;
		
	for(i= 0; i < n - n % 3; i += 3)
	{
		if((codon= get_codon(seq + i)) < 64 && codon != 48 && codon != 50 && (codon != 56 || MYCOPLASMA))
		{
		++codon_arr[codon];
		++total_codon;
		}
	}

	if(total_codon)
	{
		for(k= 0; k < 64; k++) if(codon_arr[k]) codon_arr[k] /= (double)total_codon;
	}
	else return(-1.0);

	for(k= 0; k < 64; k++)
		if(codon_arr[k] > 0.0 && codon_arr[k] < 1.0)
			entropy -= codon_arr[k] * log(codon_arr[k]);

entropy /= log(D);
	
return(entropy);
}

// End of function entropy()

/**************************/
/**** Function get_codon() /
/**************************/

int get_codon(char buff[])
{
	
int	codon= 0, i, power[] = { 16, 4, 1 };
	
	for(i = 0; i < 3; ++i)
	{
		if(buff[i] < 0 || buff[i] > 3) return(64);
		else codon += power[i] * buff[i];
	}
return(codon);
}

// End of function get_codon()

/**************************/
/**** Function score() ****/
/**************************/

double score(char *seq,int n,double *sc,int *from,int *to, int flag)
{
int	i, last_zero= n, cod;
double	Score= 0.0, maxscore= 0.0;

to[0]= from[0]= n-1;

        for(i = n - 3; i >= 0; i -= 3)
        {
	cod= 16*seq[i] + 4*seq[i+1] + seq[i+2];

	Score += sc[cod];

                if(Score < 0.0 || cod == 48 || cod == 50 || (cod == 56 && !MYCOPLASMA) )
                        if(flag) Score = 0.0;

                if(Score && Score >= maxscore)
                {
                        maxscore= Score;
                        from[0]= i;
			to[0] = last_zero - 1;
                }
                if(i==0 && flag) Score= 0.0;

		if (Score == 0.0) last_zero = i;
        }

	if(flag) Score= maxscore;

return(Score);
}

// End of function score()

/*******************************/
/**** Function annotation() ****/
/*******************************/

long annotation(int *ncds, int *nexons)
{
int     n= 0, i, j, k, len= 0, lcds, flag= 0;
char    longstr[500], *cds, *p, *p1, *p2;

cds= (char *)malloc(sizeof(char));

	while(fgets(longstr, 498, fp) && strncmp(longstr, "ORIGIN", 6) && !feof(fp))
	{
		if(!strncmp(longstr,"     CDS",8))
		{
		flag= 1;
		lcds= strlen(longstr + 21) - 1;
		cds= (char *)realloc(cds, (lcds + 2) * sizeof(char));
		strcpy(cds, longstr + 21);
		cds[strlen(cds) - 1]= '\0';
			while(cds[strlen(cds) - 1] == ',')
			{
			fgets(longstr, 498, fp);
			lcds += strlen(longstr + 21) - 1;
			cds= (char *)realloc(cds, (lcds + 2) * sizeof(char));
			strcat(cds, longstr + 21);
			cds[strlen(cds) - 1]= '\0';
			}

			if(p=strstr(cds, "join")) p += 5;
			else if(strstr(cds,"complement")) { p= strchr(cds,'('); ++p; }
			else p= cds;
		
		p= strrchr(cds,'.'); ++p;
		
		gene= (struct exons *)realloc(gene,(n+1)*sizeof(struct exons));
		
			if(strstr(cds,"complement")) gene[n].strand= 'C';
			else gene[n].strand= 'D';
		
		gene[n].num_exons= 1;
		gene[n].len= 0;
		strcpy(gene[n].name, "");
		p= cds;
			while(p= strchr(p, ',')) { gene[n].num_exons += 1; ++p; }
		
		gene[n].start= (int *)malloc(gene[n].num_exons*sizeof(int));
		gene[n].newstart= (int *)malloc(gene[n].num_exons*sizeof(int));
		gene[n].end= (int *)malloc(gene[n].num_exons*sizeof(int));
		gene[n].newend= (int *)malloc(gene[n].num_exons*sizeof(int));
		gene[n].fpos= (int *)malloc(gene[n].num_exons*sizeof(int));
		gene[n].type= (int *)malloc(gene[n].num_exons*sizeof(int));
		gene[n].color= (char *)malloc(gene[n].num_exons*sizeof(char));
		gene[n].frame= (int *)malloc(gene[n].num_exons*sizeof(int));
		
		p= cds;
		gene[n].len= 0;
		
			if(gene[n].strand == 'D')
			{
		
				for(i= 0; i < gene[n].num_exons; ++i)
				{
					while(p[0]<'0' || p[0]>'9')
					{
					++p;
					}
				gene[n].start[i]= gene[n].newstart[i]= atoi(p) - 1;
				p1= strchr(p,',');
				p2= strchr(p,'.');
					if((p1 && p2 && p1<p2) || !p2) gene[n].end[i]= gene[n].start[i];
					else
					{
					p= strstr(p,".."); p += 2;
						while(p[0]<'0' || p[0]>'9') ++p;
					gene[n].end[i]= gene[n].newend[i]= atoi(p) - 1;
					}
				p= strchr(p,','); ++p;
				gene[n].color[i]= RGB[ ( gene[n].start[i] - gene[n].len % 3 + 2 ) % 3 ];
				gene[n].frame[i]= ( gene[n].start[i] - gene[n].len % 3 + 2 ) % 3;
				gene[n].len += gene[n].end[i] - gene[n].start[i] + 1;
				++nexons[0];
				}
		
			gene[n].end[i-1] -= 3;
			gene[n].to= gene[n].end[i-1];
			gene[n].from= gene[n].start[0];
			gene[n].span= gene[n].to - gene[n].from + 1;
			}
			else if(gene[n].strand == 'C')
			{
			p += strlen(cds) - 1;
				for(i= 0; i < gene[n].num_exons; ++i)
				{
					while(p[0] != '.' && p[0] != ',' && p[0] != '(') --p;
				p1= p;
				++p;
					while(p[0] < '0' || p[0] > '9') ++p;

				gene[n].start[i]= gene[n].newstart[i]= atoi(p) - 1; --p;
					if(p1[0] != '.')
					{
					gene[n].end[i]= gene[n].start[i];
					p -= 2;
					}
					else
					{
						while(p[0]<'0' || p[0]>'9') --p;
						while(p[0]>='0' && p[0]<='9') --p;
					++p;
					gene[n].end[i]= gene[n].newend[i]= atoi(p) - 1;
						while(p[0]!=',' && p>cds) --p;
					--p;
					}
				gene[n].color[i]= RGB[ ( gene[n].start[i] + gene[n].len % 3 - 2 ) % 3 ];
				gene[n].frame[i]= ( gene[n].start[i] + gene[n].len % 3 - 2 ) % 3 + 3;
				gene[n].len += gene[n].start[i] - gene[n].end[i] + 1;
				++nexons[0];
				}
			gene[n].end[i-1] += 3;
			gene[n].from= gene[n].end[i-1];
			gene[n].to= gene[n].start[0];
			gene[n].span= gene[n].to - gene[n].from + 1;
			}

			if(gene[n].span <= 0) fprintf(stderr, "\nGene %c %d..%d across origin of replication\n", gene[n].strand, gene[n].from, gene[n].to);
		
		++n;
		}
			if(flag)
			{
				if(p= strstr(longstr, "/gene="))
				{
				p[strlen(p) - 2]= '\0';
					if(NAME_OFFSET && strstr(p + 7, common_name)) strcpy(gene[n - 1].name, p + 7 + NAME_OFFSET);
					else                                          strcpy(gene[n - 1].name, p + 7);
				flag= 0;
				}
				if((p= strstr(longstr, "/locus_tag=")) && !strlen(gene[n - 1].name))
				{
				p[strlen(p) - 2]= '\0';
				strcpy(gene[n - 1].name, p + 12);
					if(NAME_OFFSET && strstr(p + 12, common_name)) strcpy(gene[n - 1].name, p + 12 + NAME_OFFSET);
					else                                           strcpy(gene[n - 1].name, p + 12);
				flag= 0;
				}
			}
	}

	if(strncmp(longstr,"ORIGIN",6)) { fprintf(stderr,"\nLine ORIGIN not found in file. Exiting.\n"); exit(1); }

ncds[0]= n;

free(cds);

return(ftell(fp));
}

// End of function annotation()

/***************************/
/**** Function G_test() ****/
/***************************/

double G_test(char *seq, int n,char Pg[], int *th)
{
int     i,j,k,pos;
double  G,E,O,usage[5][4]= { 0.0 };
char	Prob[14][15];

strcpy(Prob[0],"(p >= 5.0e-01)");
strcpy(Prob[1],"(p >= 4.0e-01)");
strcpy(Prob[2],"(p >= 3.0e-01)");
strcpy(Prob[3],"(p >= 2.0e-01)");
strcpy(Prob[4],"(p >= 1.0e-01)");
strcpy(Prob[5],"(p > 5.0e-02) ");
strcpy(Prob[6],"(p <= 1.0e-06)");
strcpy(Prob[7],"(p <= 1.0e-05)");
strcpy(Prob[8],"(p <= 1.0e-04)");
strcpy(Prob[9],"(p <= 1.0e-03)");
strcpy(Prob[10],"(p <= 5.0e-03)");
strcpy(Prob[11],"(p <= 1.0e-02)");
strcpy(Prob[12],"(p <= 2.5e-02)");
strcpy(Prob[13],"(p <= 5.0e-02)");

  for(k=0;k<n;++k)
  {
  ++usage[seq[k]][k%3];
  ++usage[seq[k]][3];
  ++usage[4][k%3];
  ++usage[4][3];
  }

  G= 0.0;

    for(i=0;i<4;++i)
      for(j=0;j<3;++j)
        if(usage[i][3] && usage[4][j] && usage[i][j])
        {
        O= usage[i][j];
        E= usage[i][3] / usage[4][3] * usage[4][j];
        G += O * log(O/E);
        }
  G *= 2.0;

  if(G <= 5.35) strcpy(Pg,Prob[0]);
  else if(G <= 6.21) strcpy(Pg,Prob[1]);
  else if(G <= 7.23) strcpy(Pg,Prob[2]);
  else if(G <= 8.56) strcpy(Pg,Prob[3]);
  else if(G <= 10.645) strcpy(Pg,Prob[4]);
  else if(G <= 12.592) strcpy(Pg,Prob[5]);

  else if(G > 38.26) strcpy(Pg,Prob[6]);
  else if(G > 33.11) strcpy(Pg,Prob[7]);
  else if(G > 27.86) strcpy(Pg,Prob[8]);
  else if(G > 22.46) strcpy(Pg,Prob[9]);
  else if(G > 18.548) strcpy(Pg,Prob[10]);
  else if(G > 16.812) strcpy(Pg,Prob[11]);
  else if(G > 14.449) strcpy(Pg,Prob[12]);
  else if(G > 12.592) strcpy(Pg,Prob[13]);

  if(G > 38.26 && SIGNIFICANCE == 0.000001) th[0]= 4;
  else if(G > 33.11 && SIGNIFICANCE == 0.00001) th[0]= 4;
  else if(G > 27.86 && SIGNIFICANCE == 0.0001) th[0]= 4;
  else if(G > 22.46 && SIGNIFICANCE == 0.001) th[0]= 4;
  else if(G > 18.548 && SIGNIFICANCE == 0.005) th[0]= 4;
  else if(G > 16.812 && SIGNIFICANCE == 0.01) th[0]= 4;
  else if(G > 14.449 && SIGNIFICANCE == 0.025) th[0]= 4;
  else if(G > 12.592 && SIGNIFICANCE == 0.05) th[0]= 4;
  else th[0]= 0;

return(G);
}

// End of function G_test()

/*************************************************************/

/*******************************/
/**** Function complement() ****/
/*******************************/

// Function that transforms a nucleotide sequence of numbers 0-3 into its inverted complement:

void complement(char *seq, int len)
{
int     i;
char    s;

  for(i=0;i<len;++i) seq[i]= 3 - seq[i];
  for(i=0;i<len/2;++i) { s= seq[i]; seq[i]= seq[len - 1 - i]; seq[len - 1 - i]= s; }
}

/* end of function complement() */

/*************************************************************/

/*******************************/
/**** Function invert_sequence() ****/
/*******************************/

// Function that transforms a nucleotide sequence of numbers 0-3 into its inverted complement:

void invert_sequence(char *seq, int len)
{
int     i;
char    s;

  for(i=0;i<len/2;++i) { s= seq[i]; seq[i]= seq[len - 1 - i]; seq[len - 1 - i]= s; }
}

/* end of function invert_sequence() */

/*************************************************************/


/*******************************/
/**** Function write_complement() ****/
/*******************************/

// Function that writes a nucleotide sequence of numbers 0-3 into the complementary sequence

void write_complement(char *seq,int len, char *cseq)
{
int     i;
char    s;

  for(i= 0; i < len; ++i) cseq[i]= 3 - seq[len - 1 - i];
}

/* end of function write_complement() */

/*******************************/
/**** Function copy_sequence() ****/
/*******************************/

// Function that writes a nucleotide sequence of numbers 0-3 into the complementary sequence

void copy_sequence(char *seq,int len, char *cseq)
{
int     i;
char    s;

  for(i= 0; i < len; ++i) cseq[i]= seq[i];
}

/* end of function copy_sequence() */


/********************************/
/**** Function composition() ****/
/********************************/

// Funcion composition() reads a coding sequence seq (of numbers 0-3) of length len and writes the nucleotide composition (fraction) for codon positions 1, 2 and 3 (rows 0-2) in nuc[4x6] where the last two columns are S (strong) and P (purine)

void composition(char *seq, int len, double *nuc, int offset)
{
int	i,j;
double	tot;

  for(i= 0; i < 4 * 6; ++i) nuc[i]= 0.0;

  for(i= 0; i < len; ++i) ++nuc[ 6 * ( ( i + offset ) % 3 ) + seq[i] ];

  for(i= 0; i < 3; ++i)
  {
  tot= 0.0;
    for(j= 0; j < 4; ++j) tot += nuc[i*6 + j];
    for(j= 0; j < 4; ++j) nuc[i * 6 + j] /= tot;
  nuc[i * 6 + 4]= nuc[i * 6 + 1] + nuc[i * 6 + 2]; // Strong
  nuc[i * 6 + 5]= nuc[i * 6] + nuc[i * 6 + 2]; // Purines
  }

  for(j= 0; j < 6; ++j) nuc[18 + j]= (nuc[j] + nuc[6 + j] +  nuc[12 + j]) / 3.0; 
}

// End of function composition()


/****************************/
/*** Function maxG_test() ***/
/****************************/

/***************
 Needs these global definitions:

#define SIGNIFICANCE Level of significance for G-test (and HSS-test)
#define DOWN_G       Prob of G-down; d.f. = 9
#define mHL          Minimum hit-length

struct HSSs *hss;

***************/

int maxG_test(char *seq, int len, int ori, char strand, int frame, int orfn, int hit, int *on)
{
        int i, j, k, h, l, from = len - 3, nhit= 0, to, bfrom, cod, last_stop= 0, f, t, flag, flags, nt, from_max, max_pos;
        double G, H, E, O, usage[5 * 4] = { 0.0 }, Maxusage[5 * 4] = {0.0}, diffusage[5 * 4], maxG, thr, HIGH_G, lg, hss_score, max_len;

max_len= pow(2.0, 1023.0);

// orf_num[frame][orfn][0] == 828845

/* ORIGINAL SCORES FUNCTION OF LN LN LENGTH: */

max_len= log(log(max_len));
 	
lg= log(log((double)len));
 
 	if(SIGNIFICANCE == 0.01)         HIGH_G = 17.070 * lg + 1.0753;
 	else if(SIGNIFICANCE == 0.001)   HIGH_G = 18.090 * lg + 4.4864;
 	else if(SIGNIFICANCE == 0.0001)  HIGH_G = 19.071 * lg + 7.6498;
 	else if(SIGNIFICANCE == 0.00001) HIGH_G = 21.782 * lg + 9.6705;

/**/

/* MODIFIED VALUES, LINEAR DEPENDENCE OF THRESHOLDS WITH LENGTH */
	
//	lg= (double)len;
//
//       if(SIGNIFICANCE == 0.01)         HIGH_G = 0.0102 * lg + 29.845;
//       else if(SIGNIFICANCE == 0.001)   HIGH_G = 0.0172 * lg + 33.609;


        maxG = 0.0;
        hss[hit].fromp= len;

        for(k= from; k >= 0; k -= 3)
        {
	cod= 16*seq[k] + 4*seq[k+1] + seq[k+2];

                for(j = 0; j < 3; ++j)
                {
                ++usage[4 * seq[k + j] + j];
                ++usage[4 * seq[k + j] + 3];
                ++usage[4 * 4 + j];
                ++usage[4 * 4 + 3];
                }

        G = 0.0;

                for(i = 0; i < 4; ++i)
                        for(j = 0; j < 3; ++j)
                                if(usage[4 * i + 3] && usage[4 * 4 + j] && usage[4 * i + j])
                                {
                                O= usage[4 * i + j];
                                E= (usage[4 * i + 3] / usage[4 * 4 + 3]) * usage[4 * 4 + j];
                                G += O * log(O / E);
                                }
        G *= 2.0;

                if((G > maxG) && usage[4 * 4 + 3] >= mHL)
                {
                maxG= G;
                hss[hit].fromp= k;
                copy_vector(usage, Maxusage, 5 * 4);
                }


                if((G < maxG && usage[4 * 4 + 3] >= mHL) || k <= 2)  // Old: else
                {
                        H= 0.0;
                        difference_vector(usage, Maxusage, diffusage, 5 * 4);
                        for(i= 0; i < 4; ++i)
                                for(j= 0; j < 3; ++j)
                                        if(Maxusage[4 * i + j] && Maxusage[4 * 4 + j] && diffusage[4 * i + j] && diffusage[4 * 4 + j])
                                        {
                                                O= diffusage[4 * i + j];
                                                E= (Maxusage[4 * i + j] / Maxusage[4 * 4 + j]) * diffusage[4 * 4 + j];
                                                H += O * log(O / E);
                                        }
                        H *= 2.0;

                        if((H > DOWN_G && diffusage[4 * 4 + 3] >= mHL) || k <= 2)
                        {
                       	bfrom= hss[hit].fromp;
			hss[hit].top= bfrom + 2;
                        to= from;
			from_max= bfrom;
			max_pos= to;

                       	clear_vector(usage, 5 * 4);
                       	maxG= 0.0;
			max_pos= bfrom;

                               	for(h= bfrom; h <= to; h += 3)
                               	{
                                       	for(j = 0; j < 3; ++j)
                                       	{
                                       	++usage[4 * seq[h + j] + j];
                                       	++usage[4 * seq[h + j] + 3];
                                       	++usage[4 * 4 + j];
                                       	++usage[4 * 4 + 3];
                                       	}

                               	G= 0.0;

                                       	for(i= 0; i < 4; ++i)
                                               	for(j= 0; j < 3; ++j)
                                                       	if(usage[4 * i + 3] && usage[4 * 4 + j] && usage[4 * i + j])
                                                       	{
                                                       	O= usage[4 * i + j];
                                                       	E= usage[4 * i + 3] / usage[4 * 4 + 3] * usage[4 * 4 + j];
                                                       	G += O * log(O / E);
                                                       	}
                               	G *= 2.0;

                                       	if((G > maxG) && usage[4 * 4 + 3] >= mHL)
                                       	{
                                       	maxG= G;
                                       	hss[hit].top= h + 2;
                                       	hss[hit].score= (float)maxG;
					max_pos= h;
                                       	}
                               	}

                               	if(maxG > HIGH_G && max_pos - from_max + 3 >= mHL)   //  h - bfrom + 1 is length of hit
                               	{
				flag= 1;
				build_scores(seq, len, sc);      // sc[6 * 64]
				hss[hit].score= score(seq + hss[hit].fromp, hss[hit].top - hss[hit].fromp + 1, sc, &f, &t, 1);
				composition(seq + hss[hit].fromp, hss[hit].top - hss[hit].fromp + 1, hss[hit].nuc, 0);

					for(j= 0; j < 6; ++j)
					{
					hss_score= score(seq + hss[hit].fromp, hss[hit].top - hss[hit].fromp + 1, sc + 64*j, &f, &t, 0);
						if(hss_score < 0.0) { flag= 0; j= 6; }
					}

				hss[hit].entropy= entropy(seq + hss[hit].fromp, hss[hit].top - hss[hit].fromp + 1);

				flags= 0;

					for(k= hss[hit].fromp; k >= 0 && k >= hss[hit].fromp - ATG_POS5; k -= 3)
					{
					nt= 16 * seq[k] + 4 * seq[k + 1] + seq[k + 2];
						if(nt == 14) { hss[hit].atg= k; k= 0; flags= 1; }
					}

					if(!flags)
						for(k= hss[hit].fromp; k < hss[hit].top && k <= hss[hit].fromp + ATG_POS3; k += 3)
						{
						nt= 16 * seq[k] + 4 * seq[k + 1] + seq[k + 2];
							if(nt == 14) { hss[hit].atg= k; k= hss[hit].top; flags= 1; }
						}

					if(!flags)
						for(k= hss[hit].fromp; k >= 0 && k >= hss[hit].fromp - GTG_POS5; k -= 3)
						{
						nt= 16 * seq[k] + 4 * seq[k + 1] + seq[k + 2];
							if(nt == 46) { hss[hit].gtg= k; k= 0; flags= 2; }
						}

					if(!flags)
						for(k= hss[hit].fromp; k < hss[hit].top && k <= hss[hit].fromp + GTG_POS3; k += 3)
						{
						nt= 16 * seq[k] + 4 * seq[k + 1] + seq[k + 2];
							if(nt == 46) { hss[hit].gtg= k; k= hss[hit].top; flags= 2; }
						}

					if(!flags)
						for(k= hss[hit].fromp - ATG_POS5 - 3; k >= 0; k -= 3)
						{
						nt= 16 * seq[k] + 4 * seq[k + 1] + seq[k + 2];
							if(nt == 14) { hss[hit].atg= k; k= 0; flags= 3; }
							else if(nt == 46) { hss[hit].gtg= k; k= 0; flags= 4; }
						}

					if(!flags)
						for(k= hss[hit].fromp; k >= 0 && k >= hss[hit].fromp - TTG_POS5; k -= 3)
						{
						nt= 16 * seq[k] + 4 * seq[k + 1] + seq[k + 2];
							if(nt == 62) { hss[hit].ttg= k; k= 0; flags= 5; }
						}

					if(!flags)
						for(k= hss[hit].fromp; k < hss[hit].top && k <= hss[hit].fromp + TTG_POS3; k += 3)
						{
						nt= 16 * seq[k] + 4 * seq[k + 1] + seq[k + 2];
							if(nt == 62) { hss[hit].ttg= k; k= hss[hit].top; flags= 5; }
						}

					if(!flags)
						for(k= hss[hit].fromp - TTG_POS5 - 3; k >= 0; k -= 3)
						{
						nt= 16 * seq[k] + 4 * seq[k + 1] + seq[k + 2];
							if(nt == 62) { hss[hit].ttg= k; k= 0; flags= 6; }
						}

				hss[hit].start= flags;

					if(WRITE_SEQUENCES)
					{
						if(flags == 1 || flags == 3)
						{
						nt= len - hss[hit].atg + 2;
						hss[hit].seqlen= nt - 1;
						hss[hit].seq= (char *)malloc(nt * sizeof(char));
						strncpy(hss[hit].seq, seq + hss[hit].atg, nt - 1);
						}
						else if(flags == 2 || flags == 4)
						{
						nt= len - hss[hit].gtg + 2;
						hss[hit].seqlen= nt - 1;
						hss[hit].seq= (char *)malloc(nt * sizeof(char));
						strncpy(hss[hit].seq, seq + hss[hit].gtg, nt - 1);
						}
						else if(flags == 5 || flags == 6)
						{
						nt= len - hss[hit].ttg + 2;
						hss[hit].seqlen= nt - 1;
						hss[hit].seq= (char *)malloc(nt * sizeof(char));
						strncpy(hss[hit].seq, seq + hss[hit].ttg, nt - 1);
						}
						else
						{
						nt= len - hss[hit].fromp + 2;
						hss[hit].seqlen= nt - 1;
						hss[hit].seq= (char *)malloc(nt * sizeof(char));
						strncpy(hss[hit].seq, seq + hss[hit].fromp, len);
						}
					}

					if(hss[hit].start == 1 || hss[hit].start == 3)
					{
						if(strand == 'D') hss[hit].atg += ori;
						else  hss[hit].atg= ori - hss[hit].atg;
					}
					else if(hss[hit].start == 2 || hss[hit].start == 4)
					{
						if(strand == 'D') hss[hit].gtg += ori;
						else  hss[hit].gtg= ori - hss[hit].gtg;
					}
					else if(hss[hit].start == 5 || hss[hit].start == 6)
					{
						if(strand == 'D') hss[hit].ttg += ori;
						else  hss[hit].ttg= ori - hss[hit].ttg;
					}

					if(strand == 'D')
					{
					hss[hit].fromp += ori;
					hss[hit].top += ori;
					hss[hit].stop1= orf_num[frame][orfn][0];
					hss[hit].stop2= orf_num[frame][orfn][1];
					}
					else                  // strand == 'C'
					{
					j= ori - hss[hit].top;
					hss[hit].top= ori - hss[hit].fromp;
					hss[hit].fromp= j;
					hss[hit].stop1= orf_num[frame][orfn][0];
					hss[hit].stop2= orf_num[frame][orfn][1];
					}

				hss[hit].len= hss[hit].top - hss[hit].fromp + 1;
				hss[hit].strand= strand;
				hss[hit].hit_type= 'G';
				hss[hit].hsuper= 0;
				hss[hit].gsuper= 0;
				hss[hit].frame= frame;
				hss[hit].G= maxG;

/* LN LN LENGTH */
				hss[hit].sig_len= (maxG - 1.0753)/17.070;
					if(hss[hit].sig_len < max_len)
						hss[hit].sig_len= exp(exp(hss[hit].sig_len));
					else    hss[hit].sig_len= exp(exp(max_len));

/* LENGTH */
//				hss[hit].sig_len= (maxG - 29.845)/0.0102;
//					if(hss[hit].sig_len < max_len)
//						hss[hit].sig_len= hss[hit].sig_len;
//					else    hss[hit].sig_len= max_len;

/* LN LN LENGTH PARAMETERS */
					if(maxG >= 21.782 * lg + 9.6705) { hss[hit].prob= 0.00001; strcpy(hss[hit].pstring,"(p <= 1.0e-05)"); } 
					else if(maxG >= 19.071 * lg + 7.6498) { hss[hit].prob= 0.0001; strcpy(hss[hit].pstring,"(p <= 1.0e-04)"); }
					else if(maxG >= 18.090 * lg + 4.4864) { hss[hit].prob= 0.001; strcpy(hss[hit].pstring,"(p <= 1.0e-03)"); }
					else if(maxG >= 17.070 * lg + 1.0753) {  hss[hit].prob= 0.01; strcpy(hss[hit].pstring,"(p <= 1.0e-02)"); }
/**/

/* LINEAR DEPENDENCE OF SIGNIFICANCE OF G WITH LENGTH */

//        if(SIGNIFICANCE == 0.01)         HIGH_G = 0.0102 * lg + 29.845;
//        else if(SIGNIFICANCE == 0.001)   HIGH_G = 0.0172 * lg + 33.609;
//
//					if(maxG >= 0.0172 * lg + 33.609) { hss[hit].prob= 0.001; strcpy(hss[hit].pstring,"(p <= 1.0e-03)"); }
//					else if(maxG >= 0.0102 * lg + 29.845) {  hss[hit].prob= 0.01; strcpy(hss[hit].pstring,"(p <= 1.0e-02)"); }

					if(flag)
					{
					hss[hit].type= 0;
					reorder_hits(hit, orfn);
					}
					else
					{
					hss[hit].type= 5;
					hss[hit].hit_num= -1;
					}


				++nhit;
				hss[hit].orf_num= orfn;
                               	++hit;
				hss= (struct HSSs *)realloc(hss,(hit+1)*sizeof(struct HSSs));
                               	}

                                if(bfrom - last_stop < mHL || bfrom >= to) { k= last_stop; from= last_stop - 3; last_stop= 0; }
                                else { from= bfrom - 3; k= bfrom; hss[hit].fromp= bfrom; }

				if(h - max_pos >= mHL) hit= maxG_test(seq + max_pos + 3, to - max_pos, ori + ((frame / 3) * 2 - 1) * ( max_pos + 3), strand, frame, orfn, hit, on);

			maxG= 0.0;
			clear_vector(usage, 5 * 4);
			clear_vector(Maxusage, 5 * 4);
                        }
                }
        }
hss[hit].top= -1;

return(hit);
}

/*** End of function maxG_test() ***/

/*********************************/
/**** Function clear_vector() ****/
/*********************************/

void clear_vector(double *vector, int size)
{
	int i;

	for(i = 0; i < size; ++i)
		vector[i] = 0.0;
}

/************************************/
/**** Function clear_intvector() ****/
/************************************/

void clear_intvector(int *vector, int size)
{
	int i;

	for(i = 0; i < size; ++i)
		vector[i] = 0;
}

/********************************/
/**** Function copy_vector() ****/
/********************************/

void copy_vector(double *from_vector, double *to_vector, int size)
{
	int i;

	for(i = 0; i < size; ++i)
		to_vector[i] = from_vector[i];
}

/*******************************/
/**** Function add_vector() ****/
/*******************************/

void add_vector(int *vector, int *add_vector, int size)
{
	int i;

	for(i = 0; i < size; ++i)
		vector[i] += add_vector[i];
}

/**************************************/
/**** Function difference_vector() ****/
/**************************************/

void difference_vector(double *vector, double *minus_vector, double *result_vector, int size)
{
	int i;

	for(i = 0; i < size; ++i)
		result_vector[i] = vector[i] - minus_vector[i];
}


/*****************************************/
/*** Function build_scores() *************/
/*****************************************/

void build_scores(char *seg, int n, double *sc)      // sc[6 * 64]
{
int i, j = 0, k, l;
double r, x1, y1, a, b;
double N[16] = {0.0}, stop0, stop1;     // N[4][4], sc[6* 64]

	for(i = 0; i < n; i++)
	{
	++N[seg[i]];                        // ++N[0][seg[i]];
	++N[(n % 3 + 1)*4 + seg[i]];        // ++N[n % 3 + 1][seg[i]];
	}

	for(i = 0; i < 4; i++)
		N[i] /= (double)n;       // N[0][i] /= (double)n;
	
// SCORES WITH BOUNDARIES:

// A
        if(N[0]<(x1=0.1251+EPSILON))      // N[0][0]
        {
        y1= 0.8288 * x1 + 0.0527;
        a= y1 / x1;
        N[4] =  a * N[0];               // N[1][0], N[0][0]
        y1= 0.5883 * x1 + 0.1454;
        a= y1 / x1;
        N[8] =  a * N[0];               // N[2][0], N[0][0]
        y1= 1.5829 * x1 - 0.1980;
        a= y1 / x1;
        N[12] =  a * N[0];              // N[3][0], N[0][0]
        }
        else if(N[0]>(x1=0.7568-EPSILON))
        {
        y1= 0.8288 * x1 + 0.0527;
        a= (y1 - 1.0)/(x1 - 1.0);
        b= y1 - a*x1;
        N[4] =  a * N[0] + b;    // N[1][0], N[0][0]
        y1= 0.5883 * x1 + 0.1454;
        a= (y1 - 1.0)/(x1 - 1.0);
        b= y1 - a*x1;
        y1= 1.5829 * x1 - 0.1980;
        N[8] =  a * N[0] + b;      // N[2][0], N[0][0]
        a= (y1 - 1.0)/(x1 - 1.0);
        b= y1 - a*x1;
        N[12] =  a * N[0] + b;    // N[3][0], N[0][0]
        }
        else
        {
        N[4] =  0.8288 * N[0] + 0.0527;         // N[1][0] =  0.8288 * N[0][0] + 0.0527;
        N[8] =  0.5883 * N[0] + 0.1454;         // N[2][0] =  0.5883 * N[0][0] + 0.1454;
        N[12] =  1.5829 * N[0] - 0.1980;         // N[3][0] =  1.5829 * N[0][0] - 0.1980;
        }

// C
        if(N[1]<(x1=0.0865+EPSILON))        // if(N[0][1]<(x1=0.0865+EPSILON))
        {
        y1= 0.7388 * x1 + 0.0359;
        a= y1 / x1;
        N[5] =  a * N[1];             // N[1][1] =  a * N[0][1];
        y1= 0.4424 * x1 + 0.1215;
        a= y1 / x1;
        N[9] =  a * N[1];       // N[2][1] =  a * N[0][1];
        y1= 1.8188 * x1 - 0.1574;
        a= y1 / x1;
        N[13] =  a * N[1];       // N[3][1] =  a * N[0][1];
        }
        else if(N[1]>(x1=0.6364-EPSILON))     // else if(N[0][1]>(x1=0.6364-EPSILON))
        {
        y1= 0.7388 * x1 + 0.0359;
        a= (y1 - 1.0)/(x1 - 1.0);
        b= y1 - a*x1;
        N[5] =  a * N[1] + b;         // N[1][1] =  a * N[0][1] + b;
        y1= 0.4424 * x1 + 0.1215;
        a= (y1 - 1.0)/(x1 - 1.0);
        b= y1 - a*x1;
        N[9] =  a * N[1] + b;        // N[2][1] =  a * N[0][1] + b;
        y1= 1.8188 * x1 - 0.1574;
        a= (y1 - 1.0)/(x1 - 1.0);
        b= y1 - a*x1;
        N[13] =  a * N[1] + b;     // N[3][1] =  a * N[0][1] + b;
        }
        else
        {
        N[5] =  0.7388 * N[1] + 0.0359;     // N[1][1] =  0.7388 * N[0][1] + 0.0359;
        N[9] =  0.4424 * N[1] + 0.1215;    // N[2][1] =  0.4424 * N[0][1] + 0.1215;
        N[13] =  1.8188 * N[1] - 0.1574;    // N[3][1] =  1.8188 * N[0][1] - 0.1574;
        }

// G
        if(N[2]<(x1=0.1168+EPSILON))       // if(N[0][2]<(x1=0.1168+EPSILON)) 
        {
        y1= 0.7257 * x1 + 0.1620;
        a= y1 / x1;
        N[6] =  a * N[2];       // N[1][2] =  a * N[0][2];
        y1= 0.4800 * x1 + 0.0476;
        a= y1 / x1;
        N[10] =  a * N[2];      // N[2][2] =  a * N[0][2];
        y1= 1.7943 * x1 - 0.2096;
        a= y1 / x1;
        N[14] =  a * N[2];      // N[3][2] =  a * N[0][2];
        }
        else if(N[2]>(x1=0.6741-EPSILON))     //  else if(N[0][2]>(x1=0.6741-EPSILON))
        {
        y1= 0.7257 * x1 + 0.1620;
        a= (y1 - 1.0)/(x1 - 1.0);
        b= y1 - a*x1;
        N[6] =  a * N[2] + b;        // N[1][2] =  a * N[0][2] + b;
        y1= 0.4800 * x1 + 0.0476;
        a= (y1 - 1.0)/(x1 - 1.0);
        b= y1 - a*x1;
        N[10] =  a * N[2] + b;   // N[2][2] =  a * N[0][2] + b;
        y1= 1.7943 * x1 - 0.2096;
        a= (y1 - 1.0)/(x1 - 1.0);
        b= y1 - a*x1;
        N[14] =  a * N[2] + b;    // N[3][2] =  a * N[0][2] + b;
        }
        else
        {
        N[6] =  0.7257 * N[2] + 0.1620;      //  N[1][2] =  0.7257 * N[0][2] + 0.1620;
        N[10] =  0.4800 * N[2] + 0.0476;    // N[2][2] =  0.4800 * N[0][2] + 0.0476;
        N[14] =  1.7943 * N[2] - 0.2096;    // N[3][2] =  1.7943 * N[0][2] - 0.2096;
        }

//T
        if(N[3]<(x1=0.1223+EPSILON))     // if(N[0][3]<(x1=0.1223+EPSILON))
        {
        y1= 0.6013 * x1 + 0.0237;
        a= y1 / x1;
        N[7] =  a * N[3];     // N[1][3] =  a * N[0][3];
        y1= 0.2738 * x1 + 0.2361;
        a= y1 / x1;
        N[11] =  a * N[3];    // N[2][3] =  a * N[0][3];
        y1= 2.1249 * x1 - 0.2599;
        a= y1 / x1;
        N[15] =  a * N[3];     //  N[3][3] =  a * N[0][3];
        }
        else if(N[3]>(x1=0.5929-EPSILON))    //  else if(N[0][3]>(x1=0.5929-EPSILON))
        {
        y1= 0.6013 * x1 + 0.0237;
        a= (y1 - 1.0)/(x1 - 1.0);
        b= y1 - a*x1;
        N[7] =  a * N[3] + b;     //  N[1][3] =  a * N[0][3] + b;
        y1= 0.2738 * x1 + 0.2361;
        a= (y1 - 1.0)/(x1 - 1.0);
        b= y1 - a*x1;
        N[11] =  a * N[3] + b;    //  N[2][3] =  a * N[0][3] + b;
        y1= 2.1249 * x1 - 0.2599;
        a= (y1 - 1.0)/(x1 - 1.0);
        b= y1 - a*x1;
        N[15] =  a * N[3] + b;     //  N[3][3] =  a * N[0][3] + b;
        }
        else
        {
        N[7] =  0.6013 * N[3] + 0.0237;    // N[1][3] =  0.6013 * N[0][3] + 0.0237;
        N[11] =  0.2738 * N[3] + 0.2361;    // N[2][3] =  0.2738 * N[0][3] + 0.2361;
        N[15] =  2.1249 * N[3] - 0.2599;     // N[3][3] =  2.1249 * N[0][3] - 0.2599;
        }

// Old N[][]:	stop1 = N[1][3] * N[2][0] * N[3][0] + N[1][3] * N[2][0] * N[3][2] + N[1][3] * N[2][2] * N[3][0];

	stop1 = N[7] * N[8] * N[12] + N[7] * N[8] * N[14] + ( N[7] * N[10] * N[12] ) * (1 - MYCOPLASMA);

	for(i = 0; i < 4; i++)
		for(j = 0; j < 4; j++)
			for(k = 0; k < 4; k++)
			    for(l = 0; l < 6; l++)
				sc[l*64 + 16*i+4*j+k] = log(N[4 + i] * N[8 + j] * N[12 + k] / (1.0 - stop1)); 

// Old N[][]:				sc[l][16*i+4*j+k] = log(N[1][i] * N[2][j] * N[3][k] / (1.0 - stop1)); 

// Scores against random:

// Old N[][]:	stop0 = N[0][3] * N[0][0] * N[0][0] + N[0][3] * N[0][0] * N[0][2] + N[0][3] * N[0][2] * N[0][0];

	stop0 = N[3] * N[0] * N[0] + N[3] * N[0] * N[2] + ( N[3] * N[2] * N[0]) * (1 - MYCOPLASMA);

	for(i = 0; i < 4; i++)
		for(j = 0; j < 4; j++)
			for(k = 0; k < 4; k++)
				sc[16*i+4*j+k] -= log(N[i] * N[j] * N[k] / (1.0 - stop0));

// Old N[][]:				sc[0][16*i+4*j+k] -= log(N[0][i] * N[0][j] * N[0][k] / (1.0 - stop0));

// Expected frequency of stop codons in direct frame:

	stop0 = stop1;

// Scores against jki:

	for(i = 0; i < 4; i++)
		for(j = 0; j < 4; j++)
			for(k = 0; k < 4; k++)
				sc[64 + 16*i+4*j+k] -= log(N[4 + j] * N[8 + k] * N[12 + i] / (1.0 - stop0));

// Old N[][]:				sc[1][16*i+4*j+k] -= log(N[1][j] * N[2][k] * N[3][i] / (1.0 - stop0));

// Scores against kij:

	for(i = 0; i < 4; i++)
		for(j = 0; j < 4; j++)
			for(k = 0; k < 4; k++)
				sc[128 + 16*i+4*j+k] -= log(N[4 + k] * N[8 + i] * N[12 + j] / (1.0 - stop0));

// Old N[][]:				sc[2][16*i+4*j+k] -= log(N[1][k] * N[2][i] * N[3][j] / (1.0 - stop0));

// A
        if(N[3]<(x1=0.1251+EPSILON))    // if(N[0][3]<(x1=0.1251+EPSILON))
        {
        y1= 0.8288 * x1 + 0.0527;
        a= y1 / x1;
        N[4] =  a * N[3];     // N[1][0] =  a * N[0][3];
        y1= 0.5883 * x1 + 0.1454;
        a= y1 / x1;
        N[8] =  a * N[3];    // N[2][0] =  a * N[0][3];
        y1= 1.5829 * x1 - 0.1980;
        a= y1 / x1;
        N[12] =  a * N[3];    //  N[3][0] =  a * N[0][3];
        }
        else if(N[3]>(x1=0.7568-EPSILON))    //  else if(N[0][3]>(x1=0.7568-EPSILON))
        {
        y1= 0.8288 * x1 + 0.0527;
        a= (y1 - 1.0)/(x1 - 1.0);
        b= y1 - a*x1;
        N[4] =  a * N[3] + b;     //  N[1][0] =  a * N[0][3] + b;
        y1= 0.5883 * x1 + 0.1454;
        a= (y1 - 1.0)/(x1 - 1.0);
        b= y1 - a*x1;
        y1= 1.5829 * x1 - 0.1980;
        N[8] =  a * N[3] + b;     //  N[2][0] =  a * N[0][3] + b;
        a= (y1 - 1.0)/(x1 - 1.0);
        b= y1 - a*x1;
        N[12] =  a * N[3] + b;    //  N[3][0] =  a * N[0][3] + b;
        }
        else
        {
        N[4] =  0.8288 * N[3] + 0.0527;    //  N[1][0] =  0.8288 * N[0][3] + 0.0527;
        N[8] =  0.5883 * N[3] + 0.1454;    //  N[2][0] =  0.5883 * N[0][3] + 0.1454;
        N[12] =  1.5829 * N[3] - 0.1980;   // N[3][0] =  1.5829 * N[0][3] - 0.1980;
        }

// C
        if(N[2]<(x1=0.0865+EPSILON))     // if(N[0][2]<(x1=0.0865+EPSILON))
        {
        y1= 0.7388 * x1 + 0.0359;
        a= y1 / x1;
        N[5] =  a * N[2];    //  N[1][1] =  a * N[0][2];
        y1= 0.4424 * x1 + 0.1215;
        a= y1 / x1;
        N[9] =  a * N[2];  // N[2][1] =  a * N[0][2];
        y1= 1.8188 * x1 - 0.1574;
        a= y1 / x1;
        N[13] =  a * N[2];    //  N[3][1] =  a * N[0][2];
        }
        else if(N[2]>(x1=0.6364-EPSILON))    //  else if(N[0][2]>(x1=0.6364-EPSILON))
        {
        y1= 0.7388 * x1 + 0.0359;
        a= (y1 - 1.0)/(x1 - 1.0);
        b= y1 - a*x1;
        N[5] =  a * N[2] + b;    // N[1][1] =  a * N[0][2] + b;
        y1= 0.4424 * x1 + 0.1215;
        a= (y1 - 1.0)/(x1 - 1.0);
        b= y1 - a*x1;
        N[9] =  a * N[2] + b;    // N[2][1] =  a * N[0][2] + b;
        y1= 1.8188 * x1 - 0.1574;
        a= (y1 - 1.0)/(x1 - 1.0);
        b= y1 - a*x1;
        N[13] =  a * N[2] + b;    //  N[3][1] =  a * N[0][2] + b;
        }
        else
        {
        N[5] =  0.7388 * N[2] + 0.0359;    //  N[1][1] =  0.7388 * N[0][2] + 0.0359;
        N[9] =  0.4424 * N[2] + 0.1215;     //  N[2][1] =  0.4424 * N[0][2] + 0.1215;
        N[13] =  1.8188 * N[2] - 0.1574;    //  N[3][1] =  1.8188 * N[0][2] - 0.1574;
        }

// G
        if(N[1]<(x1=0.1168+EPSILON))    //  if(N[0][1]<(x1=0.1168+EPSILON))
        {
        y1= 0.7257 * x1 + 0.1620;
        a= y1 / x1;
        N[10] =  a * N[1];    //  N[1][2] =  a * N[0][1];
        y1= 0.4800 * x1 + 0.0476;
        a= y1 / x1;
        N[10] =  a * N[1];   //  N[2][2] =  a * N[0][1];
        y1= 1.7943 * x1 - 0.2096;
        a= y1 / x1;
        N[14] =  a * N[1];    //  N[3][2] =  a * N[0][1];
        }
        else if(N[1]>(x1=0.6741-EPSILON))    //  else if(N[0][1]>(x1=0.6741-EPSILON))
        {
        y1= 0.7257 * x1 + 0.1620;
        a= (y1 - 1.0)/(x1 - 1.0);
        b= y1 - a*x1;
        N[6] =  a * N[1] + b;    //  N[1][2] =  a * N[0][1] + b;
        y1= 0.4800 * x1 + 0.0476;
        a= (y1 - 1.0)/(x1 - 1.0);
        b= y1 - a*x1;
        N[10] =  a * N[1] + b;    //  N[2][2] =  a * N[0][1] + b;
        y1= 1.7943 * x1 - 0.2096;
        a= (y1 - 1.0)/(x1 - 1.0);
        b= y1 - a*x1;
        N[14] =  a * N[1] + b;    //  N[3][2] =  a * N[0][1] + b;
        }
        else
        {
        N[6] =  0.7257 * N[1] + 0.1620;    //  N[1][2] =  0.7257 * N[0][1] + 0.1620;
        N[10] =  0.4800 * N[1] + 0.0476;    //  N[2][2] =  0.4800 * N[0][1] + 0.0476;
        N[14] =  1.7943 * N[1] - 0.2096;    //  N[3][2] =  1.7943 * N[0][1] - 0.2096;
        }

//T
        if(N[0]<(x1=0.1223+EPSILON))    //  if(N[0][0]<(x1=0.1223+EPSILON))
        {
        y1= 0.6013 * x1 + 0.0237;
        a= y1 / x1;
        N[7] = a * N[0];    //  N[1][3] =  a * N[0][0];
        y1= 0.2738 * x1 + 0.2361;
        a= y1 / x1;
        N[11] =  a * N[0];    //  N[2][3] =  a * N[0][0];
        y1= 2.1249 * x1 - 0.2599;
        a= y1 / x1;
        N[15] =  a * N[0];    //  N[3][3] =  a * N[0][0];
        }
        else if(N[0]>(x1=0.5929-EPSILON))    //  else if(N[0][0]>(x1=0.5929-EPSILON))
        {
        y1= 0.6013 * x1 + 0.0237;
        a= (y1 - 1.0)/(x1 - 1.0);
        b= y1 - a*x1;
        N[7] =  a * N[0] + b;    //  N[1][3] =  a * N[0][0] + b;
        y1= 0.2738 * x1 + 0.2361;
        a= (y1 - 1.0)/(x1 - 1.0);
        b= y1 - a*x1;
        N[11] =  a * N[0] + b;    //  N[2][3] =  a * N[0][0] + b;
        y1= 2.1249 * x1 - 0.2599;
        a= (y1 - 1.0)/(x1 - 1.0);
        b= y1 - a*x1;
        N[15] =  a * N[0] + b;    //  N[3][3] =  a * N[0][0] + b;
        }
        else
        {
        N[7] =  0.6013 * N[0] + 0.0237;    //  N[1][3] =  0.6013 * N[0][0] + 0.0237;
        N[11] =  0.2738 * N[0] + 0.2361;    //  N[2][3] =  0.2738 * N[0][0] + 0.2361;
        N[15] =  2.1249 * N[0] - 0.2599;    //  N[3][3] =  2.1249 * N[0][0] - 0.2599;
        }


// Expected frequency of stop codons in complementary frame:

	stop0 = N[7] * N[8] * N[12] + N[7] * N[8] * N[14] + ( N[7] * N[10] * N[12] ) * (1 - MYCOPLASMA);

// Old N[][]:	stop0 = N[1][3] * N[2][0] * N[3][0] + N[1][3] * N[2][0] * N[3][2] + N[1][3] * N[2][2] * N[3][0];

// Scores against comp ijk:

	for(i = 0; i < 4; i++)
		for(j = 0; j < 4; j++)
			for(k = 0; k < 4; k++)
				sc[192 + 16*i+4*j+k] -= log(N[15-i] * N[11-j] * N[7-k] / (1.0 - stop0));

// Old N[][]:				sc[3][16*i+4*j+k] -= log(N[3][3-i] * N[2][3-j] * N[1][3-k] / (1.0 - stop0));

// Scores against comp jki:

	for(i = 0; i < 4; i++)
		for(j = 0; j < 4; j++)
			for(k = 0; k < 4; k++)
				sc[256 + 16*i+4*j+k] -= log(N[15-j] * N[11-k] * N[7-i] / (1.0 - stop0));

// Old N[][]:				sc[4][16*i+4*j+k] -= log(N[3][3-j] * N[2][3-k] * N[1][3-i] / (1.0 - stop0));

// Scores against comp kij:

	for(i = 0; i < 4; i++)
		for(j = 0; j < 4; j++)
			for(k = 0; k < 4; k++)
				sc[320 + 16*i+4*j+k] -= log(N[15-k] * N[11-i] * N[7-j] / (1.0 - stop0));

// Old N[][]:				sc[5][16*i+4*j+k] -= log(N[3][3-k] * N[2][3-i] * N[1][3-j] / (1.0 - stop0));
}

/*****End of function build_scores() *****/

/*****************************************/
/********* Function rescue_hss() *********/
/*****************************************/

void rescue_hss(int to_hss)
{
int	i, j, k, flag, tot= 0;

	for(i= 0; i < to_hss; ++i)
	{
		if(hss[i].type == 5 && hss[i].len >= mHL)
		{
		flag= 1;
			for(j= 0; j < to_hss; ++j)
			{
				if(hss[j].type != 5 && hss[j].len >= mHL)   // This condition also excludes conmparison with self.
					if(hss[i].fromp < hss[j].top && hss[i].top > hss[j].fromp) { flag= 0; j= to_hss; }
			}
			if(flag) hss[i].type= 0;
		}
	}

fprintf(stderr,"\n\nTotal rescued hss: %d\n\n",tot);
}

/***** End of function rescue_hss()  *****/

/*****************************************/
/********* Function process_hss() ********/
/*****************************************/

void process_hss(int from_hss, int to_hss, int ncds)
{
int	i, j, k, h, l, *o, s1, s2, flag, types[13]= { 0 }, from, to, gfrom, gto, out, nex;
char	Pg[15];
double  scoi, scoj;
double	li, lj, G;

o= (int *)malloc(to_hss * sizeof(int));

	for(i= 0; i < to_hss; ++i) o[i]= i;

//Hits sorted by maximum length of global significance (the longer the length the higher the significance)

fprintf(stderr, "\n\nEvaluating global significance.."); 

	for(i= 0; i < to_hss - 1; ++i)
		for(j= i + 1; j < to_hss; ++j)
			if(hss[o[j]].sig_len > hss[o[i]].sig_len) { k= o[i]; o[i]= o[j]; o[j]= k; }

	for(i= 0; i < to_hss; ++i)
	{
//		if(hss[o[i]].hit_type == 'H' && hss[o[i]].len >= mHL)
//		{
//			if(hss[o[i]].type != 5) fprintf(stdout,"%.3f\t%.3f\t\t%d\n",hss[o[i]].score, hss[o[i]].score/(float)hss[o[i]].len, hss[o[i]].len);
//			else if(hss[o[i]].type == 5) fprintf(stdout,"%.3f\t\t%.3f\t%d\n",hss[o[i]].score, hss[o[i]].score/(float)hss[o[i]].len, hss[o[i]].len);
//		}
		if(hss[o[i]].sig_len >= (double)search_size)
		{
		hss[o[i]].global_sig= 1;
		search_size -= hss[o[i]].len;
		}
		else hss[o[i]].global_sig= 0;
	}


// Hits are sorted by score.

// fprintf(stderr, "\n\nSorting hits by score.."); 
// 
// 	for(i= 0; i < to_hss; ++i) o[i]= i;
// 
// 	for(i= 0; i < to_hss - 1; ++i)
// 		for(j= i + 1; j < to_hss; ++j)
// 			if(hss[o[j]].score > hss[o[i]].score) { k= o[i]; o[i]= o[j]; o[j]= k; }


// Hits are sorted by length of global significance (the maximum sequence length at which the hit has significance p < 0.01 ).
//
//      for(i= 0; i < to_hss; ++i) o[i]= i;
//
//      for(i= 0; i < to_hss - 1; ++i)
//              for(j= i + 1; j < to_hss; ++j)
//                      if(hss[o[j]].sig_len > hss[o[i]].sig_len) { k= o[i]; o[i]= o[j]; o[j]= k; }


// This sorts hits by the score per unit length of hit (i.e., differential of cumulative score)
 
fprintf(stderr, "\n\nSorting hits by score slope.."); 
 
	for(i= 0; i < to_hss; ++i) o[i]= i;

	for(i= 0; i < to_hss - 1; ++i)
	{
	li= hss[o[i]].top - hss[o[i]].fromp + 1;
		for(j= i + 1; j < to_hss; ++j)
		{
		lj= hss[o[j]].top - hss[o[j]].fromp + 1;
			if(hss[o[j]].score/lj > hss[o[i]].score/li) { k= o[i]; o[i]= o[j]; o[j]= k; }
		}
	}


/**** TO BE CHECKED !!! ***/

// This sorts hits by the score per unit length of ORF, from start of hit to stop codon (i.e., differential of cumulative score)
// 
// fprintf(stderr, "\n\nSorting hits by score slope.."); 
// 
//         for(i= 0; i < to_hss; ++i) o[i]= i;
// 
//         for(i= 0; i < to_hss - 1; ++i)
//         {
//         scoi= 0.0;
//         k= o[i];
//                 while((k= hss[k].next_hit) != -1) scoi += hss[k].score;
//                 if(hss[o[i]].strand == 'D') li= hss[o[i]].stop2 - hss[o[i]].fromp + 1;
//                 else                        li= hss[o[i]].top - hss[o[i]].stop1 + 1;
//                 for(j= i + 1; j < to_hss; ++j)
//                 {
//                 scoj= 0.0;
//                 k= o[j];
//                         while((k= hss[k].next_hit) != -1) scoj += hss[k].score;
//                         if(hss[o[j]].strand == 'D') lj= hss[o[j]].stop2 - hss[o[j]].fromp + 1;
//                         else                        lj= hss[o[j]].top - hss[o[j]].stop1 + 1;
//                         if(scoj/lj > scoi/li) { k= o[i]; o[i]= o[j]; o[j]= k; }
//                 }
//         }


fprintf(stderr, "\n\nProcessing HSSs:     "); 

// Checks overlap with annotated genes and with higher-scoring HSSs

	for(i= 0; i < to_hss; ++i)
	{
	fprintf(stderr,"\b\b\b\b%3.0f%%",(float)(i+1)/(float)(to_hss+1)*100.0);
		if(hss[o[i]].type != 5)
		{
 		k= 0;
		l=  hss[o[i]].top-hss[o[i]].fromp+1;
			if(hss[o[i]].strand == 'D') { gfrom= hss[o[i]].fromp; gto= hss[o[i]].stop2; }
			else                        { gto= hss[o[i]].top; gfrom= hss[o[i]].stop1; }

		G= hss[o[i]].G;
		strcpy(Pg,hss[o[i]].pstring);

// Checks overlap with annotated genes

			for(j=0;j<ncds;++j)
			{
			nex= gene[j].num_exons;
				for(h= 0; h < nex; ++h)
				{
					if(gene[j].strand == 'D') { from= gene[j].start[h]; to= gene[j].end[h]; }
					else                      { to= gene[j].start[h]; from= gene[j].end[h]; }

					if(!(to < gfrom + mHL/2 - 1 || from > gto - mHL/2 + 1)) // predicted CDS overlapping published CDS (gene[])
					{
						if(hss[o[i]].frame == gene[j].frame[h])
						{
							if(gfrom == from && gto == to)
							{
							k= 1;		// Identical to published gene
							hss[o[i]].type= k;
							j= ncds;
							h= nex;
							}
							else if(gfrom >= from - mHL/2 && gto <= to + mHL/2)
							{
							k= 2;		// Part of published gene
							hss[o[i]].type= k;
							j= ncds;
							h= nex;
							}
							else
							{
							k= 3;           // Extending published gene
							hss[o[i]].type= k;
							j= ncds;
							h= nex;
							}
						}
						else
						{
							if(gfrom >= from - mHL/2 && gto <= to + mHL/2)
							{
								if(gene[j].type[h] == 1 || gene[j].type[h] == 2 || RANDOMIZE) k= 4;// Embedded in published
								else k= 7;	// Replacing published
							hss[o[i]].type= k;
							j= ncds;
							h= nex;
							}
							else
							{
								if(gene[j].type[h] == 1 || gene[j].type[h] == 2 || RANDOMIZE)
								{
								out= 0;
									if(gfrom < from) out += from - gfrom;
									if(gto > to) out += gto - to;

								hss[o[i]].gsuper += gto - gfrom + 1 - out;

									if(hss[o[i]].gsuper * 2 > gto - gfrom + 1) k= 6;  // Overlapped to published
									else if(hss[o[i]].type != 7) k= 0;
								}
								else k= 7; 	// Replacing excluded published
							hss[o[i]].type= k;
							}
						}
					}
				}
			}

// Checks overlap with higher-scoring HSSs

			for(j= 0; j < i && hss[o[i]].type == 0; ++j)
			{
			out= 0;
				if(hss[o[j]].type <= 3)
				{
					if(hss[o[j]].strand == 'D') { from= hss[o[j]].fromp; to= hss[o[j]].stop2; }
					else                        { from= hss[o[j]].stop1; to= hss[o[j]].top; }
	
					if(!(to < gfrom + mHL/2 - 1 || from > gto - mHL/2 + 1))  // Two newly predicted genes are overlapping
					{
						if(hss[o[i]].frame != hss[o[j]].frame)
						{
							if(gfrom >= from - mHL/2 && gto <= to + mHL/2)
							{
							k= 11;
							hss[o[i]].type= k;
							}
							else
							{
								if(gfrom < from) out += from - gfrom;
								if(gto > to) out += gto - to;
								if(gto - gfrom + 1 > 2 * out) k= 12;
								else k= hss[o[i]].type;
							hss[o[i]].type= k;
							}
						}
					}
				}
			}
		}
	}

fprintf(stderr,"\b\b\b\b100%%");

// Renumbers hits within each orf including only those of type 5:

renumber_orf_hits();
			
free(o);
}

/*** End of function process_hss() ***/

/*******************************************/
/********* Function write_results() ********/
/*******************************************/

void write_results(int from_hss, int to_hss, int ncds, int genome_size)
{
int	i, j, k, h, l, *o, s1, s2, types[13]= { 0 }, from, to, out;
char	Pg[15], name[50];
double	G, print_len, po;
FILE	*output1, *output2, *output3, *output4, *output5, *output7, *output8;

output1= fopen(Output_name + 100, "w");
output2= fopen(Output_name + 2 * 100, "w");
output3= fopen(Output_name + 3 * 100, "a");
output4= fopen(Output_name + 4 * 100, "w");
output5= fopen(Output_name + 5 * 100, "w");

	if(WRITE_SEQUENCES)
	{
	output7= fopen(Output_name + 7 * 100, "w");
	output8= fopen(Output_name + 8 * 100, "w");
	}

// Sort hits by position:

o= (int *)malloc(to_hss * sizeof(int));

	for(i= 0; i < to_hss; ++i) o[i]= i;

	for(i= 0; i < to_hss - 1; ++i)
		for(j= i + 1; j < to_hss; ++j)
			if(hss[o[i]].fromp > hss[o[j]].fromp) { k= o[i]; o[i]= o[j]; o[j]= k; }

// Print results:

// output1 = *.newcds    ( newly annotated CDSs)
// output2 = *.profiles  ( all hits (hss + G-test ) )
// output3 = *.verbose   ( detailed features of all annotations and hits )
// output4 = *.altcds    ( replacing published CDSs and overlapping CDSs )

fprintf(output3,"\nNEW ANNOTATION\n\nORF#\tName\tORF-fm\tORF-to\tORF-len\tStrand\tColor\tHit#\tHIT-fm\tHIT-to\tHIT-len\tEntropy\tScore\tProb\tG-test (Significance)\tSigLen\tPrediction type\n");

	for(i= 0; i < to_hss; ++i)
	{
	k= hss[o[i]].type;
	G= hss[o[i]].G;
	strcpy(Pg, hss[o[i]].pstring);
	s1= s2= 1;

		if(hss[o[i]].strand == 'D')
		{
			if(hss[o[i]].stop2 + 4 <= genome_size) s2 += 3;
		}
		else
		{
			if(hss[o[i]].stop1 - 2 > 0) s1 -= 3;
		}

		if(k == 0 || k == 7)
		{
                	if(hss[o[i]].global_sig == 1) sprintf(name, "%cn%d*", hss[o[i]].hit_type, o[i]);
                	else                          sprintf(name, "%cn%d", hss[o[i]].hit_type, o[i]);
		}
		else sprintf(name, "-");

	po= pow(2.0, 127.0);

		if(hss[o[i]].sig_len < po) print_len= hss[o[i]].sig_len;
		else print_len= po;

	fprintf(output3,"%d.%d\t%s\t%d\t%d\t%d\t%c\t%c\t%d\t%d\t%d\t%d\t%.2f\t%.2f\t%.4f\t%6.2f %s\t%.5e\t%c-%s", hss[o[i]].orf_num, hss[o[i]].frame, name, hss[o[i]].stop1+s1, hss[o[i]].stop2+s2, hss[o[i]].stop2-hss[o[i]].stop1+1, hss[o[i]].strand, hss[o[i]].color, hss[o[i]].hit_num, hss[o[i]].fromp+1, hss[o[i]].top+1, hss[o[i]].top-hss[o[i]].fromp+1, hss[o[i]].entropy, hss[o[i]].score, hss[o[i]].prob, G, Pg, print_len, hss[o[i]].hit_type, Prediction[k]);
		if(hss[o[i]].entropy <= MAX_ENTROPY) fprintf(output3, " repetitive");

		if(k==3) fprintf(output3," %d",hss[o[i]].exten);

	fprintf(output3,"\n\tA= %5.2f  C= %5.2f  G= %5.2f  T= %5.2f  S= %5.2f  S1= %5.2f  S2= %5.2f  S3= %5.2f  R= %5.2f  R1= %5.2f  R2= %5.2f  R3= %5.2f\n", hss[o[i]].nuc[3*6+0], hss[o[i]].nuc[3*6+1], hss[o[i]].nuc[3*6+2], hss[o[i]].nuc[3*6+3],  hss[o[i]].nuc[3*6+4], hss[o[i]].nuc[0*6+4], hss[o[i]].nuc[1*6+4], hss[o[i]].nuc[2*6+4], hss[o[i]].nuc[3*6+5], hss[o[i]].nuc[0*6+5], hss[o[i]].nuc[1*6+5], hss[o[i]].nuc[2*6+5]);

		if(k != 5)
		{
			if(k == 0)
			{
				if(hss[o[i]].hit_num == 1)
				{
					if(hss[o[i]].global_sig == 1) sprintf(name, "%cn%d*", hss[o[i]].hit_type, o[i]);
					else                          sprintf(name, "%cn%d", hss[o[i]].hit_type, o[i]);

					if(hss[o[i]].strand == 'D')
					{
						if(hss[o[i]].entropy > MAX_ENTROPY)
						{
							if(hss[o[i]].start == 1)
							{
							strcat(name, "++");
							fprintf(output1,"%-11s %d..%d\n", name, hss[o[i]].atg + s1, hss[o[i]].stop2 + s2);
								if(WRITE_SEQUENCES)
								{
								fprintf(output7,">%s %d..%d\n", name, hss[o[i]].atg + s1, hss[o[i]].stop2 + s2);
								fprintf(output8,">%s %d..%d\n", name, hss[o[i]].atg + s1, hss[o[i]].stop2 + s2);
								print_nucleotides(hss[o[i]].seq, hss[o[i]].seqlen, output7);
								print_amino_acids(hss[o[i]].seq, hss[o[i]].seqlen, output8);
								}
							}
							else if(hss[o[i]].start == 3)
							{
							strcat(name, "+");
							fprintf(output1,"%-11s %d..%d\n", name, hss[o[i]].atg + s1, hss[o[i]].stop2 + s2);
								if(WRITE_SEQUENCES)
								{
								fprintf(output7,">%s %d..%d\n", name, hss[o[i]].atg + s1, hss[o[i]].stop2 + s2);
								fprintf(output8,">%s %d..%d\n", name, hss[o[i]].atg + s1, hss[o[i]].stop2 + s2);
								print_nucleotides(hss[o[i]].seq, hss[o[i]].seqlen, output7);
								print_amino_acids(hss[o[i]].seq, hss[o[i]].seqlen, output8);
								}
							}
							else if(hss[o[i]].start == 2)
							{
							strcat(name, "--");
							fprintf(output1,"%-11s %d..%d\n", name, hss[o[i]].gtg + s1, hss[o[i]].stop2 + s2);
								if(WRITE_SEQUENCES)
								{
								fprintf(output1,">%s %d..%d\n", name, hss[o[i]].gtg + s1, hss[o[i]].stop2 + s2);
								fprintf(output1,">%s %d..%d\n", name, hss[o[i]].gtg + s1, hss[o[i]].stop2 + s2);
								print_nucleotides(hss[o[i]].seq, hss[o[i]].seqlen, output7);
								print_amino_acids(hss[o[i]].seq, hss[o[i]].seqlen, output8);
								}
							}
							else if(hss[o[i]].start == 4)
							{
							strcat(name, "-");
							fprintf(output1,"%-11s %d..%d\n", name, hss[o[i]].gtg + s1, hss[o[i]].stop2 + s2);
								if(WRITE_SEQUENCES)
								{
								fprintf(output1,">%s %d..%d\n", name, hss[o[i]].gtg + s1, hss[o[i]].stop2 + s2);
								fprintf(output1,">%s %d..%d\n", name, hss[o[i]].gtg + s1, hss[o[i]].stop2 + s2);
								print_nucleotides(hss[o[i]].seq, hss[o[i]].seqlen, output7);
								print_amino_acids(hss[o[i]].seq, hss[o[i]].seqlen, output8);
								}
							}
							else if(hss[o[i]].start == 5)
							{
							strcat(name, "==");
							fprintf(output1,"%-11s %d..%d\n", name, hss[o[i]].ttg + s1, hss[o[i]].stop2 + s2);
								if(WRITE_SEQUENCES)
								{
								fprintf(output1,">%s %d..%d\n", name, hss[o[i]].ttg + s1, hss[o[i]].stop2 + s2);
								fprintf(output1,">%s %d..%d\n", name, hss[o[i]].ttg + s1, hss[o[i]].stop2 + s2);
								print_nucleotides(hss[o[i]].seq, hss[o[i]].seqlen, output7);
								print_amino_acids(hss[o[i]].seq, hss[o[i]].seqlen, output8);
								}
							}
							else if(hss[o[i]].start == 6)
							{
							strcat(name, "=");
							fprintf(output1,"%-11s %d..%d\n", name, hss[o[i]].ttg + s1, hss[o[i]].stop2 + s2);
								if(WRITE_SEQUENCES)
								{
								fprintf(output1,">%s %d..%d\n", name, hss[o[i]].ttg + s1, hss[o[i]].stop2 + s2);
								fprintf(output1,">%s %d..%d\n", name, hss[o[i]].ttg + s1, hss[o[i]].stop2 + s2);
								print_nucleotides(hss[o[i]].seq, hss[o[i]].seqlen, output7);
								print_amino_acids(hss[o[i]].seq, hss[o[i]].seqlen, output8);
								}
							}
							else
							{
							fprintf(output1,"%-11s %d..%d\n", name, hss[o[i]].fromp + s1, hss[o[i]].stop2 + s2);
								if(WRITE_SEQUENCES)
								{
								fprintf(output7,">%s %d..%d\n", name, hss[o[i]].fromp + s1, hss[o[i]].stop2 + s2);
								fprintf(output8,">%s %d..%d\n", name, hss[o[i]].fromp + s1, hss[o[i]].stop2 + s2);
								print_nucleotides(hss[o[i]].seq, hss[o[i]].seqlen, output7);
								print_amino_acids(hss[o[i]].seq, hss[o[i]].seqlen, output8);
								}
							}
						}
						else fprintf(output5,"%-11s %d..%d %d %.4f\n", name, hss[o[i]].fromp + s1, hss[o[i]].stop2 + s2, hss[o[i]].len, hss[o[i]].entropy);
// Length and Entropy to stdout:	fprintf(stdout,"%d\t%.5f\n", hss[o[i]].stop2 -  hss[o[i]].fromp + s2, hss[o[i]].entropy);
					++length[(hss[o[i]].stop2 + s2 - hss[o[i]].fromp - s1 + 1)/50];
					}
					else
					{
						if(hss[o[i]].entropy > MAX_ENTROPY)
						{
							if(hss[o[i]].start == 1)
							{
							strcat(name, "++");
							fprintf(output1,"%-11s complement(%d..%d)\n", name, hss[o[i]].stop1 + s1, hss[o[i]].atg + s2);
								if(WRITE_SEQUENCES)
								{
								fprintf(output7,">%s++ %d..%d\n", name, hss[o[i]].stop1 + s1, hss[o[i]].atg + s2);
								fprintf(output8,">%s++ %d..%d\n", name, hss[o[i]].stop1 + s1, hss[o[i]].atg + s2);
								print_nucleotides(hss[o[i]].seq, hss[o[i]].seqlen, output7);
								print_amino_acids(hss[o[i]].seq, hss[o[i]].seqlen, output8);
								}
							}
							else if(hss[o[i]].start == 3)
							{
							strcat(name, "+");
							fprintf(output1,"%-11s complement(%d..%d)\n", name, hss[o[i]].stop1 + s1, hss[o[i]].atg + s2);
								if(WRITE_SEQUENCES)
								{
								fprintf(output7,">%s+ %d..%d\n", name, hss[o[i]].stop1 + s1, hss[o[i]].atg + s2);
								fprintf(output8,">%s+ %d..%d\n", name, hss[o[i]].stop1 + s1, hss[o[i]].atg + s2);
								print_nucleotides(hss[o[i]].seq, hss[o[i]].seqlen, output7);
								print_amino_acids(hss[o[i]].seq, hss[o[i]].seqlen, output8);
								}
							}
							else if(hss[o[i]].start == 2)
							{
							strcat(name, "--");
							fprintf(output1,"%-11s complement(%d..%d)\n", name, hss[o[i]].stop1 + s1, hss[o[i]].gtg + s2);
								if(WRITE_SEQUENCES)
								{
								fprintf(output7,">%s-- %d..%d\n", name, hss[o[i]].stop1 + s1, hss[o[i]].gtg + s2);
								fprintf(output8,">%s-- %d..%d\n", name, hss[o[i]].stop1 + s1, hss[o[i]].gtg + s2);
								print_nucleotides(hss[o[i]].seq, hss[o[i]].seqlen, output7);
								print_amino_acids(hss[o[i]].seq, hss[o[i]].seqlen, output8);
								}
							}
							else if(hss[o[i]].start == 4)
							{
							strcat(name, "-");
							fprintf(output1,"%-11s complement(%d..%d)\n", name, hss[o[i]].stop1 + s1, hss[o[i]].gtg + s2);
								if(WRITE_SEQUENCES)
								{
								fprintf(output7,">%s- %d..%d\n", name, hss[o[i]].stop1 + s1, hss[o[i]].gtg + s2);
								fprintf(output8,">%s- %d..%d\n", name, hss[o[i]].stop1 + s1, hss[o[i]].gtg + s2);
								print_nucleotides(hss[o[i]].seq, hss[o[i]].seqlen, output7);
								print_amino_acids(hss[o[i]].seq, hss[o[i]].seqlen, output8);
								}
							}
							else if(hss[o[i]].start == 5)
							{
							strcat(name, "==");
							fprintf(output1,"%-11s complement(%d..%d)\n", name, hss[o[i]].stop1 + s1, hss[o[i]].ttg + s2);
								if(WRITE_SEQUENCES)
								{
								fprintf(output7,">%s- %d..%d\n", name, hss[o[i]].stop1 + s1, hss[o[i]].ttg + s2);
								fprintf(output8,">%s- %d..%d\n", name, hss[o[i]].stop1 + s1, hss[o[i]].ttg + s2);
								print_nucleotides(hss[o[i]].seq, hss[o[i]].seqlen, output7);
								print_amino_acids(hss[o[i]].seq, hss[o[i]].seqlen, output8);
								}
							}
							else if(hss[o[i]].start == 6)
							{
							strcat(name, "=");
							fprintf(output1,"%-11s complement(%d..%d)\n", name, hss[o[i]].stop1 + s1, hss[o[i]].ttg + s2);
								if(WRITE_SEQUENCES)
								{
								fprintf(output7,">%s- %d..%d\n", name, hss[o[i]].stop1 + s1, hss[o[i]].ttg + s2);
								fprintf(output8,">%s- %d..%d\n", name, hss[o[i]].stop1 + s1, hss[o[i]].ttg + s2);
								print_nucleotides(hss[o[i]].seq, hss[o[i]].seqlen, output7);
								print_amino_acids(hss[o[i]].seq, hss[o[i]].seqlen, output8);
								}
							}
							else
							{
							fprintf(output1,"%-11s complement(%d..%d)\n", name, hss[o[i]].stop1 + s1, hss[o[i]].top + s2);
								if(WRITE_SEQUENCES)
								{
								fprintf(output7,">%s complement(%d..%d)\n", name, hss[o[i]].stop1 + s1, hss[o[i]].top + s2);
								fprintf(output8,">%s complement(%d..%d)\n", name, hss[o[i]].stop1 + s1, hss[o[i]].top + s2);
								print_nucleotides(hss[o[i]].seq, hss[o[i]].seqlen, output7);
								print_amino_acids(hss[o[i]].seq, hss[o[i]].seqlen, output8);
								}
							}
						}
						else fprintf(output5,"%-11s complement(%d..%d) %d %.4f\n", name, hss[o[i]].stop1 + s1, hss[o[i]].top + s2, hss[o[i]].len, hss[o[i]].entropy);
// Length and Entropy to stdout:	fprintf(stdout,"%d\t%.5f\n", hss[o[i]].top - hss[o[i]].stop1 + 4, hss[o[i]].entropy);
					++length[(hss[o[i]].top + s2 - hss[o[i]].stop1 - s1 + 1)/50];
					}
				}
			}
			else if(k == 7)
			{
				if(hss[o[i]].hit_num == 1)
				{
					if(hss[o[i]].global_sig == 1) sprintf(name, "%cn%d*", hss[o[i]].hit_type, o[i]);
					else                          sprintf(name, "%cn%d", hss[o[i]].hit_type, o[i]);

					if(hss[o[i]].strand == 'D') fprintf(output4,"%-11s %d..%d\n", name, hss[o[i]].fromp + s1, hss[o[i]].stop2 + s2);
					else                        fprintf(output4,"%-11s complement(%d..%d)\n", name, hss[o[i]].stop1 + s1, hss[o[i]].top + s2);
				}
			}

			if(hss[o[i]].strand == 'D') fprintf(output2,"%d..%d\n", hss[o[i]].fromp + 1, hss[o[i]].top + 1);
			else                        fprintf(output2,"complement(%d..%d)\n", hss[o[i]].fromp + 1, hss[o[i]].top + 1);
		}
	}
fprintf(output3,"\n");

fprintf(output3,"Prediction type            Tot hits\n");

	for(i= 0; i < from_hss; ++i) ++types[hss[o[i]].type];
	for(k= 0; k < 13; ++k) { fprintf(output3,"H-%-23s  %d\n",Prediction[k],types[k]); types[k]= 0; }
	for(i= from_hss; i < to_hss; ++i) ++types[hss[o[i]].type];
	for(k= 0; k < 13; ++k) fprintf(output3,"G-%-23s  %d\n",Prediction[k],types[k]);

fprintf(output3,"\n");

fclose(output1);
fclose(output2);
fclose(output3);
fclose(output4);
fclose(output5);

	if(WRITE_SEQUENCES)
	{
	fclose(output7);
	fclose(output8);
	}

free(o);
}

/*** End of function write_results() ***/


/*******************************/
/**** function find_Ghits() ****/
/*******************************/

// Requires global FILE *fp

int find_Ghits(int genome_size, int tot_hss, long bytes_from_origin, int *on)
{
int tot_Ghits= tot_hss, cod, i0= 0, i, j, n[6]= {0}, k, *o, g0= 0, g1= 0, g2, g3= 0, h0, h1, flag, frame, stop, len;

o= (int *)malloc(tot_hss * sizeof(int));
hss= (struct HSSs *)realloc(hss,(tot_hss+1)*sizeof(struct HSSs));
tot_Ghits= tot_hss;
i0= 0;
	
	for(i= 0; i < tot_hss; ++i) o[i]= i;

// Moves to the left of the array all "Excluded hits" ( type= 5 ). i0 is the starting position of all ( type != 5 ) hits.
	
	for(i= 0; i < tot_hss - 1; ++i)
		for(j= i + 1; j < tot_hss; ++j)
			if(hss[o[j]].type == 5)
			{
			i0= i + 1;
			k= o[i]; o[i]= o[j]; o[j]= k;
			}

// Orders all ( type != 5 ) hits by position in the genome
	
	for(i= i0; i < tot_hss-1; ++i)
		for(j= i+1; j < tot_hss; ++j)
			if(hss[o[j]].top < hss[o[i]].top) { k= o[i]; o[i]= o[j]; o[j]= k; }
	
j= 0;
	
	while(g1 < genome_size - 1)
	{
	g1= genome_size - 1;
		
		for(i= i0; i < tot_hss; ++i) if(hss[o[i]].top < g0) j= i + 1;

		do
		{
		flag= 0;
			for(i= j; i < tot_hss; ++i)
			{
				if(hss[o[i]].fromp <= g0 + mHL && hss[o[i]].top >= g0)
				{
				g0= hss[o[i]].top + 1;
				j= i + 1;
				flag= 1;
				}
			}
		}
		while(flag);
		
		for(i= j; i < tot_hss; ++i)
			if(hss[o[i]].fromp > g0 && hss[o[i]].fromp - 1 < g1) { g1= hss[o[i]].fromp - 1; g2= hss[o[i]].top + 1; }
		
		if(g1 - g0 + 1  >= mHL)
		{
		fprintf(stderr,"\b\b\b\b%3d%%",(int)((double)g0 / ((double)genome_size) * 100.0) );

		get_sequence(g0 - g3 + 1, g1 - g0 + 1, 'D', ORF);

			for(k= 0; k < 3; ++k)		// DIRECT STRAND
			{
			h0= k;
			frame= (g0 + h0 + 2) % 3;
			len= g1 - g0 + 1 - k;

				for(i= k; i < len - 2; i += 3)
				{
				cod= 16 * ORF[i] + 4 * ORF[i+1] + ORF[i+2];
            				if(cod == 48 || cod == 50 || ( cod == 56 && !MYCOPLASMA) || i >= len - 5)
					{
					h1= i - 1;
            					if(cod == 48 || cod == 50 || ( cod == 56 && !MYCOPLASMA)) stop= 1;
            					else { stop= 0; h1 += 3; }
						if(h1 - h0 + 1 >= mHL)
						{
							while(orf_num[frame][n[frame]][1] < g0 + h0) ++n[frame];

                                                        if(RANDOMIZE)
                                                        {
                                                        copy_sequence(ORF + h0, h1 - h0 + 1, ORF + g1 - g0 + 1);
                                                        shuffle(ORF + g1 - g0 + 1, h1 - h0 + 1, 1);
							tot_Ghits= maxG_test(ORF + g1 - g0 + 1, h1 - h0 + 1, g0 + h0, 'D', frame, n[frame], tot_Ghits, on);
                                                        }
							else tot_Ghits= maxG_test(ORF + h0, h1 - h0 + 1, g0 + h0, 'D', frame, n[frame], tot_Ghits, on);
						}

					h0= h1 + 4;
					}
					else h1= i + 2;
				}
			}

			complement(ORF, g1 - g0 + 1);

			for(k= 0; k < 3; ++k)		// COMPLEMENTARY STRAND
			{
			h0= k;
			frame= (g1 - h0 - 2) % 3 + 3;
			len= g1 - g0 + 1 - k;
				while(n[frame] < tot_orfs[frame] - 1 && orf_num[frame][n[frame]][1] < g1 - h0) ++n[frame];

				for(i= k; i < len - 2; i += 3)
				{
				cod= 16 * ORF[i] + 4 * ORF[i+1] + ORF[i+2];
            				if(cod == 48 || cod == 50 || ( cod == 56 && !MYCOPLASMA) || i >= len - 5)
					{
					h1= i - 1;
            					if(cod == 48 || cod == 50 || ( cod == 56 && !MYCOPLASMA)) stop= 1;
            					else { stop= 0; h1 += 3; }
						if(h1 - h0 + 1 >= mHL)
						{
							while(n[frame]>0 && orf_num[frame][n[frame]][0] > g1 - h0) --n[frame];

                                                        if(RANDOMIZE)
                                                        {
                                                        copy_sequence(ORF + h0, h1 - h0 + 1, ORF + g1 - g0 + 1);
                                                        shuffle(ORF + g1 - g0 + 1, h1 - h0 + 1, 1);
							tot_Ghits= maxG_test(ORF + g1 - g0 + 1, h1 - h0 + 1, g1 - h0, 'C', frame, n[frame], tot_Ghits, on);
                                                        }
							else tot_Ghits= maxG_test(ORF + h0, h1 - h0 + 1, g1 - h0, 'C', frame, n[frame], tot_Ghits, on);
						}
					h0= h1 + 4;
					}
					else h1= i + 2;
				}
			}

		g3= g1 + 1;
		}

	g0= g2;
	}
	
free(o);
	
fprintf(stderr,"\b\b\b\b100%%\n");
fprintf(stdout,"\nTotal G-hits: %d", tot_Ghits - tot_hss);
	
return(tot_Ghits);
}

/*** End of function find_Ghits() ****/

/*************************************************/
/**** Function characterize_published_genes() ****/
/*************************************************/

// Requires global FILE *fp

void characterize_published_genes(int ncds, int tot_Ghits, double nuc[], long bytes_from_origin, long *ffrom)
{
int	i, j, l, k, h, from, to, s1, s2, out;
char	Pg[15];
double	G;
FILE	*output1, *output2, *output3;

output1= fopen(Output_name, "w");
output2= fopen(Output_name + 6 * 100, "w");
output3= fopen(Output_name + 3 * 100, "w");

// output1 = *.modified ( published CDSs modified)
// output2 = *.confirmed ( published CDSs confirmed)
// output3 = *.verbose   ( detailed features of all annotations and hits )

fprintf(output1,"List of published CDSs with modified annotation: NS= Not supported; MO= Modified; CO= Contradicted.\n");
fprintf(output2,"List of published CDSs confirmed by this analysis (p <= %.5f) (coordinates from start of first hit to stop codon)\n", SIGNIFICANCE);
fprintf(output3,"PUBLISHED ANNOTATION\n\nNo\tGene\tCDS-fm\tCDS-to\tCDS-len\tStrand\tColor\tEntropy\tG-test (significance)\tCharacterization\n");

	for(i= 0; i < tot_Ghits; ++i)
	{
		if(hss[i].strand == 'D') hss[i].color= RGB[hss[i].top % 3];
		else                     hss[i].color= RGB[hss[i].fromp % 3];
	}

	for(j= 0; j < ncds; ++j)
	{
	fprintf(stderr,"\b\b\b\b%3.0f%%",(float)(j+1)/((float)ncds)*100.0);
	l= 0;

// Assembles exons

		for(h= 0; h < gene[j].num_exons; ++h)
		{
		fseek(fp, ffrom[gene[j].fpos[h]], SEEK_SET);
		s1= s2= 1;

        		if(gene[j].strand == 'D') { from= gene[j].start[h]; to= gene[j].end[h]; if(h == gene[j].num_exons - 1) s2= 4; }
        		else                      { to= gene[j].start[h]; from= gene[j].end[h]; if(h == gene[j].num_exons - 1) s1= -2; }

		get_sequence(1, to - from + 1, gene[j].strand, ORF + l);

		gene[j].entropy= entropy(ORF, to - from + 1);

		G= G_test(ORF + l, to - from + 1, Pg, &k);  // Multi-exon structure implemented!!
		gene[j].type[h]= k;

		composition(ORF + l, to - from + 1, nuc, l % 3);   // Records the composition of the segment as A, C G, T, S, R in codon positions 1, 2, 3 and overall.
		l += to - from + 1;

			for(i= 0; i < tot_Ghits; ++i)
			{
				if(hss[i].type != 5)
				{
					if(!(hss[i].top < from + mHL/2 - 1 || hss[i].fromp > to - mHL/2 + 1))  // if CDS is superimposed to hit
					{
						if(hss[i].frame == gene[j].frame[h])	// same frame overlapped
						{
							if(hss[i].fromp >= from - mHL/2 && hss[i].top <= to + mHL/2)  // hit embedded
							{
								if(gene[j].type[h] != 2) gene[j].type[h]= k= 1;
							hss[i].type= 2;
							}
							else    // hit overlapped
							{
							gene[j].type[h]= k= 2;
							hss[i].type= 3;
								if(gene[j].strand == 'D')
								{
									if(from > hss[i].fromp && to < hss[i].top)
									{
									hss[i].exten= from - hss[i].fromp + hss[i].top - to;
									from= gene[j].newstart[h]= hss[i].fromp;
									to= gene[j].newend[h]= hss[i].top;
									}
									else if(from > hss[i].fromp)
									{
									hss[i].exten= from - hss[i].fromp;
									from= gene[j].newstart[h]= hss[i].fromp;
									}
									else
									{
									hss[i].exten= hss[i].top - to;
									to= gene[j].newend[h]= hss[i].top;
									}
								}
								else
								{
									if(from > hss[i].fromp && to < hss[i].top)
									{
									hss[i].exten= from - hss[i].fromp + hss[i].top - to;
									from= gene[j].newend[h]= hss[i].fromp;
									to= gene[j].newstart[h]= hss[i].top;
									}
									else if(from > hss[i].fromp)
									{
									hss[i].exten= from - hss[i].fromp;
									from= gene[j].newend[h]= hss[i].fromp;
									}
									else
									{
									hss[i].exten= hss[i].top - to;
									to= gene[j].newstart[h]= hss[i].top;
									}
								}
							}
						}
						else if(gene[j].type[h] == 0 || gene[j].type[h] == 4)  // superimposed in different frame and not previously classified
						{
						gene[j].type[h]= k= 3;
						out= 0;
							if(hss[i].fromp < from) out += from - hss[i].fromp;
							if(hss[i].top > to) out += hss[i].top - to;
						hss[i].hsuper += hss[i].top - hss[i].fromp + 1 - out;
						out= hss[i].top - hss[i].fromp + 1 - hss[i].hsuper;
							if(out > mHL) hss[i].type= 6;
							else          hss[i].type= 4;
						}
					}
				}
			}
		write_published_exon(j, h, k, from, to, s1, s2, G, Pg, nuc, output1, output2, output3);
		}
	}

fclose(output1);
fclose(output2);
fclose(output3);
fseek(fp, bytes_from_origin, SEEK_SET);
}

/**** End of function characterize_published_genes() ****/

/*************************************************/
/**** Function write_published_exon() ****/
/*************************************************/

// Requires global FILE *fp

void write_published_exon(int j, int h, int k, int from, int to, int s1, int s2, double G, char Pg[], double nuc[], FILE *output1, FILE *output2, FILE *output3)
{
char	name[50];

// output1 = *.modified ( published CDSs modified)
// output2 = *.confirmed   ( pulished CDSs confirmed )
// output3 = *.verbose   ( detailed features of all annotations and hits )

fprintf(output3,"%d\t%s",j+1, gene[j].name);
	if(gene[j].num_exons > 1) fprintf(output3,".%d",h+1);
fprintf(output3, "\t%d\t%d\t%d\t%c\t%c\t%.2f\t%6.2f %s\t%s", from + s1, to + s2, to - from + 4, gene[j].strand, gene[j].color[h], gene[j].entropy, G, Pg, Annotation[k]);
	if(gene[j].entropy <= MAX_ENTROPY) fprintf(output3, " repetitive");
fprintf(output3, "\n\tA= %5.2f  C= %5.2f  G= %5.2f  T= %5.2f  S= %5.2f  S1= %5.2f  S2= %5.2f  S3= %5.2f  R= %5.2f  R1= %5.2f  R2= %5.2f  R3= %5.2f\n", nuc[3*6+0], nuc[3*6+1], nuc[3*6+2], nuc[3*6+3], nuc[3*6+4], nuc[0*6+4], nuc[1*6+4], nuc[2*6+4], nuc[3*6+5], nuc[0*6+5], nuc[1*6+5], nuc[2*6+5]);

	if(k == 0 || k == 2 || k == 3)
	{
		if(strlen(gene[j].name))
		{
			if(k == 0) sprintf(name, "NS_");
			else if(k == 2) sprintf(name, "MO_");
			else if(k == 3) sprintf(name, "CO_");
		strcat(name, gene[j].name);
		}
		else
		{
			if(k == 0) sprintf(name, "NS%d", j+1);
			else if(k == 2) sprintf(name, "MO%d", j+1);
			else if(k == 3) sprintf(name, "CO%d", j+1);
		}
			
		if(gene[j].strand == 'D') fprintf(output1,"%-11s %d..%d\n", name, from + s1, to + s2);
		else                      fprintf(output1,"%-11s complement(%d..%d)\n", name, from + s1, to + s2);
	}
	else
	{
		if(gene[j].strand == 'D') fprintf(output2,"%-11s %d..%d\n", gene[j].name, from + s1, to + s2);
		else                      fprintf(output2,"%-11s complement(%d..%d)\n", gene[j].name, from + s1, to + s2);
	}
}

/**** End of function print_exon() ****/


/*********************************************/
/**** Function define_characterizations() ****/
/*********************************************/

void define_characterizations()
{
strcpy(Prediction[0],"Not-superimp");
strcpy(Prediction[1],"Identical to annotated");
strcpy(Prediction[2],"Part of annotated");	// Same strand and frame than annotated
strcpy(Prediction[3],"Extending annotated");
strcpy(Prediction[4],"Embedded in annotated");  // Different frame or strand than annotated
strcpy(Prediction[5],"Excluded");
strcpy(Prediction[6],"Overlapped to annotated");
strcpy(Prediction[7],"Replacing excluded");

strcpy(Prediction[8],"Identical to predicted");
strcpy(Prediction[9],"Within predicted");
strcpy(Prediction[10],"Extending predicted");
strcpy(Prediction[11],"Embedded in predicted");
strcpy(Prediction[12],"Overlapped to predicted");

strcpy(Annotation[0],"Not supported");
strcpy(Annotation[1],"Confirmed");
strcpy(Annotation[2],"Modified");
strcpy(Annotation[3],"Contradicted");
strcpy(Annotation[4],"Contrasted");
}

/**** End of function define_characterizations() ****/

/****************************/
/**** Function shuffle() ****/
/****************************/

void shuffle(char *seq, int n, int remove_stops)
{
int	i, j, k, cod;
char	c;
double	r;

// Shuffles sequence:

	for(i = n - 1; i > 0; i--)
	{
		while((r = drand48()) >= 1.0);
			j = (int)(r * (double)(i + 1));
			c = seq[i]; seq[i] = seq[j]; seq[j] = c;
	}

// Removes stop codons shuffling codon

	if(remove_stops)
		for(i = n - 3; i >= 0; i -= 3)
		{
		cod= 16 * seq[i] + 4 * seq[i + 1] + seq[i + 2];
			if(cod == 48 || cod == 50 || (cod == 56 && !MYCOPLASMA)) shuffle(seq + i, 3, 1);
		}
}

/**** End of function shuffle() ****/

/****************************/
/*** Function position() ****/
/****************************/

int position(int a, int c, int g, int t)
{
int     n, pos;

n= a + c + g + t;

pos= a * ( a * (a - 6) + 11 + n * (3 * n + 12 - 3 * a) ) / 6;
pos += c * ( n - a + 1) - c * ( c - 1 ) / 2 + g;

return(pos);
}

/*** End of function position() ****/

/*******************************/
/*** Function read_tables()  ***/
/*******************************/

void read_tables()
{
int	i, j;
char	input_file[200], longstr[1000];
FILE	*input;

// Reads threshold values at p= 0.01

strcpy(input_file, BASE_DIR_THRESHOLD_TABLES);
strcat(input_file, "scores100.table");

	if((input= fopen(input_file,"r"))==NULL) { fprintf(stderr,"\n\nInput table %s not found.\n",input_file); exit(1); }

fgets(longstr, 998, input);

	for(i= 0; i < 23426; ++i)
	{
	fgets(longstr, 998, input);
	sscanf(longstr + 39 * LEN_TRANSFORM,"%f %f", threshold[0][i], threshold[0][i] + 1);
	}

fclose(input);

// Reads threshold values at p= 0.001

strcpy(input_file, BASE_DIR_THRESHOLD_TABLES);
strcat(input_file, "scores1000.table");

	if((input= fopen(input_file,"r"))==NULL) { fprintf(stderr,"\n\nInput table %s not found.\n",input_file); exit(1); }

fgets(longstr, 998, input);

	for(i= 0; i < 23426; ++i)
	{
	fgets(longstr, 998, input);
	sscanf(longstr + 39 * LEN_TRANSFORM,"%f %f", threshold[1][i], threshold[1][i] + 1);
	}

fclose(input);

// Reads threshold values at p= 0.0001

strcpy(input_file, BASE_DIR_THRESHOLD_TABLES);
strcat(input_file, "scores10000.table");

	if((input= fopen(input_file,"r"))==NULL) { fprintf(stderr,"\n\nInput table %s not found.\n",input_file); exit(1); }

fgets(longstr, 998, input);

	for(i= 0; i < 23426; ++i)
	{
	fgets(longstr, 998, input);
	sscanf(longstr + 39 * LEN_TRANSFORM,"%f %f", threshold[2][i], threshold[2][i] + 1);
	}

fclose(input);
}

/*** End of function read_tables() ***/

/*******************************************/
/*** Function assign_gene_positions() ******/
/*******************************************/

void assign_gene_positions(long *ffrom, int *o, int ncds, int nexons)
{
int	i, j, k, h;

	for(i= 0; i < nexons; ++i) o[i]= i;

k= 0;

	for(i= 0; i < ncds; ++i)
	{
		for(j= 0; j < gene[i].num_exons; ++j)
		{
			if(gene[i].strand == 'D') ffrom[k]= (long)gene[i].start[j] - 1;
			else                      ffrom[k]= (long)gene[i].end[j] - 1;
		gene[i].fpos[j]= k;
		++k;
		}
	}

	for(i= 0; i < k - 1; ++i)
		for(j= i + 1; j < k; ++j)
			if(ffrom[o[j]] < ffrom[o[i]]) { h= o[i]; o[i]= o[j]; o[j]= h; }
}

/*** End of function assign_gene_positions() ******/

/**********************************/
/****  Function reorder_hits() ****/
/**********************************/

void reorder_hits(int n, int orfn)
{
int	j, frame;

frame= hss[n].frame;

hss[n].hit_num= 1;
hss[n].previous_hit= -1;
hss[n].next_hit= -1;


j= orf_num[frame][orfn][2];

	if(j != -1)
	{
		if(hss[n].strand == 'D')
		{
			while(j != -1)
			{
				if(hss[j].fromp < hss[n].fromp)
				{
				++hss[n].hit_num;
				hss[n].previous_hit= j;
				j= hss[j].next_hit;
				}
				else if(hss[j].fromp > hss[n].fromp)
				{
				hss[n].next_hit= j;
				hss[j].previous_hit= n;
				j= -1;
				}
			}
		}
		else
		{
			while(j != -1)
			{
				if(hss[j].fromp > hss[n].fromp)
				{
				++hss[n].hit_num;
				hss[n].previous_hit= j;
				j= hss[j].next_hit;
				}
				else if(hss[j].fromp < hss[n].fromp)
				{
				hss[n].next_hit= j;
				j= -1;
				}
			}

		}
	j= hss[n].previous_hit;
		if(j != -1) hss[j].next_hit= n;
	j= hss[n].next_hit;
		while(j != -1)
		{
		++hss[j].hit_num;
		j= hss[j].next_hit;
		}
	}
	else orf_num[frame][orfn][2]= n;
}

/****  End of function reorder_hits() ****/

/**********************************/
/****  Function renumber_hits() ****/
/**********************************/

void renumber_orf_hits()
{
int	i, j, k, h, next;

	for(k= 0; k < 6; ++k)
	{
		for(i= 0; i < tot_orfs[k]; ++i)
		{
		j= orf_num[k][i][2];
		h= -1;
			while(j != -1)
			{
				if(hss[j].type == 5)
				{
				hss[j].hit_num= -1;
				j= hss[j].next_hit;
				}
				else
				{
				h= j;
				j= -1;
				}
			}
		j= orf_num[k][i][2]= h;
		h= 1;

			while(j != -1)
			{
			next= hss[j].next_hit;
				if(hss[j].type == 5)
				{
					if(hss[j].next_hit != -1) hss[hss[j].next_hit].previous_hit= hss[j].previous_hit;
					if(hss[j].previous_hit != -1) hss[hss[j].previous_hit].next_hit= hss[j].next_hit;
				hss[j].hit_num= -1;
				hss[j].next_hit= -1;
				hss[j].previous_hit= -1;
				}
				else
				{
				hss[j].hit_num= h;
				++h;
				}
			j= next;
			}
		}
	}
}

/****  End of function renumber_hits() ****/

/**********************************/
/****  Function check_lists() ****/
/**********************************/

void check_lists()
{
int	i, k, j, n= 0, h= 0;

fprintf(stdout,"Number of ORFS:");
	for(k= 0; k < 6; ++k) fprintf(stdout,"  %d", tot_orfs[k]);
fprintf(stdout,"\n\n");

	for(k= 0; k < 6; ++k)
	{
		for(i= 0; i < tot_orfs[k]; ++i)
		{
		j= orf_num[k][i][2];
			if(j != -1)
			{
			++n;
			++h;
			fprintf(stdout, "%d.\t%d.%d",n,k,i);
				while(j != -1)
				{
				fprintf(stdout, "  %d..%d(%c%d)",hss[j].fromp,hss[j].top,hss[j].hit_type,hss[j].type);
				j= hss[j].next_hit;
				++h;
				}
			fprintf(stdout,"\n");
			}
		}
	}
fprintf(stdout, "Total hits included: %d\n\n", h);
}

/****  end of function check_lists() ****/

/**********************************/
/****  Function print_nucleotides() ****/
/**********************************/

void	print_nucleotides(char *seq, int len, FILE *output)
{
int	i;

	for(i= 0; i < len; ++i)
	{
	fprintf(output, "%c", letters[seq[i]]);
		if(!((i + 1) % 60)) fprintf(output, "\n"); 
		else if(!((i + 1) % 3)) fprintf(output, " "); 
	}

	if(i % 60) fprintf(output, "\n");
}


/****  end of function print_nucleotides() ****/

/**********************************/
/****  Function print_amino_acids() ****/
/**********************************/

void	print_amino_acids(char *seq, int len, FILE *output)
{
int	i;

	for(i= 0; i < len / 3; ++i)
	{
	fprintf(output, "%c", aa_letters[16 * seq[3 * i] + 4 * seq[3 * i + 1] + seq[3 * i + 2]]);
		if(!((i + 1) % 60)) fprintf(output, "\n"); 
		else if(!((i + 1) % 10)) fprintf(output, " "); 
	}

	if(i % 60) fprintf(output, "\n");
}