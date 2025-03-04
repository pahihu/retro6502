/* 6502 disassembler	*/
/* (c) 2025 pahihu 	*/

#include <stdlib.h>
#include <stdio.h>

#define MIN(a,b)  ((a) < (b) ? (a) : (b))

/* addressing mode encoding

00  X,ind   V   (aa,X)  1
04  zp      z   aa      1
08  #       #   #       1
0C  abs     a   aaaa    2
10  ind,Y   W   (aa),Y  1
14  zp,X    x   aa,X    1
18  abs,Y   Y   aaaa,Y  2
1C  abs,X   X   aaaa,X  2
    A       A   A       0
    ind     I   (aaaa)  2
    zp,y    y   aa,Y    1
    unknown -
    implied .

*/

static const char *ops[3]={
/*   00 20 40 60 80 A0 C0 E0    */
    "BRKBITJMPJMPSTYLDYCPYCPX",
    "ORAANDEORADCSTALDACMPSBC",
    "ASLROLLSRRORSTXLDXDECINC"
};
        
static const char *subops[2][4]={
{   /* 0 */
    /* 00 */ "BRKJSRRTIRTS-  #  #  #  ",
    /* 08 */ "PHPPLPPHAPLADEYTAYINYINX",
    /* 10 */ "BPLBMIBVCBVSBCCBCSBNEBEQ",
    /* 18 */ "CLCSECCLISEITYACLVCLDSED"
},
{   /* 2 */
    /* 00 */ NULL,
    /* 08 */ "A  A  A  A  TXATAXDEXNOP",
    /* 10 */ NULL,
    /* 18 */ "-  -  -  -  TXSTSX-  -  "
}
};

static const char *amodes[3][8]={
{
      /*     1111 */
      /* 048C048C */
/*BRK*/ ".-.-.-.-",
/*BIT*/ ".z.a.-.-",
/*JMP*/ ".-.a.-.-",
/*JMP*/ ".-.I.-.-",
/*STY*/ "-z.a.x.-",
/*LDY*/ "#z.a.x.X",
/*CPY*/ "#z.a.-.-",
/*CPX*/ "#z.a.-.-"
},
{
      /*     1111 */
      /* 048C048C */
/*ORA*/ "Vz#aWxYX",
/*AND*/ "Vz#aWxYX",
/*EOR*/ "Vz#aWxYX",
/*ADC*/ "Vz#aWxYX",
/*STA*/ "Vz-aWxYX", /**/
/*LDA*/ "Vz#aWxYX",
/*CMP*/ "Vz#aWxYX",
/*SBC*/ "Vz#aWxYX"
},
{
      /*     1111 */
      /* 048C048C */
/*ASL*/ "-zAa-x-X",
/*ROL*/ "-zAa-x-X",
/*LSR*/ "-zAa-x-X",
/*ROR*/ "-zAa-x-X",
/*STX*/ "-z.a-y.-",
/*LDX*/ "#z.a-y.Y",
/*DEC*/ "-z.a-x-X",
/*INC*/ "-z.a-x-X"
},
};

typedef unsigned char Byte;
typedef unsigned short Word;

#define NLINE 80
#define NSTR  6
Byte *M;
Word P;
int R, E;
char cLine[NLINE + 1], cLabel[NSTR + 1], cMnemo[NSTR + 1], AMode; /* replace ';' with NUL */
Word Arg;
char cText[NLINE + 1];

/* opcode table */
#define NOPS 256
Word *OpKeys;
Byte *OpVals;
Word hBYT, hWOR, hDBY, hJSR, hJMP, hBIT;

/* symbol table */
#define NSYMS 1024
Word *SymKeys;        /* hash val */
const char **Syms;    /* str key */
Byte *SymDefd;        /* symbol defined? */
Word *SymVals;        /* sym val */
char *TxtArea, *Txt;  /* sym storage */


#define LO(x) ((x) & 0xFF)
#define HI(x) LO((x) >> 8)

