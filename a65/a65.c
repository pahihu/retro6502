/*
 *  A65 - an assembler for the MOS Technology 6502 microprocessor
 *
 *  This assembler is a re-work of as6502 written by J. H. Van Ornum
 *  and modified by J. Swank.
 *
 *  To compile A65 under Borland Turbo-C 2.0 :
 *
 *    tcc a65.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "a65.h"

char	*pname  = "A65 v1.04";			/* program version */
char	*pdate  = "30-May-02"; 			/* program date */
char	*ptitle = "6502 cross-assembler";	/* default title */

FILE	*iptr;			/* source input file */
FILE	*optr;			/* hex output file */
FILE	*bptr;			/* binary output file */
int	errcnt;			/* error counter */
int	eflag;			/* errors flag */
int	warncnt;		/* warning counter */
int	gflag;			/* generate flag */
char	hex[5];			/* hexadecimal character buffer	*/
int	lablptr;		/* label pointer into symbol table */
int	lflag;			/* listing flag */
int	loccnt;			/* location counter */
int	nflag;			/* normal/split address mode */
int	mflag;			/* generate MOS Technology object format */
unsigned nxt_free;		/* next free location in symtab */
int	objcnt;			/* object byte counter */
int	oflag;			/* object output flag */
int	opflg;			/* operation code flags */
int	opval;			/* operation code value */
int	pass;			/* pass counter */
char	prlnbuf[LINESIZE+1];	/* print line buffer */
int	sflag;			/* symbol table output flag */
int	slnum;			/* source line number counter */
char	*symtab;		/* symbol table */
				/* struct sym_tab */
				/* 	char	size; */
				/*	char	chars[size]; */
				/*	char	flag; */
				/*	int	value; */
				/*	int	line defined */
				/*	char	# references */
				/*	int	line referenced	*/
unsigned size;			/* symbol table size */
char	symbol[SBOLSZ+1];	/* temporary symbol storage */
int	udtype;			/* undefined symbol type */
int	undef;			/* undefined symbol in expression flg */
int	value;			/* operand field value */
char	zpref;			/* zero page reference flag */
int	pagect;			/* count of pages */
int	paglin;			/* lines printed on current page */
int	pagesize;		/* maximum lines per page */
int	linesize;		/* maximum characters per line */
int	titlesize;		/* title string size */
char	titlbuf[LINESIZE+1];	/* buffer for title from .page */
int	badflag;
int	act;
char	**avt;
char	objrec[60];		/* buffer for object record */
int	objptr;			/* pointer to object record */
unsigned objloc;		/* object file location counter */
int	objbytes;		/* count of bytes in current record */
int	reccnt;			/* count of records in file */
int	cksum;			/* record check sum accumulator */
int	cflag;			/* symbol case sensitive flag */
int	endflag;		/* end assembly flag */
int	bflag;			/* binary output flag */
unsigned cbase;			/* code base */

/*********************************************************************/

/* operation code flags */

#define IMM1	0x1000		/* opval + 0x00	2 byte */
#define IMM2	0x0800		/* opval + 0x08	2 byte */
#define ABS	0x0400		/* opval + 0x0C	3 byte */
#define ZER	0x0200		/* opval + 0x04	2 byte */
#define INDX	0x0100		/* opval + 0x00	2 byte */
#define ABSY2	0x0080		/* opval + 0x1C	3 byte */
#define INDY	0x0040		/* opval + 0x10	2 byte */
#define ZERX	0x0020		/* opval + 0x14	2 byte */
#define ABSX	0x0010		/* opval + 0x1C	3 byte */
#define ABSY	0x0008		/* opval + 0x18	3 byte */
#define ACC	0x0004		/* opval + 0x08	1 byte */
#define IND	0x0002		/* opval + 0x2C	3 byte */
#define ZERY	0x0001		/* opval + 0x14	2 byte */

/* opcode classes */

#define PSEUDO	0x6000
#define CLASS1	0x2000
#define CLASS2	0x4000
#define CLASS3	ABS
#define CLASS4	ABS|IND
#define CLASS5	ABS|ZER
#define CLASS6	ABS|ZER|INDX|INDY|ZERX|ABSX|ABSY
#define CLASS7	ABS|ZER|ZERX
#define CLASS8	ABS|ZER|ZERX|ABSX
#define CLASS9	ABS|ZER|ZERX|ABSX|ACC
#define CLASS10	ABS|ZER|ZERY
#define CLASS11	IMM1|ABS|ZER
#define CLASS12	IMM1|ABS|ZER|ABSX|ZERX
#define CLASS13	IMM1|ABS|ZER|ABSY2|ZERY
#define CLASS14	IMM2|ABS|ZER|INDX|INDY|ZERX|ABSX|ABSY

#define A	0x20)+('A'&0x1f))
#define B	0x20)+('B'&0x1f))
#define C	0x20)+('C'&0x1f))
#define D	0x20)+('D'&0x1f))
#define E	0x20)+('E'&0x1f))
#define F	0x20)+('F'&0x1f))
#define G	0x20)+('G'&0x1f))
#define H	0x20)+('H'&0x1f))
#define I	0x20)+('I'&0x1f))
#define J	0x20)+('J'&0x1f))
#define K	0x20)+('K'&0x1f))
#define L	0x20)+('L'&0x1f))
#define M	0x20)+('M'&0x1f))
#define N	0x20)+('N'&0x1f))
#define O	0x20)+('O'&0x1f))
#define P	0x20)+('P'&0x1f))
#define Q	0x20)+('Q'&0x1f))
#define R	0x20)+('R'&0x1f))
#define S	0x20)+('S'&0x1f))
#define T	0x20)+('T'&0x1f))
#define U	0x20)+('U'&0x1f))
#define V	0x20)+('V'&0x1f))
#define W	0x20)+('W'&0x1f))
#define X	0x20)+('X'&0x1f))
#define Y	0x20)+('Y'&0x1f))
#define Z	0x20)+('Z'&0x1f))

#define OPSIZE	127

/* nmemonic operation code table
   entries consists of 3 fields - opcode name (hashed to a 16-bit
   value and sorted numerically), opcode class, opcode value. */

