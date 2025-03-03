/* 6502 disassembler	*/
/* (c) 2025 pahihu 	*/

#include <stdlib.h>
#include <stdio.h>

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
#define NSTR  8
Byte *M;
Word P;
int R;
char Line[NLINE + 1], Label[NSTR], Mnemo[NSTR], AMode; /* replace ';' with NUL */
Word Addr;

#define NKEYS 256
Word keys[NKEYS];
Byte vals[NKEYS];


#define LO(x) ((x) & 0xFF)
#define HI(x) LO((x) >> 8)

void emit(char c) { R++; putchar(c); }
void bl() { emit(' '); }
void cr() { emit('\n'); }
void type(char *s) { char c; while ((c = *s++)) emit(c); }
void wn(Byte x) { emit("0123456789ABCDEF"[x]); }
void wb(Byte x) { wn(x >> 4); wn(x & 15); }
void ww(Word w) { wb(HI(w)); wb(LO(w)); }
const char* skip(const char *s, char c) {
  char p;

  p = *s++;
  while (p && p == c)
    p = *s++;
  return s;
}
const char* scan(const char *s, char c, char *dst) {
  char p;
  int x;

  x = 0; p = *s++;
  while (p && p != c) {
    if (x < NSTR)
      dst[x++] = p;
    p = *s++;
  }
  dst[x] = 0;
  return s;
}

int hget(Word *keys, Word key)
{
  int x;

  x = key & 0xFF;
  for (;;) {
    if (0 == keys[x] || key == keys[x])
      return x;
    if (++x == NKEYS)
      x = 0;
  }
  return 0;
}

Word hash(const char *s, int n)
{
  int i;
  Word h;

  h = 0;
  for (i = 0; i < n; i++)
    h = (27 * h) + (s[i] - 'A') + 1;

  return h;
}

void hput(const char *s, int n, Byte b)
{
  Word h;
  int x;

  h = hash(s, n);
  x = hget(keys, h);
  keys[x] = h;
  vals[x] = b;
}

void fillh()
{
  int i, j, k;
  Byte opcode;
  const char *s;

  for (i = 0; i < NKEYS; i++) {
    keys[i] = 0;
    vals[i] = 0;
  }
  for (i = 0; i < 3; i++) {
    opcode = i;
    for (j = 0; j < 24; j += 3) {
      hput(ops[i] + j, 3, opcode);
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
            hput(s + k, 3, opcode);
          opcode += 32;
        }
    }
  }
}

int err(const char *s) {
  Line[s - Line] = 0;
  type(Line); emit('?');
  return 1;
}

const char* hex(const char *s)
{
  /* TODO */
  return s;
}

int parse(const char *s) {
  char c;
  int paren, comma, idx;

  Label[0] = 0; Mnemo[0] = 0; Addr = 0; AMode = 0;
  paren = comma = idx = 0;
  c = *s++;
  if (' ' != c) {
    /* label */
    Label[0] = c;
    s = scan(s, ' ', Label + 1);
  } 
  s = skip(s, ' '); c = *s++;
  if (!c) return 0;
  /* mnemonic */
  Mnemo[0] = c;
  s = scan(s, ' ', Mnemo + 1);
  s = skip(s, ' '); c = *s++;
  if (!c) {
    AMode = '.'; return 0;
  }
  /* amode */
  if ('A' == c) {
    AMode = 'A'; return 0;
  }
  else if ('#' == c) {
    AMode = '#'; c = *s++;
  } else if ('(' == c) {
    c = *s++;
  }
  if ('$' != c) return err(s);
  s = hex(s); c = *s++;
  if (!AMode) AMode = Addr < 256 ? 'z' : 'a';

  /* aaa[,XY]')'[,XY] */
  if (',' == c) {
    comma = 1;
ParseIdx:
    c = *s++;
    if (!c) return err(s);
    AMode = idx = c; c = *s++;
    if (paren) {
      AMode = 'W'; return 0;
    }
  }
  if (')' == c) {
    paren = 1;
    if (comma) {
      AMode = 'V'; return 0;
    }
    c = *s++;
  }
  if (',' == c) {
    if (comma) return err(s);
    goto ParseIdx;
  }
  /* X,Y => x,y */
  if (idx) {
    if (Addr < 256) AMode = idx + 'a' - 'A';
  } else if (paren) AMode = 'I';
  else AMode = '-';
  return 0;
}

void b() { emit('$'); wb(M[P++]); }
void w() { emit('$'); wb(M[P+1]); wb(M[P]); P+=2; }
void r() { int disp = M[P++]; if (disp > 127) disp -= 256; disp += P; emit('$'); ww(disp); }
void prm(const char *s) {
    int i, n = '.' == *s ? 5 : 3;
    for (i = 0; i < n; i++)
        emit(*s++);
    bl();
}

void usage()
{
  type("usage: d65 rom.bin [addr]\n");
  exit(1);
}

int main(int argc, char*argv[])
{
  FILE *fin;
  Word nBytes, LIMIT;
  Byte *B;

  if (argc < 2)
    usage();

  fin = fopen(argv[1], "rb");
  if (NULL == fin)
    usage();

  fseek(fin, 0, SEEK_END);
  nBytes = ftell(fin);
  fseek(fin, 0, SEEK_SET);

  B = (Byte*) malloc(nBytes);
  nBytes -= fread(B, 1, nBytes, fin);
  fclose(fin);

  P = 0;
  if (argc > 2)
    P = strtol(argv[2], NULL, 16);

  /* adjust M to M'[P] = M[0] */
  M = B - P; LIMIT = P + nBytes;
  while (P < LIMIT) {
      Byte I;
      Word S;
      char m;
      const char *mnemo;
      int x, y, z;

      I = M[P];
      /* I: zzzyyyxx */
      x = I & 3; y = (I >> 2) & 7; z = (I >> 5);
      /* get mnemo & amode */
      m = '-';
      if (x < 3) {
          mnemo = ops[x];
          m = amodes[x][z][y];
          if ('.' == m) {
              mnemo = subops[x >> 1][y >> 1];
              if (0x20 == I) m = 'a'; /*JSR*/
              if (4 == y && 0 == x) m = 'r'; /*Bxx*/
          }
          mnemo += 3 * z;
      }
      if ('-' == m) mnemo = ".byte";
    
      R = 0;
      ww(S = P++); bl(); prm(mnemo);
      switch(m) {
      case 'V': emit('('); b(); type(",X)"); break;
      case 'z': b(); break;
      case '#': emit('#'); b(); break;
      case 'a': w(); break;
      case 'W': emit('('); b(); type("),Y"); break;
      case 'x': b(); type(",X"); break;
      case 'Y': w(); type(",Y"); break;
      case 'X': w(); type(",X"); break;
      case 'A': emit('A'); break;
      case 'I': emit('('); w(); emit(')'); break;
      case 'y': b(); type(",Y"); break;
      case 'r': r(); break;
      case '-': wb(I); break;
      case '.': break; /* implied */
      default: type("unknown amode="); emit(m); cr(); exit(1);
      };
      while (R < 17) bl();
      while (S < P) wb(M[S++]);
      cr();
  }

  free(B);
  return 0;
}

/* vim:set ts=2 sw=2 et: */