void emit(char c) { R++; putchar(c); }
void bl(void) { emit(' '); }
void cr(void) { emit('\n'); }
void type(const char *s) { char c; while ((c = *s++)) emit(c); }
int zcount(const char *s) { int n = 0; while (*s++) n++; return n; }
int compare(const char *s, const char *d)
{ int x, y;
  do { x = *s++; y = *d++;
  } while (x && y && x == y);
  return (x - y);
}
void wn(Byte x) { emit("0123456789ABCDEF"[x]); }
void wb(Byte x) { wn(x >> 4); wn(x & 15); }
void ww(Word w) { wb(HI(w)); wb(LO(w)); }
const char* skip(const char *s, char c) {
  char p;

  while ((p = *s) && p == c) s++;
  return s;
}

const char* scan(const char *s, char c, char *dst) {
  char p;
  int x;

  x = 0;
  while ((p = *s) && p != c) {
    if (dst && (x < NSTR))
      dst[x++] = p;
    s++;
  }
  if (dst) dst[x] = 0;
  return p == c ? s : NULL;
}

const char* err(const char *s) {
  cLine[s - cLine] = 0;
  type(cLine); emit('?');
  return NULL;
}

const char* errs(const char *msg) {
  type("Error: "); type(msg); cr();
  return NULL;
}

Word hash(const char *s, int n)
{
  int i;
  Word h;

  h = 0;
  for (i = 0; i < n; i++)
    h = (26 * h) + (s[i] - 'A');

  return h + 1;
}

int hget(Word *keys, int N, Word key)
{
  int i, x;

  x = key & (N - 1);
  for (i = 0; i < N; i++) {
    if (0 == keys[x] || key == keys[x])
      return x;
    if (++x == N)
      x = 0;
  }
  return N;
}

int hgets(Word *keys, int N, Word key, const char *s)
{
  int i, x;

  x = key & (N - 1);
  for (i = 0; i < N; i++) {
    if (0 == keys[x]) 
      return x;
    if (key == keys[x] && !compare(s, Syms[x]))
      return x;
    if (++x == N)
      x = 0;
  }
  return N;
}

int defined(int x)
{
  return (1 << (x & 7)) & SymDefd[x >> 3];
}

void define(int x)
{
  SymDefd[x >> 3] |= (1 << (x & 7));
}

void hput(const char *s, Byte b)
{
  Word h;
  int x;

  h = hash(s, 3);
  x = hget(OpKeys, NOPS, h);
  OpKeys[x] = h;
  OpVals[x] = b;
}

void hputs(const char *s, Word w)
{
  Word key;
  int x;

  key = hash(s, zcount(s));
  x = hgets(SymKeys, NSYMS, key, s);
  if (NSYMS == x) {
    errs("symbol table full");
    return;
  }
  if (0 == Syms[x]) {
    SymKeys[x] = key;
    Syms[x] = Txt; while ((*Txt++ = *s++));
    define(x);
  }
  SymVals[x] = w;
}

void fillH(void)
{
  int i, j, k;
  Byte opcode;
  const char *s;

  for (i = 0; i < NOPS; i++) {
    OpKeys[i] = 0;
    OpVals[i] = 0;
  }
  for (i = 0; i < 3; i++) {
    opcode = i;
    for (j = 0; j < 24; j += 3) {
      hput(ops[i] + j, opcode);
      opcode += 32;
    }
  }
  for (i = 0; i < 2; i++) {
    opcode = (i << 1);
    for (j = 0; j < 4; j++) {
      s = subops[i][j];
      opcode = (i << 1) + (j << 3);
      if (s)
        for (k = 0; k < 24; k += 3) {
          if (' ' != s[k+1])
            hput(s + k, opcode);
          opcode += 32;
        }
    }
  }
}

int within(int c, int lo, int hi) { return (lo <= c) && (c < hi); }
int between(int c, int lo, int hi) { return within(c, lo, hi + 1); }
int alphaq(int c) { return between(c, 'A', 'Z'); }
int numq(int c) { return between(c, '0', '9'); }

int digitq(int *d, int c, int base)
{
  int ret;

  if (256 == base) {
    *d = c;
    return alphaq(c);
  }
  ret = within(c, '0', MIN(base, 10));
  *d = c - '0';
  if (16 == base && !ret) {
    ret = between(c, 'A', 'F');
    if (ret)
      *d = c - 'A' + 10;
  }
  return ret;
}

