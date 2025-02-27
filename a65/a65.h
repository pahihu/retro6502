
#define SFIELD	23	/* source line offset in print line buffer */
#define STABSZ	35000	/* default symbol table size */
#define SBOLSZ	16	/* maximum symbol size */

/* symbol flags */

#define DEFZRO	2	/* defined - page zero address	*/
#define MDEF	3	/* multiply defined		*/
#define UNDEF	1	/* undefined - may be zero page */
#define DEFABS	4	/* defined - two byte address	*/
#define UNDEFAB	5	/* undefined - two byte address */
#define PAGESIZE 60	/* number of lines on a page    */
#define LINESIZE 132	/* number of characters on a line */
#define TITLESIZE 80	/* maximum characters in title  */

/* pass flags */

#define FIRST_PASS	0
#define LAST_PASS	1
#define DONE		2

#define CPMEOF EOF