int	optab[]	=					/* '.' = 31
							   '*' = 30
							   '=' = 29 */
{
	((0*0x20)+(29)),PSEUDO,1,			/* 0x001d = */
	((((0*0x20)+(30))*0x20)+(29)),PSEUDO,3,		/* 0x03dd *= */
	((((((0*A*D*C,CLASS14,0x61,			/* 0x0483 ADC */
	((((((0*A*N*D,CLASS14,0x21,			/* 0x05c4 AND */
	((((((0*A*S*L,CLASS9,0x02,			/* 0x06c6 ASL */
	((((((0*B*C*C,CLASS2,0x90,			/* 0x0863 BCC */
	((((((0*B*C*S,CLASS2,0xb0,			/* 0x0873 BCS */
	((((((0*B*E*Q,CLASS2,0xf0,			/* 0x08b1 BEQ */
	((((((0*B*I*T,CLASS5,0x20,			/* 0x0934 BIT */
	((((((0*B*M*I,CLASS2,0x30,			/* 0x09a9 BMI */
	((((((0*B*N*E,CLASS2,0xd0,			/* 0x09c5 BNE */
	((((((0*B*P*L,CLASS2,0x10,			/* 0x0a0c BPL */
	((((((0*B*R*K,CLASS1,0x00,			/* 0x0a4b BRK */
	((((((0*B*V*C,CLASS2,0x50,			/* 0x0ac3 BVC */
	((((((0*B*V*S,CLASS2,0x70,			/* 0x0ad3 BVS */
	((((((0*C*L*C,CLASS1,0x18,			/* 0x0d83 CLC */
	((((((0*C*L*D,CLASS1,0xd8,			/* 0x0d84 CLD */
	((((((0*C*L*I,CLASS1,0x58,			/* 0x0d89 CLI */
	((((((0*C*L*V,CLASS1,0xb8,			/* 0x0d96 CLV */
	((((((0*C*M*P,CLASS14,0xc1,			/* 0x0db0 CMP */
	((((((0*C*P*X,CLASS11,0xe0,			/* 0x0e18 CPX */
	((((((0*C*P*Y,CLASS11,0xc0,			/* 0x0e19 CPY */
	((((((0*D*E*C,CLASS8,0xc2,			/* 0x10a3 DEC */
	((((((0*D*E*X,CLASS1,0xca,			/* 0x10b8 DEX */
	((((((0*D*E*Y,CLASS1,0x88,			/* 0x10b9 DEY */
	((((((0*E*O*R,CLASS14,0x41,			/* 0x15f2 EOR */
	((((((0*I*N*C,CLASS8,0xe2,			/* 0x25c3 INC */
	((((((0*I*N*X,CLASS1,0xe8,			/* 0x25d8 INX */
	((((((0*I*N*Y,CLASS1,0xc8,			/* 0x25d9 INY */
	((((((0*J*M*P,CLASS4,0x40,			/* 0x29b0 JMP */
	((((((0*J*S*R,CLASS3,0x14,			/* 0x2a72 JSR */
	((((((0*L*D*A,CLASS14,0xa1,			/* 0x3081 LDA */
	((((((0*L*D*X,CLASS13,0xa2,			/* 0x3098 LDX */
	((((((0*L*D*Y,CLASS12,0xa0,			/* 0x3099 LDY */
	((((((0*L*S*R,CLASS9,0x42,			/* 0x3272 LSR */
	((((((0*N*O*P,CLASS1,0xea,			/* 0x39f0 NOP */
	((((((0*O*R*A,CLASS14,0x01,			/* 0x3e41 ORA */
	((((((0*P*H*A,CLASS1,0x48,			/* 0x4101 PHA */
	((((((0*P*H*P,CLASS1,0x08,			/* 0x4110 PHP */
	((((((0*P*L*A,CLASS1,0x68,			/* 0x4181 PLA */
	((((((0*P*L*P,CLASS1,0x28,			/* 0x4190 PLP */
	((((((0*R*O*L,CLASS9,0x22,			/* 0x49ec ROL */
	((((((0*R*O*R,CLASS9,0x62,			/* 0x49f2 ROR */
	((((((0*R*T*I,CLASS1,0x40,			/* 0x4a89 RTI */
	((((((0*R*T*S,CLASS1,0x60,			/* 0x4a93 RTS */
	((((((0*S*B*C,CLASS14,0xe1,			/* 0x4c43 SBC */
	((((((0*S*E*C,CLASS1,0x38,			/* 0x4ca3 SEC */
	((((((0*S*E*D,CLASS1,0xf8,			/* 0x4ca4 SED */
	((((((0*S*E*I,CLASS1,0x78,			/* 0x4ca9 SEI */
	((((((0*S*T*A,CLASS6,0x81,			/* 0x4e81 STA */
	((((((0*S*T*X,CLASS10,0x82,			/* 0x4e98 STX */
	((((((0*S*T*Y,CLASS7,0x80,			/* 0x4e99 STY */
	((((((0*T*A*X,CLASS1,0xaa,			/* 0x5038 TAX */
	((((((0*T*A*Y,CLASS1,0xa8,			/* 0x5039 TAY */
	((((((0*T*S*X,CLASS1,0xba,			/* 0x5278 TSX */
	((((((0*T*X*A,CLASS1,0x8a,			/* 0x5301 TXA */
	((((((0*T*X*S,CLASS1,0x9a,			/* 0x5313 TXS */
	((((((0*T*Y*A,CLASS1,0x98,			/* 0x5321 TYA */
	((((((0*0x20)+(31))*B*Y^((0*T,PSEUDO,0,		/* 0x7c4d .BYT */
	((((((0*0x20)+(31))*D*B^((0*Y,PSEUDO,6,		/* 0x7c9b .DBY */
	((((((0*0x20)+(31))*E*N^((0*D,PSEUDO,4,		/* 0x7caa .END */
	((((((0*0x20)+(31))*G*B^((0*Y,PSEUDO,6,		/* 0x7c9b .GBY */
	((((((0*0x20)+(31))*O*P^((0*T,PSEUDO,5,		/* 0x7de4 .OPT */
	((((((0*0x20)+(31))*P*A^((0*G,PSEUDO,7,		/* 0x7e06 .PAG */
	((((((0*0x20)+(31))*S*K^((0*I,PSEUDO,8,		/* 0x7e62 .SKI */
	((((((0*0x20)+(31))*W*O^((0*R,PSEUDO,2,		/* 0x7ef8 .WOR */
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0,
	0x7fff,0,0
};

int	step[] =
{
	3*((OPSIZE+1)/2),
	3*((((OPSIZE+1)/2)+1)/2),
	3*((((((OPSIZE+1)/2)+1)/2)+1)/2),
	3*((((((((OPSIZE+1)/2)+1)/2)+1)/2)+1)/2),
	3*((((((((((OPSIZE+1)/2)+1)/2)+1)/2)+1)/2)+1)/2),
	3*(2),
	3*(1),
	0
};

static void loadv(int val,int  f,int  outflg);
static void newlc(unsigned val);
static int colnum(int *ip);
static int evaluate(int *ip);
static void prtobj(void);
static int symval(int *ip);
static void putobj(unsigned val);
static void startobj(void);
static int addref(int ip);
static int openspc(int ptr,int len);
static int colsym(int *ip);
static int stinstal(int ptr);
static void printhead(void);
static void class3(int *ip);
static void class2(int *ip);
static void class1(void);
static void pseudo(int *ip);
static void loadlc(int val, int f);
static void getargs(int argc,char *argv[]);
static void help(void);
static void initvar(void);
static void initialize(int ac, char *av[], int argc);
static int readline(void);
static void assemble(void);
static void stprnt(void);
static void wrapup(void);
static void clrlin(void);
static void finobj(void);
static void hexcon(int digit, int num);
static int labldef(int lval);
static void prsyline(void);
static void prsymhead(void);
static void println(void);
static int stlook(void);
static int oplook(int *ip);
static void error(char *stptr);

/*********************************************************************/

/* main */

int main(int argc, char *argv[])
{
	int	i;
	int	ac;
	char	**av;

	size = STABSZ;
	pagesize = PAGESIZE;
	linesize = LINESIZE;

	fprintf(stderr, "\n%s  %s  %s\n\n",pname,ptitle,pdate);
	getargs(argc, argv);	/* parse the command line arguments */
	if (badflag > 0) exit(1);
	if (act == 0) {
		help();		/* if no arg show help */
		exit(1);
	}
	symtab = malloc(size);
	if (symtab == 0) {
		fprintf(stderr,"Symbol table allocation failed - specify smaller size\n");
		exit(2);  }
	pagect = 0;
	paglin = pagesize;
	titlesize = linesize-SFIELD-11;
	for (i=0; i<LINESIZE; i++) titlbuf[i] = ' ';
	for (i=0; i<strlen(pname); i++) titlbuf[i] = pname[i];
	for (i=0; i<strlen(ptitle); i++) titlbuf[i+SFIELD] = ptitle[i];
	for (; i<TITLESIZE; i++) titlbuf[i+SFIELD] = ' ';
	titlbuf[SFIELD+titlesize] = '\0';
	ac = act;
	av = avt;
	pass = FIRST_PASS;
	initvar();
	gflag = eflag = 1;
	while (pass != DONE) {
		initialize(ac, av, act);
		fprintf(stderr,"Pass %d %s\n",pass+1,*av);
		endflag = 0;
		if(pass == LAST_PASS && ac == act)
			initvar();
		/* lower level routines can terminate assembly by setting
		   pass = DONE  ('symbol table full' does this) */
		while (readline() != -1 && pass != DONE && endflag == 0)
			assemble(); /* rest of assembler executes from here */
		if (errcnt != 0) {
			pass = DONE;
		}
		switch (pass) {
		case FIRST_PASS:
			--ac;
			++av;
			if (ac == 0){
				pass = LAST_PASS;
				/* if (lflag == 0)
					lflag++; */
				ac = act;
				av = avt;
			}
			break;
		case LAST_PASS:
			--ac;
			++av;
			if (ac == 0) {
				pass = DONE;
				if (sflag != 0)
					stprnt();
			}
		}
		wrapup();
	}
	fclose(stdout);
	free(symtab);
	fprintf(stderr, "\nErrors   = %d\nWarnings = %d\n", errcnt, warncnt);
	return(0);
}

/*********************************************************************/

/* parse the command args and save data */

static void getargs(int argc,char *argv[])
{
	int	i;
	char 	c;
	unsigned	sz;
	while (--argc > 0 && (*++argv)[0] == '-') {
		for (i = 1; (c = (*argv)[i]) != '\0'; i++) {
			switch (toupper(c)) {
			case 'B':		/* binary output flag */
				bflag++;
				break;
			case 'C':		/* symbol case sensitive */
				cflag++;
				break;
			case 'L':		/* enable listing flag */
				lflag++;
				break;
			case 'O':		/* object output flag */
				oflag++;
				break;
			case 'M':		/* MOS Tech. object format */
				mflag++;
				oflag++;	/* -m implies -o */
				break;
			case 'S':		/* list symbol table flag */
				sflag++;
				break;
			case 'T':           	/* input symbol table size */
				{
				if ((*argv)[++i] == '\0') {
					++argv;
					argc--;
					sz = atoi(*argv);
					} else sz = atoi(&(*argv)[i]);
				if (sz>=1000) size=sz;
				else {
				     fprintf(stderr,
				     "Invalid symbol table size (min 1000)\n");
				     badflag++; }
				goto outofloop;
				}
			case 'P':		/* input lines per page */
				{
				if ((*argv)[++i] == '\0') {
					++argv;
					argc--;
					sz = atoi(*argv);
					} else sz = atoi(&(*argv)[i]);
				if (sz>=10 || sz == 0 ) pagesize=sz;
				else {
				     fprintf(stderr,
				     "Invalid pagesize (min 10)\n");
				     badflag++; }
				goto outofloop;
				}
			case 'W':		/* input characters per line */
				{
				if ((*argv)[++i] == '\0') {
					++argv;
					argc--;
					sz = atoi(*argv);
					} else sz = atoi(&(*argv)[i]);
				if (sz >= 80 && sz < LINESIZE+1) linesize=sz;
				else {
				     fprintf(stderr,
				     "Invalid linesize (min 80, max 132)\n");
				     badflag++; }
				goto outofloop;
				}
			default:
			help();
			badflag++;
			} /* end switch */
		} /* end for  */
		outofloop: ;
	}
	act=argc; /* return values to main */
	avt=argv;
}

/*********************************************************************/

/* open files */

static void initialize(int ac, char *av[], int argc)
{
	if ((iptr = fopen(*av, "r")) == NULL) {
		fprintf(stderr, "Open error for file '%s'\n", *av);
		exit(1);
	}
	if ((pass == LAST_PASS) && ac == argc) {
		if (oflag != 0) {
			if ((optr = fopen("6502.hex", "wb")) == NULL) {
			fprintf(stderr, "Create error for object file 6502.hex\n");
			exit(1);
			}
		}
		if (bflag != 0) {
			if ((bptr = fopen("6502.bin", "wb")) == NULL) {
			fprintf(stderr, "Create error for binary file 6502.bin\n");
			exit(1);
			}
		}
	}
}

/*********************************************************************/

/* reads and formats an input line */

int	field[] =
{
	SFIELD,
	SFIELD + 8,
	SFIELD + 16,
	SFIELD + 24,
	SFIELD + 60,
	SFIELD + 80
};

static int readline(void)
{
	int	i;		/* pointer into prlnbuf */
	int	ch;		/* current character */
	unsigned temp1;		/* temp used for line number conversion */

	temp1 = ++slnum;
	clrlin();			/* clear line buffer */
	i = 4;				/* line# offset */
	while (temp1 != 0) {		/* put source line number into prlnbuf */
		prlnbuf[i--] = temp1 % 10 + '0';
		temp1 /= 10;
	}
	i = SFIELD;
	while ((ch = getc(iptr)) != '\n') { /* while not EOL */
		if (ch == '\r')		/* ignore CR */
			continue;
		prlnbuf[i++] = ch;	/* place char */
		if (ch == '\t') {	/* perform tab */
			prlnbuf[i - 1] = ' ';
			i = (((i - SFIELD-1) + 8) & 0x78) + SFIELD;
		}
		else if (ch == EOF || ch == CPMEOF) /* end of file */
			return(-1);
		else { /* other char */
			if (i >= LINESIZE-1) /* truncate long line */
				--i;
		}
	}
	prlnbuf[i] = 0;
	return(0);
}

/*********************************************************************/

/* closes the source, object and stdout files */

static void wrapup(void)
{
	fclose(iptr); /* close source file */
	if (pass == DONE) {
		if ((oflag != 0) && (optr != 0)) {
			finobj();
			fclose(optr);
		}
		if ((bflag != 0) && (bptr != 0)) {
			fclose(bptr);
		}
	}
	return;
}

/*********************************************************************/

/* symbol table print */

static void stprnt(void)
{
	int	i;		/* print line position */
	int	ptr;		/* symbol table position */
	int	j;		/* integer conversion variable */
	int	k;		/* printf buffer pointer */
	int	refct;		/* counter for references */
	char	buf[16];
	paglin = pagesize;
	ptr = 0;
	clrlin();
	while (ptr < nxt_free)
		{
		for (i=1; i <= symtab[ptr]; i++) prlnbuf[i] = symtab[ptr+i];
		ptr += i+1; i=19;         /* value at pos 19  */
		j = symtab[ptr++] & 0xff;
		j += (symtab[ptr++] << 8);
		hexcon(4,j);
		for (k=1; k<5; k++) prlnbuf[i++] = hex[k];
		j = symtab[ptr++] & 0xff;
		j += (symtab[ptr++] << 8);
		sprintf(buf,"%d",j);
		k=0;i=26;		/* line defined at pos 26 */
		while (buf[k] != '\0') prlnbuf[i++] = buf[k++];
		k=0;i=32;		/* count of references    */
		refct = symtab[ptr++] & 0xff;
		sprintf(buf,"(%d)",refct);
		while (buf[k] != '\0') prlnbuf[i++] = buf[k++];
		i++;			/* and all the references   */
		while (refct > 0)
			{
			j = symtab[ptr++] & 0xff;
			j += (symtab[ptr++] << 8);
			sprintf(buf,"%d",j);
			k=0;
			while (buf[k] != '\0') prlnbuf[i++] = buf[k++];
			i++;
			refct--;
			if ( i > linesize-5 && refct > 0) {
				prlnbuf[i] = '\0';
				prsyline(); i=32+4; }
			}
		prlnbuf[i] = '\0';
		prsyline();
		}
}

/*********************************************************************/

/* prints the contents of prlnbuf */

static void prsyline(void)
{
	if (paglin == pagesize) prsymhead();
	prlnbuf[linesize] = '\0';
	fprintf(stdout, "%s\n", prlnbuf);
	paglin++ ;
	clrlin();
}

/*********************************************************************/

/* prints the symbol page heading */

static void prsymhead(void)
{
	if (pagesize == 0) return;
	pagect++ ;
	fprintf(stdout, "\f\n%sPage %d\n",titlbuf,pagect);
	fprintf(stdout, "Symbol             Value  Line  References\n\n");
	paglin = 0;
}

/*********************************************************************/

/* clear the print buffer */

static void clrlin(void)
{
	int i;
	for (i = 0; i < LINESIZE; i++)
		prlnbuf[i] = ' ';
}

/*********************************************************************/

/* display help */

static void help(void)
{
	fprintf(stderr, "use:  A65 [options] file[s]\n\n");
	fprintf(stderr, "-B   output binary file\n");
	fprintf(stderr, "-C   symbols case sensitive\n");
	fprintf(stderr, "-L   output listing\n");
	fprintf(stderr, "-M   output MOS Technology hex object file\n");
	fprintf(stderr, "-O   output INTEL hex object file\n");
	fprintf(stderr, "-Pn  page length n (0 = no paging)\n");
	fprintf(stderr, "-S   include symbol table in listing\n");
	fprintf(stderr, "-Tn  symbol table size n\n");
	fprintf(stderr, "-Wn  page width n\n");
}

/*********************************************************************/

/* translate source line to machine language */

static void assemble(void)
{
	int	flg;
	int	i;	/* prlnbuf pointer */

	if ((prlnbuf[SFIELD] == ';') | (prlnbuf[SFIELD] == 0)) {
		if (pass == LAST_PASS)
			println();
		return;
	}
	lablptr = -1;
	i = SFIELD;
	udtype = UNDEF;
	if (colsym(&i) != 0 && (lablptr = stlook()) == -1)
		return;
	while (prlnbuf[i] == ' ') i++;	/* find first non-space */
	if ((flg = oplook(&i)) < 0) {	/* collect operation code */
		labldef(loccnt);
		if (flg == -1)
			error("Invalid operation code");
		if ((flg == -2) && (pass == LAST_PASS)) {
			if (lablptr != -1)
				loadlc(loccnt, 0);
			println();
		}
		return;
	}
	if (opflg == PSEUDO)
		pseudo(&i);
	else if (labldef(loccnt) == -1)
		return;
	else {
		if (opflg == CLASS1)
			class1();
		else if (opflg == CLASS2)
			class2(&i);
		else class3(&i);
	}
}

/*********************************************************************/

/* prints the contents of prlnbuf */

static void println(void)
{
	if (lflag > 0)
		{
		if (paglin == pagesize) printhead();
		prlnbuf[linesize] = '\0';
		fprintf(stdout, "%s\n", prlnbuf);
		paglin++ ;
		}
}

/*********************************************************************/

/* prints the page heading */

static void printhead(void)
{
	if (pagesize == 0) return;
	pagect++ ;
	fprintf(stdout, "\f\n%sPage %d\n",titlbuf,pagect);
	fprintf(stdout, "Line#  Loc   Code      Line\n\n");
	paglin = 0;
}

/*********************************************************************/

/* collects a symbol from prlnbuf into symbol[], leaves prlnbuf pointer
   at first invalid symbol character, returns 0 if no symbol collected */

static int colsym(int *ip)
{
	int	valid;
	int	i;
	char	ch;

	valid = 1;
	i = 0;
	while (valid == 1) {
		ch = prlnbuf[*ip];
		if (cflag == 0) ch = toupper(ch);
		if (ch == '_' && i != 0);
		else if (ch >= 'a' && ch <= 'z');
		else if (ch >= 'A' && ch <= 'Z');
		else if (ch >= '0' && ch <= '9' && i != 0);
		else valid = 0;
		if (valid == 1) {
			if (i < SBOLSZ)
				symbol[++i] = ch;
			(*ip)++;
		}
	}
	if (i == 1) {
		switch (symbol[1]) {
		case 'A': case 'a':
		case 'S': case 's':
		case 'P': case 'p':
		case 'X': case 'x':
		case 'Y': case 'y':
			error("Symbol is reserved (A,X,Y,S,P)");
			i = 0;
		}
	}
	symbol[0] = i;
	return(i);
}

/*********************************************************************/

/* symbol table lookup, if found, return pointer to symbol else,
   install symbol as undefined, and return pointer */

static int stlook(void)
{
	int ptr, ln, eq;
	ptr = 0;
	while (ptr < nxt_free) {
		ln = symbol[0]; if (symtab[ptr] < ln) ln = symtab[ptr];
		if ((eq = strncmp(&symtab[ptr+1], &symbol[1], ln)) == 0 &&
		symtab[ptr] == symbol[0]) return ptr;
		if (eq > 0) return(stinstal(ptr));
		ptr = ptr+6+ symtab[ptr];
		ptr = ptr +1 + 2*(symtab[ptr] & 0xff);
	}
return (stinstal(ptr));
}

/*********************************************************************/

/* install symbol into symtab */

static int stinstal(int ptr)
{
	int	ptr2, i;

	if (openspc(ptr,symbol[0]+7) == -1) {
		error("Symbol table full"); /* print error msg and ...  */
		pass = DONE;		    /* cause termination of assembly */
		return -1;
	}
	ptr2 = ptr;
	for (i=0; i< symbol[0]+1; i++)
		symtab[ptr2++] = symbol[i];
	symtab[ptr2++] = udtype;
	symtab[ptr2+4] = 0;
	return(ptr);
}

/*********************************************************************/

/* add a reference line to the symbol pointed to by ip. */

static int addref(int ip)
{
	int	rct, ptr;

	rct = ptr =ip + symtab[ip] + 6;
	if ((symtab[rct] & 0xff) == 255) {	/* non-fatal error */
		fprintf(stderr,"%s\n",prlnbuf);
		fprintf(stderr,"Too many references\n");
		return 0;
	}
	ptr += (symtab[rct] & 0xff) * 2 +1;
	if (openspc(ptr,2) == -1) {
		error("Symbol table full");
		return -1;
	}
	symtab[ptr] = slnum & 0xff;
	symtab[ptr+1] = (slnum >> 8) & 0xff;
	symtab[rct]++;
   return 0;
}

/*********************************************************************/

/* open up a space in the symbol table. the space will be at (ptr)
   and will be len characters long. return -1 if no room. */

static int openspc(int ptr,int len)
{
	int	ptr2, ptr3;
	if (nxt_free + len > size) return -1;
	if (ptr != nxt_free) {
		ptr2 = nxt_free -1;
		ptr3 = ptr2 + len;
		while (ptr2 >= ptr) symtab[ptr3--] = symtab[ptr2--];
	}
	nxt_free += len;
	if (lablptr >= ptr) lablptr += len;
	return 0;
}

/*********************************************************************/

/* operation code table lookup if found, return pointer to symbol,
   else return -1 */

static int oplook(int *ip)
{
register char	ch;
register int	i;
register int	j;
	 int	k;
	 int	temp[3];
	 int	flag;

	i = j = flag = 0;
	temp[0] = temp[1] = 0;
	while((ch=prlnbuf[*ip])!= ' ' && ch!= 0 && ch!= '\t' && ch!= ';') {
		if (flag == 0) {
			if (ch >= 'A' && ch <= 'Z')
				ch &= 0x1f;
			else if (ch >= 'a' && ch <= 'z')
				ch &= 0x1f;
			else if (ch == '.')
				ch = 31;
			else if (ch == '*')
				ch = 30;
			else if (ch == '=')
				ch = 29;
			else return(-1);
			temp[j] = (temp[j] * 0x20) + (ch & 0xff);
			if (ch == 29)
				break;
			/* ++(*ip); */
			if (++i >= 3) {
				i = 0; ++j;
			}
			if ((j > 0) && (i >= 1)) flag = 1;
		}
		++(*ip);
	}
	if ((j = temp[0]^temp[1]) == 0)
		return(-2);
	k = 0;
	i = step[k] - 3;
	do {
		if (j == optab[i]) {
			opflg = optab[++i];
			opval = optab[++i];
			return(i);
		}
		else if (j < optab[i])
			i -= step[++k];
		else i += step[++k];
	} while (step[k] != 0);
	return(-1);
}

/*********************************************************************/

/* error printing routine */

static void error(char *stptr)
{
	loadlc(loccnt, 0);
	loccnt += 3;
	loadv(0,0,0);
	loadv(0,1,0);
	loadv(0,2,0);
	fprintf(stdout, "%s\n", prlnbuf);
	fprintf(stdout, "%s\n", stptr);
	errcnt++;
}

/*********************************************************************/

/* load 16 bit value in printable form into prlnbuf */

static void loadlc(int val, int f)
{
	int	i;

	i = 7 + 6*f; /* start pos */
	hexcon(4, val);
	prlnbuf[i++] = hex[1];
	prlnbuf[i++] = hex[2];
	prlnbuf[i++] = hex[3];
	prlnbuf[i] = hex[4];
}

/*********************************************************************/

/* load value in hex into prlnbuf[contents[i]] and output
   hex characters to obuf if LAST_PASS & oflag == 1 */

static void loadv(int val,int  f,int  outflg)
/* int	f;		   contents field subscript */
/* int	outflg;	flag to output object bytes */
{
	hexcon(2, val);
	prlnbuf[13 + 3*f] = hex[1];
	prlnbuf[14 + 3*f] = hex[2];
	if ((pass == LAST_PASS) && (outflg != 0)) {
		if (oflag != 0) {
			if (objcnt == 0) startobj();
			putobj(val);
			objcnt--;
			objloc++;
		}
		if (bflag != 0) {
			fputc(val,bptr);
			if (cbase == 0xffff) cbase = loccnt;
		}
	}
}

/*********************************************************************/

/* convert number supplied as argument to hexadecimal
   in hex[digit] (lsd) through hex[1] (msd) */

static void hexcon(int digit, int num)
{
	for (; digit > 0; digit--) {
		hex[digit] = (num & 0x0f) + '0';
		if (hex[digit] > '9')
			hex[digit] += 'A' -'9' - 1;
		num >>= 4;
	}
}

/*********************************************************************/

/* assign <value> to label pointed to by lablptr, checking for
   valid definition, etc. */

static int labldef(int lval)
{
	int	i;

	if (lablptr != -1) {
		lablptr += symtab[lablptr] + 1;
		if (pass == FIRST_PASS) {
			if (symtab[lablptr] == UNDEF) {
				symtab[lablptr + 1] = lval & 0xff;
				i = symtab[lablptr + 2] = (lval >> 8) & 0xff;
				if (i == 0)
					symtab[lablptr] = DEFZRO;
				else	symtab[lablptr] = DEFABS;
			}
			else if (symtab[lablptr] == UNDEFAB) {
				symtab[lablptr] = DEFABS;
				symtab[lablptr + 1] = lval & 0xff;
				symtab[lablptr + 2] = (lval >> 8) & 0xff;
			}
			else {
				symtab[lablptr] = MDEF;
				symtab[lablptr + 1] = 0;
				symtab[lablptr + 2] = 0;
				error("Label multiply defined");
				return(-1);
			}
		symtab[lablptr+3] = slnum & 0xff;
		symtab[lablptr+4] = (slnum >> 8) & 0xff;
		}
		else {
			i = (symtab[lablptr + 2] << 8) +
				(symtab[lablptr+1] & 0xff);
			i &= 0xffff;
			if (i != lval && pass == LAST_PASS) {
				error("Sync error");
				return(-1);
			}
		}
	}
	return(0);
}

/*********************************************************************/

/* determine the value of the symbol, given pointer to first
   character of symbol in symtab */

static int symval(int *ip)
{
	int	ptr;
	int	svalue;

	svalue = 0;
	colsym(ip);
	if ((ptr = stlook()) == -1)
		undef = 1;		/* no room error */
	else if (symtab[ptr + symtab[ptr] + 1] == UNDEF)
		undef = 1;
	else if (symtab[ptr + symtab[ptr] + 1] == UNDEFAB)
		undef = 1;
	else svalue = ((symtab[ptr + symtab[ptr] + 3] << 8) +
		(symtab[ptr + symtab[ptr] + 2] & 0xff)) & 0xffff;
	if (symtab[ptr + symtab[ptr] + 1] == DEFABS)
		zpref = 1;
	if (undef != 0)
		zpref = 1;

	/* add a reference entry to symbol table on first pass only,
	   except for branch instructions (CLASS2) which do not come
	   through here on the first pass */
	if (ptr >= 0 && pass == FIRST_PASS) addref(ptr);
	if (ptr >= 0 && opflg == CLASS2) addref(ptr); /* branch addresses */
	return(svalue);
}

/*********************************************************************/

/* put one object byte in hex */

static void putobj(unsigned val)
{
	hexcon(2,val);
	objrec[objptr++] = hex[1];
	objrec[objptr++] = hex[2];
	cksum += (val & 0xff);
	objbytes++;
}

/*********************************************************************/

/* start an object record (end previous) */

static void startobj(void)
{
	prtobj();	/* print the current record if any */
	hexcon(4,objloc);
	objbytes=0;
	for (objptr=0; objptr<4; objptr++)
		objrec[objptr] = hex[objptr+1];
	cksum = (objloc>>8) + (objloc & 0xff);
	if (mflag == 0) objcnt = 16;
	else objcnt = 24;
}

/*********************************************************************/

/* print the current object record if any */

static void prtobj(void)
{
	int	i;

	if (objbytes == 0) return;
	cksum += objbytes;
	hexcon(2,objbytes);
	objrec[objptr] = '\0';

	if (mflag == 0) {
		fprintf(optr,":%c%c",hex[1],hex[2]); /* number data bytes */
		for (i = 0; i <= 3; ++i) fputc(objrec[i],optr);
		fprintf(optr,"00%s",objrec+4);
		hexcon(2,0-cksum);
		fprintf(optr,"%c%c\r\n",hex[1],hex[2]);
	}
	else {
		fprintf(optr,";%c%c",hex[1],hex[2]); /* number data bytes */
		fprintf(optr,"%s",objrec);
		hexcon(4,cksum);
		fprintf(optr,"%c%c%c%c\r\n",hex[1],hex[2],hex[3],hex[4]);
	}
	reccnt++;
}

/*********************************************************************/

/* finish object file */

static void finobj(void)
{
	unsigned i, j;

	prtobj();
	if (mflag == 0)
		fprintf(optr,":00000001FF");
	else {
		hexcon(4,reccnt);
		fprintf(optr,";00");
		for (j=1; j<3; j++)
			for (i=1; i<5; i++) fputc(hex[i],optr);
	}
	fprintf(optr,"\r\n");
}

/*********************************************************************/

/* machine operations processor - 1 byte, no operand field */

static void class1(void)
{
	if (pass == LAST_PASS) {
		loadlc(loccnt, 0);
		loadv(opval, 0, 1);
		println();
	}
	loccnt++;
}

/*********************************************************************/

/* machine operations processor - 2 byte, relative addressing */

static void class2(int *ip)
{
	if (pass == LAST_PASS) {
		loadlc(loccnt, 0);
		loadv(opval, 0, 1);
		while (prlnbuf[++(*ip)] == ' ');
		if (evaluate(ip) != 0) {
			loccnt += 2;
			return;
		}
		loccnt += 2;
		if ((value -= loccnt) >= -128 && value < 128) {
			loadv(value, 1, 1);
			println();
		}
		else error("Invalid branch address");
	}
	else loccnt += 2;
}

/*********************************************************************/

/* machine operations processor - various addressing modes */

static void class3(int *ip)
{
	char	ch;
	int	code;
	int	flag;
	int	i;
	int	ztmask;

	ztmask = 0;
	while ((ch = prlnbuf[++(*ip)]) == ' ');
	switch(ch) {
	case 0:
	case ';':
		error("Operand field missing");
		return;
	case 'A':
	case 'a':
		if ((ch = prlnbuf[*ip + 1]) == ' ' || ch == ';' || ch == 0) {
			flag = ACC;
			break;
		}
	default:
		switch(ch = prlnbuf[*ip]) {
		case '#':
			flag = IMM1|IMM2;
			++(*ip);
			break;
		case '(':
			flag = IND|INDX|INDY;
			++(*ip);
			break;
		default:
			flag = ABS|ZER|ZERX|ABSX|ABSY|ABSY2|ZERY;
		}
		if ((flag & (INDX|INDY|ZER|ZERX|ZERY) & opflg) != 0)
			udtype = UNDEFAB;
		if (evaluate(ip) != 0)
			return;
		if (zpref != 0) {
			flag &= (ABS|ABSX|ABSY|ABSY2|IND|IMM1|IMM2);
			ztmask = 0;
		}
		else ztmask = ZER|ZERX|ZERY;
		code = 0;
		i = 0;
		while (( ch = prlnbuf[(*ip)++]) != ' ' && ch != ';' && ch != 0 && i++ < 4) {
			code *= 8;
			switch(toupper(ch)) {
			case ')':		/* ) = 4 */
				++code;
			case ',':		/* , = 3 */
				++code;
			case 'X':		/* X = 2 */
				++code;
			case 'Y':		/* Y = 1 */
				++code;
				break;
			default:
				flag = 0;
			}
		}
		switch(code) {
		case 0:		/* no termination characters */
			flag &= (ABS|ZER|IMM1|IMM2);
			break;
		case 4:		/* termination = ) */
			flag &= IND;
			break;
		case 25:	/* termination = ,Y */
			flag &= (ABSY|ABSY2|ZERY);
			break;
		case 26:	/* termination = ,X */
			flag &= (ABSX|ZERX);
			break;
		case 212:	/* termination = ,X) */
			flag &= INDX;
			break;
		case 281:	/* termination = ),Y */
			flag &= INDY;
			break;
		default:
			flag = 0;
		}
	}
	if ((opflg &= flag) == 0) {
		error("Invalid addressing mode");
		return;
	}
	if ((opflg & ztmask) != 0)
		opflg &= ztmask;
	switch(opflg) {
	case ACC:		/* single byte - class 3 */
		if (pass == LAST_PASS) {
			loadlc(loccnt, 0);
			loadv(opval + 8, 0, 1);
			println();
		}
		loccnt++;
		return;
	case ZERX: case ZERY:	/* double byte - class 3 */
		opval += 4;
	case INDY:
		opval += 8;
	case IMM2:
		opval += 4;
	case ZER:
		opval += 4;
	case INDX: case IMM1:
		if (pass == LAST_PASS) {
			loadlc(loccnt, 0);
			loadv(opval, 0, 1);
			loadv(value, 1, 1);
			println();
		}
		loccnt += 2;
		return;
	case IND:		/* triple byte - class 3 */
		opval += 16;
	case ABSX:
	case ABSY2:
		opval += 4;
	case ABSY:
		opval += 12;
	case ABS:
		if (pass == LAST_PASS) {
			opval += 12;
			loadlc(loccnt, 0);
			loadv(opval, 0, 1);
			loadv(value, 1, 1);
			loadv(value >> 8, 2, 1);
			println();
		}
		loccnt += 3;
		return;
	default:
		error("Invalid addressing mode");
		return;
	}
}

/*********************************************************************/

/* pseudo operations processor */

static void pseudo(int *ip)
{
	int	count;
	int	i,j;
	int	tvalue;
	int	quote;
	int	ch;

	switch(opval) {
	case 0:					/* .BYTE pseudo */
		labldef(loccnt);
		loadlc(loccnt, 0);
		count = quote = 0;
		while (prlnbuf[++(*ip)] == ' ');
		do {
			/* while (prlnbuf[(*ip)] == ' ') ++(*ip); */
			if (prlnbuf[*ip] == '\'') {
				++quote;
				while (quote != 0) {
					tvalue = prlnbuf[++(*ip)];
					if (tvalue == 0) {
						error("Unterminated ASCII string");
						return;
					}
					if (tvalue == '\'') {
						if ((tvalue = prlnbuf[++(*ip)]) != '\'') {
							quote = 0;
							--(*ip);
							goto done1;
						}
					}
					loccnt++;
					if (pass == LAST_PASS) {
						loadv(tvalue, count, 1);
						if (++count >= 3) {
							println();
							for (i = 0; i < SFIELD; i++)
								prlnbuf[i] = ' ';
							prlnbuf[i] = 0;
							loadlc(loccnt, 0);
							count = 0;
						}
					}
					done1:;
				}
				++(*ip);
			}
			else {
				if (evaluate(ip) != 0) {
					loccnt++;
					return;
				}
				loccnt++;
				if (value > 0xff) {
					error("Operand field size error");
					return;
				}
				else if (pass == LAST_PASS) {
					loadv(value, count, 1);
					if (++count >= 3) {
						println();
						for (i = 0; i < SFIELD; i++)
							prlnbuf[i] = ' ';
						prlnbuf[i] = 0;
						count = 0;
						loadlc(loccnt, 0);
					}
				}
			}
		} while ((ch=prlnbuf[(*ip)++]) == ',');
		if (ch != ' ' && ch != ';' && ch != 0) {
			error("Invalid operand field");
			return;
		}
		if ((pass == LAST_PASS) && (count != 0))
			println();
		return;
	case 1:					/* = pseudo*/
		while (prlnbuf[++(*ip)] == ' ');
		if (evaluate(ip) != 0)
			return;
		labldef(value);
		if (pass == LAST_PASS) {
			loadlc(value, 0);
			println();
		}
		return;
	case 2:					/* .WORD pseudo */
		labldef(loccnt);
		loadlc(loccnt, 0);
		while (prlnbuf[++(*ip)] == ' ');
		do {
			/* while (prlnbuf[(*ip)] == ' ') ++(*ip); */
			if (evaluate(ip) != 0) {
				loccnt += 2;
				return;
			}
			loccnt += 2;
			if (pass == LAST_PASS) {
				loadv(value, 0, 1);
				loadv(value>>8, 1, 1);
				println();
				for (i = 0; i < SFIELD; i++)
					prlnbuf[i] = ' ';
				prlnbuf[i] = 0;
				loadlc(loccnt, 0);

			}
		} while (prlnbuf[(*ip)++] == ',');
		return;
	case 3:					/* *= pseudo */
		while (prlnbuf[++(*ip)] == ' ');
		if (prlnbuf[*ip] == '*') {
			if (evaluate(ip) != 0)
				return;
			if (undef != 0) {
				error("Undefined symbol in operand field");
				return;
			}
			tvalue = loccnt;
		}
		else {
			if (evaluate(ip) != 0)
				return;
			if (undef != 0) {
				error("Undefined symbol in operand field");
				return;
			}
			tvalue = value;
		}
		newlc(value);
		labldef(tvalue);
		if (pass == LAST_PASS) {
			loadlc(tvalue, 0);
			println();
		}
		return;
	case 4:					/* .END pseudo */
		labldef(loccnt);
		loadlc(loccnt, 0);
		if (pass == LAST_PASS) {
			println();
			}
		endflag = 1;
		return;
	case 5:					/* .OPT pseudo */
		++(*ip);
      /* ap: while (prlnbuf[++(*ip)] == ' '); */
		do {
			i = 0; j = 1;
			while (prlnbuf[(*ip)] == ' ') ++(*ip);
			while ((ch=prlnbuf[*ip]) != ' ' && ch != ',' && ch != ';' && ch != '\0') {
				if (i < 3) { /* hash string */
					j = j * (toupper(ch) & 0x1f);
					++i;
				}
				++(*ip);
			}
			switch (j) {
			case 0x1ea:	/* GEN */
				gflag = 1;
				break;
			case 0x41a:	/* NOE */
				eflag = 0;
				break;
		 	case 0x5be:	/* NOG */
				gflag = 0;
				break;
		 	case 0x654:	/* ERR */
				eflag = 1;
				break;
		 	case 0x804:	/* LIS */
				lflag = 1;
				break;
		 	case 0x9d8:	/* NOL */
				lflag = 0;
				break;
		 	case 0xf96:	/* NOS */
				sflag = 0;
				break;
		 	case 0x181f:	/* SYM */
				sflag = 1;
				break;
			default:
				error("Invalid option");
				return ;
			}
		} while (prlnbuf[(*ip)++] == ',');
		return;
	case 6:					/* .DBYTE pseudo */
		labldef(loccnt);
		loadlc(loccnt, 0);
		while (prlnbuf[++(*ip)] == ' ');
		do {
			/* while (prlnbuf[(*ip)] == ' ') ++(*ip); */
			if (evaluate(ip) != 0) {
				loccnt += 2;
				return;
			}
			loccnt += 2;
			if (pass == LAST_PASS) {
				loadv(value>>8, 0, 1);
				loadv(value, 1, 1);
				println();
				for (i = 0; i < SFIELD; i++)
					prlnbuf[i] = ' ';
				prlnbuf[i] = 0;
				loadlc(loccnt, 0);
			}
		} while (prlnbuf[(*ip)++] == ',');
		return;
	case 7:					/* .PAGE pseudo */
		if (pagesize == 0) return;
		while (prlnbuf[++(*ip)] == ' ');
		if (prlnbuf[(*ip)] == '\'') {
			i = quote = 0;
			++quote;
			while (quote != 0) {
				tvalue = prlnbuf[++(*ip)];
				if (tvalue == 0) {
					error("Unterminated ASCII string");
					return;
				}
				if (tvalue == '\'') {
					if ((tvalue = prlnbuf[++(*ip)]) != '\'') {
						quote = 0;
						--(*ip);
						goto done2;
					}
				}
				if (i < titlesize-2) {
					titlbuf[SFIELD+i++] = tvalue;
				}
			}
			done2:;
			if (i<titlesize) for (j=i; j<titlesize; j++) titlbuf[SFIELD+j]=' ';
		}
		if ((lflag > 0) && (pass == LAST_PASS)) printhead();
		return;
	case 8:					/* .SKIP pseudo */
	/* unimplemented directives which are non-fatal */
		if (pass == LAST_PASS) {
			fprintf(stderr,"%s\n",prlnbuf);
			fprintf(stderr,"Not implemented\n");
		}
		++warncnt;
		return;
	}
}

/* evaluate expression */

static int evaluate(int *ip)
{
	int	tvalue;
	int	invalid;
	int	parflg, value2;
	char	ch;
	char	op;
	char	op2;

	op = '+';
	parflg = zpref = undef = value = invalid = 0;

/* hcj: zpref should reflect the value of the expression, not the value of
   the intermediate symbols */

	while ((ch=prlnbuf[*ip]) != ' ' && ch != ')' && ch != ',' && ch != ';') {
		tvalue = 0;
		if (ch == '$' || ch == '@' || ch == '%')
			tvalue = colnum(ip);
		else if (ch >= '0' && ch <= '9')
			tvalue = colnum(ip);
		else if (ch >= 'a' && ch <= 'z')
			tvalue = symval(ip);
		else if (ch >= 'A' && ch <= 'Z')
			tvalue = symval(ip);
		else if (ch == '_')
			tvalue = symval(ip);
		else if (ch == '*') {
			tvalue = loccnt;
			++(*ip);
		}
		else if (ch == '\'') {
			++(*ip);
			tvalue = prlnbuf[*ip] & 0xff;
			++(*ip);
			if (prlnbuf[*ip] == '\'') ++(*ip);
		}
		else if (ch == '[') {
			if (parflg == 1) {
				error("Too many [ in expression");
				invalid++;
			}
			else {
				value2 = value;
				op2 = op;
				value = tvalue = 0;
				op = '+';
				parflg = 1;
			}
			goto next;
		}
		else if (ch == ']') {
			if (parflg == 0) {
				error("Missing [ in expression");
				invalid++;
			}
			else {
				parflg = 0;
				tvalue = value;
				value = value2;
				op = op2;
			}
			++(*ip);
		}
		switch(op) {
		case '+':
			value += tvalue;
			break;
		case '-':
			value -= tvalue;
			break;
		case '/':
			value = (unsigned) value/tvalue;
			break;
		case '*':
			value *= tvalue;
			break;
		case '%':
			value = (unsigned) value%tvalue;
			break;
		case '^':
			value ^= tvalue;
			break;
		case '~':
			value = ~tvalue;
			break;
		case '&':
			value &= tvalue;
			break;
		case '|':
			value |= tvalue;
			break;
		case '>':
			tvalue >>= 8;		/* fall through to '<' */
		case '<':
			if (value != 0) {
				error("High/low byte operator misplaced");
			}
			value = tvalue & 0xff;
			zpref = 0;
			break;
		default:
			invalid++;
		}
		if ((op=prlnbuf[*ip]) == ' '
				|| op == ')'
				|| op == ','
				|| op == ';')
			break;
		else if (op != ']')
next:			++(*ip);
	}
	if (parflg == 1) {
		error("Missing ] in expression");
		return(1);
	}
	if (value < 0 || value >= 256) {
		zpref = 1;
	}
	if (undef != 0) {
		if (pass != FIRST_PASS) {
			error("Undefined symbol in operand field");
			invalid++;
		}
		value = 0;
	}
	else if (invalid != 0)
	{
		error("Invalid operand field");
	}
	else { /* This is the only way out that may not signal error */
		if (value < 0 || value >= 256)
			zpref = 1;
		else
			zpref = 0;
	}
	return(invalid);
}

/*********************************************************************/

/* collect number operand */

static int colnum(int *ip)
{
	int	mul;
	int	nval;
	char	ch;

	nval = 0; mul = 10;
	if ((ch = prlnbuf[*ip]) == '$')
		mul = 16;
	else if (ch >= '0' && ch <= '9') {
		mul = 10;
		nval = ch - '0';
	}
	else if (ch == '@')
		mul = 8;
	else if (ch == '%')
		mul = 2;
	while ((ch = prlnbuf[++(*ip)] - '0') >= 0) {
		if (ch > 9) {
			ch -= ('A' - '9' - 1);
			if (ch > 15)
				ch -= ('a' - 'A');
			if (ch > 15)
				break;
			if (ch < 10)
				break;
		}
		if (ch >= mul)
			break;
		nval = (nval * mul) + ch;
	}
	return(nval);
}

/*********************************************************************/

/* location counter has changed */

static void newlc(unsigned val)
{
	if (val == loccnt) return;
	if (pass == LAST_PASS) {
		if (oflag != 0) { /* start new record */
			objloc = val;
			startobj();
		}
		if ((bflag != 0)&&(cbase != 0xffff )) { /* move file pointer */
			if (val < cbase) error("Invalid binary reposition");
			fseek(bptr,(val - cbase),SEEK_SET);
		}
	}
	loccnt = val;
}

/*********************************************************************/

/* init variables */

static void initvar(void)
{
	loccnt = objloc = 0;		/* location counter */
	slnum = 0;			/* line number */
	errcnt = warncnt = 0;		/* error/warning count */
	reccnt = cksum = 0;		/* hex file variables */
	cbase = 0xffff;			/* code base */
}

/*********************************************************************/

/* Details of hex object files

(all data is in ASCII encoded hexadecimal)

INTEL
-----
 Data record : :nnaaaattdddd...xx[cr]
 Last record : :00ssssttxx[cr]

 Where:
	:	= Start of record (ASCII 3A)
	nn	= Number of data bytes in the record (max = 16 bytes).
	aaaa	= address of first data byte in the record.
	tt	= record type: 00 for data, 01 for ending record.
	dd	= 1 data byte.
	xx	= checksum byte which when added the sum of all the
		  previous bytes in this record gives zero (mod 256).
	ssss	= optional start address; also signifies end-of-file.
	[cr]	= ASCII end-of-line sequence (CR,LF).


MOS Technology
--------------
 Data record : ;nnaaaadddd...xxxx[cr]
 Last record : ;00ccccxxxx[cr]

 Where:
	;	= Start of record (ASCII 3B)
	nn	= Number of data bytes in the record (max = 24 bytes).
	aaaa	= address of first data byte in the record.
	dd	= 1 data byte.
	xxxx	= checksum that is the twos compliment sum of all
		  data bytes, the count byte and the address bytes.
	cccc	= count of records in the file (not including the
		  last record).
	[cr]	= ASCII end-of-line sequence (CR,LF).

*/