const char* getsym(const char *s, char *sym)
{
  char c;
  int n;

  n = 0; c = *s;
  if (alphaq(c)) {
    while ((c = *s) && (alphaq(c) || numq(c))) {
      if (n < NSTR)
        sym[n++] = c;
      s++;
    }
    sym[n] = 0;
  }
  return n ? s : errs("invalid symbol");
}

const char* getarg(const char *s)
{
  int base, c, d;
  char pfx;

  pfx = 0; base = 10; c = *s;
  if ('~' == c || '<' == c || '>' == c) {
    pfx = c; c = *++s;
  }
  if ('*' == c) {
    Arg = P; ++s; goto Out;
  }
  else if ('$' == c) {
    base = 16; c = *++s;
  }
  else if ('@' == c) {
    base = 8; c = *++s;
  }
  else if ('%' == c) {
    base = 2; c = *++s;
  }
  else if ('\'' == c) {
    base = 256; c = *++s;
  }
  else if (alphaq(c)) {
    Word h;
    char sym[NSTR + 1];
    int x;

    if (!(s = getsym(s, sym))) return NULL;
    h = hash(sym, zcount(sym));
    x = hgets(SymKeys, NSYMS, h, sym);
    if (0 == Syms[x] || !defined(x))
      return errs("undefined symbol");
    Arg = SymVals[x];
    return s;
  }
  Arg = 0;
  while (digitq(&d, c, base)) {
    Arg = base * Arg + d;
    c = *++s;
  }
Out:
  if (pfx) {
    if ('~' == pfx) Arg = ~Arg;
    else if ('<' == pfx) Arg = LO(Arg);
    else if ('>' == pfx) Arg = HI(Arg);
  }
  return s;
}

int isop(char c)
{
  return NULL != scan("+-*/^&|", c, NULL);
}

const char* expr(const char *s)
{
  Word Arg1;
  char c, op;

  if (!(s = getarg(s))) return s;
  while ((c = *s++) && isop(c)) {
    Arg1 = Arg;
    op = c; if (!(s = getarg(s))) return s;
    switch(op){
    case '+': Arg = Arg1 + Arg; break;
    case '-': Arg = Arg1 - Arg; break;
    case '*': Arg = Arg1 * Arg; break;
    case '/': Arg = Arg1 - Arg; break;
    case '^': Arg = Arg1 ^ Arg; break;
    case '&': Arg = Arg1 & Arg; break;
    case '|': Arg = Arg1 | Arg; break;
    }
  }
  return s;
}

/* .byte expr [, expr] .dbyte .word */
/* label '=' expr */
/*  *=expr */

const char* getstr(const char *s)
{
  int n;
  char c;

  n = 0;
  while((c = *s++) && '\'' != c) {
    if (n < NSTR)
      cText[n++] = c;
  }
  cText[n] = 0;
  return s;
}

void sb(void) { M[P++] = LO(Arg); }
void sw(void) { sb(); Arg = HI(Arg); sb(); }
void sr(void) {
  int disp = Arg - (P + 1);
  if (disp < -128 || disp > 127) {
    errs("Bxx out of range");
    disp = 0;
  }
  Arg = LO(disp); sb();
}

int instemit(Byte opc)
{
  Word savP;

  savP = P;
  M[P++] = opc;
  switch(AMode){
  case 'V': case 'z': case '#': case 'W': case 'x': case 'y':
    sb(); break;
  case 'a': case 'Y': case 'X': case 'I':
    sw(); break;
  case 'r': sr(); break;
  case '.': break; /* implied */
  }
  return P - savP;
}

void pseudo(const char *s) {
  Word h;
  char sym[NSTR + 1];
  int cond;

  if (!(s = getsym(s, sym))) return;
  h = hash(sym, zcount(sym));
  if (hBYT == h || hDBY == h || hWOR == h)
    do {
      if (!(s = skip(s, ' '))) return;
      if (!(s = expr(s))) return;
      if (hBYT == h) sb();
      else if (hWOR == h) sw();
      else { Word tmp = Arg; Arg = HI(Arg); sb(); Arg = tmp; sb(); }
      if ((cond = ',' == *s)) s++;
    } while (cond);
  else errs("invalid pseudo");
}

void parse(const char *s) {
  char c;
  int paren, comma, idx;

  cLabel[0] = 0; cMnemo[0] = 0; Arg = 0; AMode = 0;
  E = 0;
  paren = comma = idx = 0;
  c = *s++;
  if (' ' != c) {
    /* label */
    cLabel[0] = c;
    s = scan(s, ' ', cLabel + 1);
  }
  s = skip(s, ' '); c = *s++;
  if (!c) return;
  /* mnemonic */
  if ('*' == c) { /* *=expr */
    c = *s++;
    if ('=' != c) { err(s); return; }
    if (!(s = expr(s))) return;
    P = Arg; return;
  } else if ('=' == c) { /* label '=' expr */
    if (!cLabel[0]) { errs("missing label"); return; }
    if (!(s = expr(s))) return;
    hputs(cLabel, Arg);
    return;
  }
  else if ('.' == c) {
    pseudo(s); return;
  }
  cMnemo[0] = c;
  s = scan(s, ' ', cMnemo + 1);
  if (3 != zcount(cMnemo)) {
    errs("unknown opcode"); return;
  }
  s = skip(s, ' '); c = *s++;
  if (!c) {
    AMode = '.'; return;
  }
  /* amode */
  if ('A' == c) {
    AMode = 'A'; return;
  }
  else if ('#' == c) {
    AMode = '#'; c = *s++;
  } else if ('(' == c) {
    c = *s++;
  }
  s = getarg(s); c = *s++;
  if (!AMode) AMode = Arg < 256 ? 'z' : 'a';

  /* aaa[,XY]')'[,XY] */
  if (',' == c) {
    comma = 1;
ParseIdx:
    c = *s++;
    if (!c) { err(s); return; }
    AMode = idx = c; c = *s++;
    if (paren) {
      AMode = 'W'; return;
    }
  }
  if (')' == c) {
    paren = 1;
    if (comma) {
      AMode = 'V'; return;
    }
    c = *s++;
  }
  if (',' == c) {
    if (comma) { err(s); return; }
    goto ParseIdx;
  }
  /* X,Y => x,y */
  if (idx) {
    if (Arg < 256) AMode = idx + 'a' - 'A';
  } else if (paren) AMode = 'I';
  else AMode = '-';
  return;
}

void b(Byte b) { emit('$'); wb(b); }
void w(Word w) { emit('$'); wb(HI(w)); wb(LO(w)); }
void r(Word loc, Byte disp) { if (disp > 127) disp -= 256; disp += loc; emit('$'); ww(disp); }
void prm(const char *s) {
    int i, n = '.' == *s ? 5 : 3;
    for (i = 0; i < n; i++)
        emit(*s++);
    bl();
}

void usage(void)
{
  type("usage: d65 rom.bin [addr]\n");
  exit(1);
}

#define ALLOT(n,T)  (T*)malloc((n)*sizeof(T))

void init(void)
{
  int i;

  OpKeys = ALLOT(NOPS, Word);
  OpVals = ALLOT(NOPS, Byte);
  fillH();

  SymKeys = ALLOT(NSYMS, Word);
  Syms = ALLOT(NSYMS, const char *);
  TxtArea = ALLOT(NSYMS * (NSTR + 1), char);
  SymDefd = ALLOT(NSYMS >> 3, Byte);
  SymVals = ALLOT(NSYMS, Word);

  for (i = 0; i < NSYMS; i++) {
    SymKeys[i] = 0;
    Syms[i] = NULL;
    SymDefd[i] = 0;
    SymVals[i] = 0;
  }
  for (i = 0; i < sizeof(SymDefd); i++)
    SymDefd[i] = 0;

  hBYT = hash("BYT", 3); hWOR = hash("WOR", 3); hDBY = hash("DBY", 3);
  hJSR = hash("JSR", 3); hJMP = hash("JMP", 3); hBIT = hash("BIT", 3);
  Txt = &TxtArea[0];
}

int isadr(char mode) { return ('a' == mode) || ('z' == mode); }

const char* decode(Byte op, char *amode) {
  int x, y, z;
  char m;
  const char *mnemo;

  /* I: zzzyyyxx */
  x = op & 3; y = (op >> 2) & 7; z = (op >> 5);
  /* get mnemo & amode */
  m = '-';
  if (x < 3) {
    mnemo = ops[x];
    m = amodes[x][z][y];
    if ('.' == m) {
      mnemo = subops[x >> 1][y >> 1];
      if (0x20 == op) m = 'a'; /*JSR*/
      if (4 == y && 0 == x) m = 'r'; /*Bxx*/
    }
    mnemo += 3 * z;
  }
  if ('-' == m) mnemo = ".byte";

  *amode = m;
  return mnemo;
}

Byte encode(char *amode) {
  int x;
  Word h;
  char m;
  Byte op;

  m = '-';
  h = hash(cMnemo, 3);
  if (hJSR == h) {
    if (isadr(AMode)) {
      op = 0x20; m = 'a';
    } else goto ErrOut;
  } else if (hJMP == h) {
    if ('I' == AMode) {
      op = 0x4C; m = 'I';
    } else if (isadr(AMode)) {
      op = 0x6C; m = 'a';
    } else goto ErrOut;
  } else {
    x = hget(OpKeys, NOPS, h);
    if (NOPS == x || 0 == OpKeys[x])
      errs("unknown opcode");
    op = OpVals[x];
    decode(op, &m);
    if (hBIT == h || '.' != m) {
      int x, z;
      const char *s, *p;
      /* split opcode */
      x = op & 3; z = (op >> 5);
      s = amodes[x][z];
      if ((p = scan(s, AMode, NULL))) {
        op += ((p - s) << 2) + x;
      } else goto ErrOut;
    }
  }

  *amode = m;
  return op;
ErrOut:
  errs("invalid amode");
  *amode = '.';
  return 0x00; /* BRK */
}


#define HILO(x)  ((M[x+1] << 8) + M[x])

Word disasm(Word P) {
  Byte I, disp;
  Word S;
  char m;
  const char *mnemo;

  mnemo = decode(I = M[P], &m);

  R = 0;
  ww(S = P++); bl(); prm(mnemo);
  switch(m){
  case 'V': emit('('); b(M[P++]); type(",X)"); break;
  case 'z': b(M[P++]); break;
  case '#': emit('#'); b(M[P++]); break;
  case 'a': w(HILO(P)); P += 2; break;
  case 'W': emit('('); b(M[P++]); type("),Y"); break;
  case 'x': b(M[P++]); type(",X"); break;
  case 'Y': w(HILO(P)); type(",Y"); break;
  case 'X': w(HILO(P)); type(",X"); break;
  case 'A': emit('A'); break;
  case 'I': emit('('); w(HILO(P)); emit(')'); break;
  case 'y': b(M[P++]); type(",Y"); break;
  case 'r': disp = M[P++]; r(P, disp); break;
  case '-': wb(I); break;
  case '.': break; /* implied */
  default: type("unknown amode="); emit(m); cr(); exit(1);
  };
  while (R < 17) bl();
  while (S < P) wb(M[S++]);
  cr();

  return P;
}

int main(int argc, char*argv[]) {
  FILE *fin;
  Word nBytes, LIMIT;
  Byte *B;

  if (argc < 2)
    usage();

  fin = fopen(argv[1], "rb");
  if (NULL == fin)
    usage();

  P = 0;
  if (argc > 2)
    P = strtol(argv[2], NULL, 16);

  fseek(fin, 0, SEEK_END);
  nBytes = ftell(fin);
  fseek(fin, 0, SEEK_SET);

  B = (Byte*) malloc(nBytes);
  LIMIT = P + nBytes;
  nBytes -= fread(B, 1, nBytes, fin);
  fclose(fin);

  /* adjust M to M'[P] = M[0] */
  M = B - P;
  while (P < LIMIT)
      P = disasm(P);

  free(B);
  return 0;
}

/* vim:set ts=2 sw=2 et: */
