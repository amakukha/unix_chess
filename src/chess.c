#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

// * * * CONSTANTS * * *

#define uleft   04040
#define uright  04004
#define dleft   00440
#define dright  00404
#define left    00040
#define right   00004
#define up      04000
#define down    00400
#define u2r1    06004
#define u1r2    04006
#define d1r2    00406
#define d2r1    00604
#define d2l1    00640
#define d1l2    00460
#define u1l2    04060
#define u2l1    06040
#define rank2   00200
#define rank7   02000

#define MAX_TOTAL_VALUE 13800
#define IGNORE  0

// * * * GLOBAL VARIABLES AND CONSTANTS * * *

int     attacv[64];
int     center[64] = {
            2,3,4,4,4,4,3,2,
            3,6,8,8,8,8,6,3,
            4,8,12,12,12,12,8,4,
            4,8,12,14,14,12,8,4,
            4,8,12,14,14,12,8,4,
            4,8,12,12,12,12,8,4,
            3,6,8,8,8,8,6,3,
            2,3,4,4,4,4,3,2
};

int     wheur1() {}
int     wheur2() {} 
int     wheur3() {}
int     wheur4() {}
int     wheur5() {}
int     wheur6() {}
int (*wheur[])() = {
        &wheur1,
        &wheur2,
        &wheur3,
        &wheur4,
        &wheur5,
        &wheur6,
        0
};

int     bheur1() {}
int     bheur2() {}
int     bheur3() {}
int     bheur4() {}
int     bheur5() {}
int     bheur6() {}
int (*bheur[])() = {
        &bheur1,
        &bheur2,
        &bheur3,
        &bheur4,
        &bheur5,
        &bheur6,
        0
};

int     control[64];
int     clktim[2];      // clock time for each player; 0 - white
int     testf;
int     qdepth = 8;
int     mdepth = 4;
int     bookf;          // book file
int     bookp;          // first two bytes of the book?
int     manflg;
int     matflg;
int     intrp;
int     moveno = 1;     // move number (counts pairs of moves)
int     gval;           // who is winning? (ternary value)
int     game;           // "stage" of the game
int     abmove;         // alpha-beta move? (next move)
int     *lmp;           // pointer to lmbuf
int     *amp;           // pointer to ambuf
char    *sbufp;
int     lastmov;
bool    mantom = false; // true = White, false = Black
int     ply;
int     value;          // position value
int     ivalue;
int     mfmt;
int     depth = 2;
int     flag = 033;
int     eppos = 64;
int     bkpos = 4;      // black king position
int     wkpos = 60;     // white king position
int     column;
const int edge[8] = { 040, 020, 010, 0, 0, 1, 2, 4 };
int     pval[13];
const int ipval[13] = { // initial piece values
            -3000, -900, -500, -300, -300, -100,
            0,
            100, 300, 300, 500, 900, 3000
};
int     dir[64];        // generated from edge
// Negative: white
// Positive: black
int     board[64] = {
            4, 2, 3, 5, 6, 3, 2, 4,
            1, 1, 1, 1, 1, 1, 1, 1,
            0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0,
            -1, -1, -1, -1, -1, -1, -1, -1,
            -4, -2, -3, -5, -6, -3, -2, -4,
};
int     lmbuf[1000];
int     ambuf[1200];
char    sbuf[100];      // string buffer

// * * * LOGIC * * *

void _qsort(int* a, int* b) {
    // TODO: defined in qsort.s
}

int clock() {
    // TODO: defined in qsort.s
    return 0;
}

void show_clock_time(const char *color, t) { // original name: ctime
    printf("%s: %d:%d%d\n", color, t/60, (t/10)%6, t%10);
}

bool wattack(int bkpos) {
    // TODO
    return false;
}

bool battack(int wkpos) {
    // TODO
    return false;
}

bool check() {
    return !wattack(bkpos) || !battack(wkpos);
}

void increm() {
    clktim[mantom] += clock();
    if (mantom)
        moveno++;
    mantom = !mantom;
}

void decrem() {
    mantom = !mantom;
    if (mantom)
        moveno--;
}

/**
 * Determines "stage" of the game.
 * There are four stages: from 0 to 3.
 * Stages 2 and 3 are determined by how much material left on both sides.
 */
void stage() {
    int i, a;

    qdepth = depth + 8;      // TODO: why?

    for (i=0; i<13; i++) { // TODO: memcopy
            pval[i] = ipval[i];
    }

    // Calculate current value
    value = 0;
    for (i=0; i<64; i++) {
            a = board[i];
            value += (pval+6)[a];
    }

    // Who is winning?
    if (value > 150) {
            gval = 1; 
    } else if (value < -150) {
            gval = -1; 
    } else {
            gval = 0;
    }

    // Modify piece values
    // TODO: what's the point of this?
    for (i = -6; i <= 6; i++) {
            a = (pval+6)[i];
            a += (a < 0) ? -50 : 50;
            a /= 100;
            if (i) {
                    (pval+6)[i] = a*100 - gval;
            }
    }

    // Calculate total remaining value
    a = MAX_TOTAL_VALUE;
    for (i = 64; i--;) {
            a -= abs((pval+6)[board[i]]);
    }

    // Determine the game stage
    if (a > 4000) {
            game = 3;
    } else if (a > 2000) {
            game = 2;
    } else if (moveno > 5) {
            game = 1; 
    } else {
            game = 0;
    }
}

void bmove(int m) {
    // TODO: defined in bmove.s
}

void bremove() {
    // TODO: defined in bmove.s
}

void wmove(int m) {
    // TODO: defined in wmove.s
}

void wremove() {
    // TODO: defined in wmove.s
}

// TODO: position?
void posit(void (*f)(), int* p, int a) {
        int m; // TODO: what is it?

        // TODO: what is `amp`?
        while (amp != p) {
                m = amp[3] << 8;
                m |= amp[4] & 0xFF;
                (*f)(m, a);
                if (mantom) {
                        bmove(m);
                        moveno++;
                        mantom = 0;
                } else {
                        wmove(m);
                        mantom = 1;
                }
        }
}

/**
 * Compare two extended boards. If equal, increment 66th byte.
 */
void rept1(int m, int* a) {
    // Same player?
    if (mantom != a[64]) {
        return;
    }
    // Compare two boards
    for (int i=0; i<64; i++) {
        if(board[i] != a[i])
            return;
    }
    a[65]++;
}

/**
 * @return Number of repetitions
 */
int repetition() {
    // original name: rept()

    // TODO: how does it work?
    int a[66], *p, i;

    for (i=0; i<64; i++) { // TODO: memcpy
        a[i] = board[i];
    }
    a[64] = mantom;
    a[65] = 0;
    p = amp;
    while (amp[-1] != -1) {
        if (amp[-2])
            break;
        i = board[amp[-3]];
        if (i == 1 || i == -1)
            break;
        mantom ? wremove(): bremove();
        decrem();
    }
    posit(rept1, p, a);
    return a[65]; // repetition counter
}

/**
 * Book move
 */
int bookm() {
    return 0;
    // TODO
}

void pause() {
    printf("...\n");
}

void putdig(n) {
    char *s;

    s = "god only knows";
    switch(n) {

    case 0:
            s = "zero";
            break;

    case 1:
            s = "one";
            break;

    case 2:
            s = "two";
            break;

    case 3:
            s = "three";
            break;

    case 4:
    case 15:
            s = "four";
            break;

    case 5:
            s = "five";
            break;

    case 6:
    case 17:
            s = "six";
            break;

    case 7:
    case 18:
            s = "seven";
            break;

    case 8:
    case 19:
            s = "eight";
            break;

    case 9:
    case 20:
            s = "nine";
            break;

    case 10:
            s = "ten";
            break;

    case 11:
            s = "eleven";
            break;

    case 12:
            s = "twelve";
            break;

    case 13:
            s = "twen";
            break;

    case 14:
            s = "thir";
            break;

    case 16:
            s = "fif";
            break;
    }
    printf(s);
}

/**
 * Convert a number to English words and print it.
 */
void putnumb(n) {
    if (n <= 12) {
        // case 0..12
        putdig(n);
        putchar('\n');
        return;
    }
    if (n <= 19) {
        // case 13..19
        putdig(n+1);
        printf("teen\n");
        return;
    }
    if (n >= 100) {
        // case >= 100
        putnumb(n/100);
        printf("hundred\n");
        n %= 100;
        if (n) {
            putnumb(n);
        }
        return;
    }
    // case 20..99
    putdig(n/10 + 11);
    printf("ty\n"); // original: tee
    n %= 10;
    if (n) {
        putnumb(n);
    }
}

void putpiece(char p) {
    char *s;

    s = "god only knows";
    switch(p) {

    case 'p':
        s = "pawn";
        break;

    case 'n':
        s = "knight";
        break;

    case 'b':
        s = "bishop";
        break;

    case 'r':
        s = "rook";
        break;

    case 'q':
        s = "queen";
        break;

    case 'k':
        s = "king";
        break;
    }
    printf("%s\n", s);
}

void stdp(int p) {
    if(p < 0)
        p = -p;
    p = "ppnbrqk"[p];
    putpiece(p);
}

void stdb(int b) {
    int r, f;

    r = b/8;
    if((f = b%8) < 4)
        putpiece('q'); else {
        putpiece('k');
        f = 7-f;
    }
    f = "rnb\0"[f];
    if(f)
        putpiece(f);
    putnumb(mantom? r+1: 8-r);
}

int out(int m) {
        int from, to, epf, pmf;

        from = m >> 8;
        to = m & 0xFF;
        mantom ? bmove(m) : wmove(m);
        epf = pmf = 0;
        switch(amp[-1]) {

        case 0:
        case 1:
                stdp(board[to]);
        ed:
                printf("at\n");
                stdb(from);
                if(amp[-2]) {
                        printf("takes\n");
                        stdp(amp[-2]);
                        printf("at\n");
                } else
                        printf("to\n");
                stdb(to);
                break;

        case 3:
                printf("castle queen side\n");
                break;

        case 2:
                printf("castle king side\n");
                break;

        case 4:
                epf = 1;
                putpiece('p');
                goto ed;

        case 5:
                pmf = 1;
                putpiece('p');
                goto ed;
        }
        if(pmf) {
                printf("becomes\n");
                putpiece('q');
        }
        if(epf) {
                printf("en passent\n");
        }
        if(check())
                printf("check\n");
        mantom ? bremove() : wremove();
}

void out1(int m) {
    putnumb(moveno);
    pause();
    out(m);
    pause();
}

void makmov(int m) {
    out1(m);
    mantom ? bmove(m) : wmove(m);
    increm();

// TODO
//      int buf[2];
//
//      if (!bookp) return;
//      lseek(bookf, (long)(unsigned)bookp, 0);
//
//loop:
//      read(bookf, buf, 4);
//      *buf = booki(*buf);
//      if(m == *buf || *buf == 0) {
//              bookp = buf[1] & ~1;
//              goto l1;
//      }
//      if(*buf < 0) {
//              bookp = 0;
//              goto l1;
//      }
//      goto loop;
//
//l1:
//      if(!bookp) {
//              putchar('\n');
//              return;
//      }
}

int booki(int m) {
// TODO
//      int i;
//      struct {
//              char low;
//              char high;
//      };
//      i.high = m.low;
//      i.low = m.high;
//      return(i);
}

int mate(int a, int b) {
    // TODO
}

bool bcheck(int from, int to) {
    if (board[to] > 0) {
            return true; // TODO: cannot move into a black piece? 
    }
    *lmp++ = (pval+6)[board[to]] - value;
    *lmp++ = (from<<8) | to;
    return (board[to] != 0); // TODO: beating a white piece?
}

bool wcheck(int from, int to) {
    if (board[to] < 0) {
        return true;
    }
    *lmp++ = value - (pval+6)[board[to]];
    *lmp++ = (from<<8) | to;
    return (board[to] != 0);
}

void btry(int from, int mask, int offset) {
    if ((dir[from] & mask) == 0)
        bcheck(from, from + offset);
}

void wtry(int from, int mask, int offset) {
    if ((dir[from] & mask) == 0)
        wcheck(from, from + offset);
}

void wgen() {
    // TODO: defined in wgen.s
}

void bgen() {
    // TODO: defined in bgen.s
}

void bagen() {
        int *p1, *p2, v;

        p1 = lmp;
        if ((flag&010)!=0)
        if (board[5]==0 && board[6]==0 && board[7]==4)
        if (wattack(4) && wattack(5) && wattack(6))
                btry(4, 0, 2); /* kingside castle */
        if ((flag&020)!=0)
        if (board[0]==4 && board[1]==0 && board[2]==0 && board[3]==0)
        if (wattack(2) && wattack(3) && wattack(4))
                btry(4, 0, -2); /* queenside castle */
        bgen();
        p2 = p1;
        while(p2 != lmp) {
                v = *p2++;
                bmove(*p2);
                if(wattack(bkpos)) {
                        *p1++ = v;
                        *p1++ = *p2;
                }
                p2++;
                bremove();
        }
        lmp = p1;
}

void wagen() {
        int *p1, *p2, v;

        p1 = lmp;
        if ((flag&1)!=0)
        if (board[61]==0 && board[62]==0 && board[63]== -4)
        if (battack(60) && battack(61) && battack(62))
                wtry(60, 0, 2); /* kingside castle */
        if ((flag&2)!=0)
        if (board[56]== -4 && board[57]==0 && board[58]==0 && board[59]==0)
        if (battack(58) && battack(59) && battack(60))
                wtry(60, 0, -2); /* queenside castle */
        wgen();
        p2 = p1;
        while(p2 != lmp) {
                v = *p2++;
                wmove(*p2);
                if(battack(wkpos)) {
                        *p1++ = v;
                        *p1++ = *p2;
                }
                p2++;
                wremove();
        }
        lmp = p1;
}

int wstatic(int f) {
    int i, j, h, (*p)();

    h = i = 0;
    while(p = wheur[h++]) {
        j = (*p)();
        if(f)
            printf("%4d ", j);
        i += j;
    }
    if(f)
        printf("=%4d ", i);
    return(-i);
}

int bstatic(int f) {
    int i, j, h, (*p)();

    h = i = 0;
    while(p = bheur[h++]) {
        j = (*p)();
        if(f)
            printf("%4d ", j);
        i += j;
    }
    if(f)
        printf("=%4d ", i);
    return(-i);
}

int* statl() {
    int *p1, *p2, *p3;

    p1 = p2 = lmp;
    stage();
    mantom ? bagen() : wagen();
    if(lmp == p1+1)
            return(p1);
    while(p2 != lmp) {
        p3 = p2++;
        if (mantom) {
            bmove(*p2++);
            *p3 = bstatic(0);
            bremove();
        } else {
            wmove(*p2++);
            *p3 = wstatic(0);
            wremove();
        }
    }
    _qsort(p1, lmp);
    return(p1);
}

int xheur(int ploc) {
    int *p1, *p2, from, to, pie;

    pie = board[ploc];
    p1 = lmp;
    p2 = p1;
    mantom ? wgen() : bgen();
    while(p2 != lmp) {
        p2++;
        to = *p2++ & 0xFF;
        if(to == ploc) {
            from = p2[-1] >> 8;
            if(abs(board[from]) < abs(pie)) {
                lmp = p1;
                return((pval+6)[pie]/60);
            }
        }
    }
    lmp = p1;
    return(0);
}

void srnd1(int p, int m, int o) {
    if ((dir[p]&m) == 0)
        control[p+o] += 10;
}

void srnd(int p) {
    srnd1(p, uleft, -9);
    srnd1(p, uright, -7);
    srnd1(p, dleft, 7);
    srnd1(p, dright, 9);
    srnd1(p, up, -8);
    srnd1(p, left, -1);
    srnd1(p, right, 1);
    srnd1(p, down, 8);
    srnd1(p, 0, 0);
}

int wplay1(int);

int bplay() {
        int v1, v2, *p1, *p2, *p3, ab;

        if(value > ivalue)
                ivalue = value;
        ab = 0;
        v1 = -3000;
        ply = 0;
        p1 = statl();
        if(lmp == p1+2) {
                abmove = p1[1];
                lmp = p1;
                return(ivalue);
        }
        p2 = p1;
        mantom = !mantom;
        while(p2 != lmp) {
                p2++;
                bmove(*p2);
                if(testf) {
                        mantom = !mantom;
                        bstatic(1);
                        mantom = !mantom;
                }
                if(repetition())
                        v2 = 0;
                else
                        v2 = wplay1(v1);
                if(v2 > v1 && !mate(3, 0)) {
                        ab = *p2;
                        v1 = v2;
                }
                bremove();
                if(testf) {
                        mantom = !mantom;
                        printf("%6d ", v2);
                        out(*p2);
                        printf("\n");
                        mantom = !mantom;
                }
                p2++;
        }
        if(ab == 0 && lmp != p1)
                ab = p1[1];
        mantom = !mantom;
        lmp = p1;
        abmove = ab;
        return(v1);
}

int bquies(int ab);

int bplay1(int ab) {
        int v1, v2, *p1, *p2;

        if(ply >= depth)
                return(bquies(ab));
        ply++;
        p1 = p2 = lmp;
        bgen();
        _qsort(p1, lmp);
        v1 = -3000;
        while(p2 != lmp) {
                if(intrp)
                        goto out;
                p2++;
                bmove(*p2);
                if(wattack(bkpos)) {
                        v2 = wplay1(v1);
                        if(v2 > v1)
                                v1 = v2;
                }
                bremove();
                if(v1 >= ab)
                        goto out;
                p2++;
        }
out:
        ply--;
        lmp = p1;
        if(v1 == -3000) {
                v1++;
                if(!check())
                        v1 = 0;
        }
        return(v1);
}

int wquies(int ab) {
        int *p1, *p2, *p3, v1, v2;

        if(ply >= qdepth)
                return(ivalue);
        p1 = p2 = p3 = lmp;
        wgen();
        while(p2 != lmp) {
                v1 = *p2++;
                if(v1 != value && v1 <= ivalue+50) {
                        *p3++ = (((pval+6)[board[*p2>>8]]/100)<<8) |
                                (-(pval+6)[board[*p2&0377]]/100);
                        *p3++ = *p2;
                }
                p2++;
        }
        if(p3 == p1) {
                lmp = p1;
                return(value);
        }
        ply++;
        _qsort(p1, p3);
        lmp = p3;
        p2 = p1;
        v1 = value;
        while(p2 != lmp) {
                p2++;
                wmove(*p2);
                if(battack(wkpos)) {
                        v2 = bquies(v1);
                } else
                        v2 = 3000;
                if(v2 < v1)
                        v1 = v2;
                wremove();
                if(v1 <= ab)
                        goto out;
                p2++;
        }
out:
        ply--;
        lmp = p1;
        return(v1);
}

int bquies(int ab) {
        int *p1, *p2, *p3, v1, v2;

        if(ply >= qdepth)
                return(ivalue);
        p1 = p2 = p3 = lmp;
        bgen();
        while(p2 != lmp) {
                v1 = -(*p2++);
                if(v1 != value && v1 >= ivalue-50) {
                        *p3++ = ((-(pval+6)[board[*p2>>8]]/100)<<8) |
                                ((pval+6)[board[*p2&0377]]/100);
                        *p3++ = *p2;
                }
                p2++;
        }
        if(p3 == p1) {
                lmp = p1;
                return(value);
        }
        ply++;
        _qsort(p1, p3);
        lmp = p3;
        p2 = p1;
        v1 = value;
        while(p2 != lmp) {
                p2++;
                bmove(*p2);
                if(wattack(bkpos)) {
                        v2 = wquies(v1);
                } else
                        v2 = -3000;
                if(v2 > v1)
                        v1 = v2;
                bremove();
                if(v1 >= ab)
                        goto out;
                p2++;
        }
out:
        ply--;
        lmp = p1;
        return(v1);
}

int wplay() {
        int v1, v2, *p1, *p2, *p3, ab;

        if(value < ivalue)
                ivalue = value;
        ab = 0;
        v1 = 3000;
        ply = 0;
        p1 = statl();
        if(lmp == p1+2) {
                abmove = p1[1];
                lmp = p1;
                return(ivalue);
        }
        p2 = p1;
        mantom = !mantom;
        while(p2 != lmp) {
                p2++;
                wmove(*p2);
                if(testf) {
                        mantom = !mantom;
                        wstatic(1);
                        mantom = !mantom;
                }
                if(repetition())
                        v2 = 0; else
                        v2 = bplay1(v1);
                if(v2 < v1 && !mate(3, 0)) {
                        ab = *p2;
                        v1 = v2;
                }
                wremove();
                if(testf) {
                        mantom = !mantom;
                        printf("%6d ", v2);
                        out(*p2);
                        printf("\n");
                        mantom = !mantom;
                }
                p2++;
        }
        if(ab == 0 && lmp != p1)
                ab = p1[1];
        mantom = !mantom;
        lmp = p1;
        abmove = ab;
        return(v1);
}

int wplay1(int ab) {
        int v1, v2, *p1, *p2;

        if(ply >= depth)
                return(wquies(ab));
        ply++;
        p1 = p2 = lmp;
        wgen();
        _qsort(p1, lmp);
        v1 = 3000;
        while(p2 != lmp) {
                if(intrp)
                        goto out;
                p2++;
                wmove(*p2);
                if(battack(wkpos)) {
                        v2 = bplay1(v1);
                        if(v2 < v1)
                                v1 = v2;
                }
                wremove();
                if(v1 <= ab)
                        goto out;
                p2++;
        }
out:
        ply--;
        lmp = p1;
        if(v1 == 3000) {
                v1--;
                if(!check())
                        v1 = 0;
        }
        return(v1);
}

int xplay() {
    int a;

    stage();
    abmove = 0;
    a = mantom ? bplay(): wplay();
    ivalue = a;
    return a;
}

void itinit() {
    // TODO: defined in qsort.s
}

void onhup() {
    // TODO: defined in qsort.s
}

int* done() {
    int *p;

    if (repetition() > 3) {
        printf("Draw by repetition\n");
        onhup();
    }
    p = lmp;
    mantom ? bagen(): wagen();
    if (p == lmp) {
        if (check()) {
            printf("%s wins\n", mantom ? "White" : "Black");
        } else {
            printf("Stale mate\n");
        }
        onhup();
    }
    return(p);
}

int cooin() {
    int a, b;

    a = sbufp[0];
    if(a<'a' || a>'h') return(0);
    b = sbufp[1];
    if(b<'1' || b>'8') return(0);
    sbufp += 2;
    a = (a-'a')+8*('8'-b);
    return(a);
}

int algin() {
    int from, to;

    from = cooin();
    to = cooin();
    if(*sbufp != '\0') return(0);
    return((from<<8)|to);
}

/**
 * Copy string `s` to `sbuf`
 */
void spread(char* s) {
    char *p;
    p = sbuf;
    // TODO: replace with strncpy
    while ((*p++ = *s++));
}

/**
 * Read line
 */
void rline() {
        char *p1;
        int c;

loop0:
        p1 = sbuf;
loop:
        c = getchar();
        if (c <= 0)
                exit(0);
        if (c == '#')
                goto loop0;
        if (c != '*') {
                *p1++ = c;
                goto loop;
        }
        switch (getchar()) {

        case '#':
                goto loop0;

        case '*':
                if (p1 != sbuf+4) {
                        printf("bad input\n");
                        goto loop0;
                }
                sbuf[0] += 'a' - '1';
                sbuf[2] += 'a' - '1';
                *p1++ = '\0';
                return;

        case '0':
                exit(0);

        case '1':
                spread("");
                return;

        case '2':
                spread("first");
                return;

        case '3':
                spread("clock");
                return;

        case '4':
                spread("score");
                return;

        case '5':
                spread("remove");
                return;

        case '6':
                spread("repeat");
                return;

        case '7':
                spread("save");
                return;

        case '8':
                spread("restore");
                return;
        }
        printf("bad option\n");
        goto loop;
}

/** 
 * Match input to `sbuf`.
 * @return true if `s` matches `sbuf`.
 */
bool match(const char* s) {
    char *p1;
    int c;

    p1 = sbufp;
    while ((c = *s++) != '\0') {
        if (*p1++ != c) {
            return false;
        }
    }
    sbufp = p1;
    return true;
}

void save() {
// TODO
//      int i;
//      int f;
//
//      f = creat("chess.out", 0666);
//      if(f < 0) {
//              printf("cannot create file\n");
//              return;
//      }
//      write(f, clktim, 4);
//      write(f, &bookp, 2);
//      write(f, &moveno, 2);
//      write(f, &game, 2);
//      i = amp - ambuf;
//      write(f, &i, 2);
//      write(f, &mantom, 2);
//      write(f, &value, 2);
//      write(f, &ivalue, 2);
//      write(f, &depth, 2);
//      write(f, &flag, 2);
//      write(f, &eppos, 2);
//      write(f, &bkpos, 2);
//      write(f, &wkpos, 2);
//      write(f, board, 128);
//      write(f, ambuf, i*2);
//      close(f);
}

void restore() {
//      int i;
//      int f;
//
//      f = open("chess.out", 0);
//      if(f < 0) {
//              printf("cannot open file\n");
//              return;
//      }
//      read(f, clktim, 4);
//      read(f, &bookp, 2);
//      read(f, &moveno, 2);
//      read(f, &game, 2);
//      read(f, &i, 2);
//      amp = ambuf+i;
//      read(f, &mantom, 2);
//      read(f, &value, 2);
//      read(f, &ivalue, 2);
//      read(f, &depth, 2);
//      read(f, &flag, 2);
//      read(f, &eppos, 2);
//      read(f, &bkpos, 2);
//      read(f, &wkpos, 2);
//      read(f, board, 128);
//      read(f, ambuf, i*2);
//      close(f);
}

int manual();

void move() {
    int a, *p, *p1;

    while (true) {
        lmp = done();
        a = manual();
        p = done();
        p1 = p;
        while (p1 != lmp) {
            p1++;
            if (*p1++ == a) {
                lmp = p;
                makmov(a);
                return;
            }
        }
        printf("Illegal move\n");
        lmp = p;
    }
}

void times(int* p) {
    // TODO: what's this?
}

void play(int f) {
        int t, i, ts[9];

        // TODO: why is this needed?
        clock();
        ts[8] = 0;
        if(f) goto first;
loop:
        intrp = 0;
        move();

first:
        if(manflg)
                goto loop;
        i = mantom;
        t = clktim[i];
        if(!bookm())
        if(!mate(mdepth, 1))
                xplay();
        if(intrp) {
                decrem();
                mantom? bremove(): wremove();
                goto loop;
        }
        if(!abmove) {
                printf("Resign\n");
                onhup();
        }
        makmov(abmove);
        i = clktim[i];
        t = i-t;
        times(ts);      // TODO: what is this?
        ts[8] = ts[1];
        if(i/moveno > 150) {
                if(depth > 1)
                        goto decr;
                goto loop;
        }
        if(depth==3 && t>180)
                goto decr;
        if(depth==1 && t<60)
                goto incr;
        if(game==3 && t<60 && depth==2)
                goto incr;
        goto loop;

incr:
        depth++;
        goto loop;

decr:
        goto loop;
}

void score1(m) {
    if (!mantom) {
        putnumb(moveno);
        pause();
    }
    out(m);
    pause();
}

void score() {
    int *p;

    p = amp;
    while (amp[-1] != -1) {
        mantom ? wremove() : bremove();
        decrem();
    }
    posit(score1, p, IGNORE);
    printf("the end\n");
}

/**
 * Print board.
 * TODO: this doesn't look like V6 chess? 
 */
void pboard() {
    int i, x, y, c, p;

    i = 0;
    x = 8;
    while (x--) {
        y = 8;
        while (y--) {
            p = board[i++];
            if (p == 0) {
                printf("space\n");
            } else {
                printf((p < 0) ? "white " : "black ");
                putpiece("kqrbnp pnbrqk"[p+6]);
            }
        }
        pause();
        printf("end\n");
        pause();
    }
}

void setup() {
        char bd[64];
        char *p, *ip;
        int i, err, nkng, c;
        int wkp, bkp;

        for(p=bd; p<bd+64; )
                *p++ = 0;
        err = 0;
        nkng = 101;
        p = bd;
        for(i=0; i<8; i++) {
                ip = p+8;

        loop:
                switch(c = getchar()) {

                case 'K':
                        nkng -= 100;
                        c = 6;
                        bkp = p-bd;
                        break;

                case 'k':
                        nkng--;
                        c = -6;
                        wkp = p-bd;
                        break;

                case 'P':
                        c = 1;
                        break;

                case 'p':
                        c = -1;
                        break;

                case 'N':
                        c = 2;
                        break;

                case 'n':
                        c = -2;
                        break;

                case 'B':
                        c = 3;
                        break;

                case 'b':
                        c = -3;
                        break;

                case 'R':
                        c = 4;
                        break;

                case 'r':
                        c = -4;
                        break;

                case 'Q':
                        c = 5;
                        break;

                case 'q':
                        c = -5;
                        break;

                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                        p += c-'0';
                        goto loop;

                case ' ':
                        p++;
                        goto loop;

                case '\n':
                        if(p > ip)
                                err++;
                        p = ip;
                        continue;

                default:
                        err++;
                        if(c <= 0)
                                onhup();
                        goto loop;
                }
                if(p < ip)
                        *p++ = c;
                goto loop;
        }
        if(nkng)
                err++;
        if(err) {
                printf("Illegal setup\n");
                return;
        }
        for(i=0; i<64; i++)
                board[i] = bd[i];
        amp = ambuf+1;
        lmp = lmbuf+1;
        eppos = 64;
        bookp = 0;
        mantom = 0;
        moveno = 1;
        wkpos = wkp;
        bkpos = bkp;
        flag = 0;
        if(wkpos == 60) {
                if(board[56] == -4)
                        flag |= 2;
                if(board[63] == -4)
                        flag |= 1;
        }
        if(bkpos == 4) {
                if(board[0] == 4)
                        flag |= 020;
                if(board[7] == 4)
                        flag |= 010;
        }
        printf("Setup successful\n");
}

void stdbin(int* as, int* ar, int* af) {
        int c;

loop:
        c = *sbufp++;
        if(c == 'q') {
                *as = 0;
                goto kq;
        }
        if(c == 'k') {
                *as = 1;
        kq:
                stdbin(as, ar, af);
                if(*af < 0)
                        *af = 3;
                return;
        }

        if(c == 'r') {
                *af = 0;
                goto loop;
        }
        if(c == 'n') {
                *af = 1;
                goto loop;
        }
        if(c == 'b') {
                *af = 2;
                goto loop;
        }
        if(c>'0' && c<'9')
                *ar = c-'1'; else
                sbufp--;
}

bool comp(int p, int v) {
    if (p < 0) return 1;
    return (p == abs(v));
}

int pcomp(int p, int l, int pp, int sp, int rp, int fp) {
    int r, f, s;

    f = l%8;
    r = l/8;
    if(!mantom)
        r = 7-r;
    if(f > 3) {
        f = 7-f;
        s = 1;
    } else
        s = 0;

    if (comp(pp, p) && comp(sp, s) && comp(rp, r) && comp(fp, f)) {
        return(1);
    }
    return(0);
}

void stdpin(int* ap, int* as, int* ar, int* af) {
    int c;

    c = *sbufp++;
    if(c == 'q') {
        *as = 0;
        stdpin(ap, as, ar, af);
        return;
    }
    if(c == 'k') {
        *as = 1;
        stdpin(ap, as, ar, af);
        return;
    }
    if(c == 'p') {
        *ap = 1;
        if(*as >= 0)
            *af = 3;
        goto loc;
    }
    if(c == 'n') {
        *ap = 2;
        goto pie;
    }
    if(c == 'b') {
        *ap = 3;
        goto pie;
    }
    if(c == 'r') {
        *ap = 4;
        goto pie;
    }
    sbufp--;
    goto loc;

pie:
    if(*sbufp == 'p') {
        *af = (*ap-1)%3;
        *ap = 1;
        sbufp++;
    }

loc:
    if(*ap<0 && *as>=0) {
        *ap = *as+5;
        *as = -1;
    }
    if(*sbufp == '/') {
        sbufp++;
        stdbin(as, ar, af);
    }
}

int _stdin() {
        int piece1, piece2, side1, side2, rnk1, rnk2, file1, file2;
        int ckf, c, m, *p1, *p2, to, amb, piece;

        piece1 = piece2 = side1 = side2 = -1;
        rnk1 = rnk2 = file1 = file2  = -1;
        ckf = 0;
        if(match("o-o-o")||match("ooo")) {
                piece1 = 6;
                file1 = 3;
                side1 = 1;
                file2 = 2;
                side2 = 0;
                goto search;
        }
        if(match("o-o")||match("oo")) {
                piece1 = 6;
                file1 = 3;
                file2 = 1;
                goto search;
        }
        stdpin(&piece1, &side1, &rnk1, &file1);
        c = *sbufp++;
        if(c=='*' || c=='x')
                stdpin(&piece2, &side2, &rnk2, &file2); else
        if(c == '-')
                stdbin(&side2, &rnk2, &file2); else
                sbufp--;

search:
        c = *sbufp++;
        if(c == '+') {
                ckf = 1;
                c = *sbufp++;
        }
        if(c != '\0')
                return(0);

        p1 = p2 = lmp;
        mantom? bagen(): wagen();
        m = -1;
        amb = 0;
        while(p1 != lmp) {
                p1++;
                piece = board[*p1>>8];
                mantom? bmove(*p1): wmove(*p1);
                to = amp[-3];
                if(pcomp(piece, amp[-4],
                        piece1, side1, rnk1, file1))
                if(pcomp(amp[-2], to,
                        piece2, side2, rnk2, file2))
                if(comp(ckf, check())) {
                        if(m >= 0) {
                                if(!amb) {
                                        printf("ambiguous\n");
                                        amb = 1;
                                }
                        }
                        m = *p1;
                }
                p1++;
                mantom? bremove(): wremove();
        }
        lmp = p2;
        if(amb) return(-1);
        return(m);
}

int manual() {
    int a, b, c;
    char *p1;
    //extern out1;

    while (true) {
        intrp = 0;
        stage();
        rline();
        sbufp = sbuf;
        if (match("save")) {
                save();
                continue;
        }
        if (match("test")) {
                testf = !testf;
                continue;
        }
        if (match("remove")) {
                if(amp[-1] != -1) {
                        decrem();
                        mantom? bremove(): wremove();
                }
                if(amp[-1] != -1) {
                        decrem();
                        mantom? bremove(): wremove();
                }
                continue;
        }
        if (match("exit"))
                exit(0);
        if (match("manual")) {
                manflg = !manflg;
                continue;
        }
        if (match("resign"))
                onhup();
        if (moveno == 1 && mantom == 0) {
                if (match("first"))
                        play(1);
                if (match("alg")) {
                        mfmt = 1;
                        continue;
                }
                if (match("restore")) {
                        restore();
                        continue;
                }
        }
        if (match("clock")) {
                clktim[mantom] += clock();
                show_clock_time("white", clktim[0]);
                show_clock_time("black", clktim[1]);
                continue;
        }
        if (match("score")) {
                score();
                continue;
        }
        if (match("setup")) {
                setup();
                continue;
        }
        if (match("hint")) {
                a = xplay();
                out(abmove);
                printf(" %d\n", a);
                continue;
        }
        if (match("repeat")) {
                if(amp[-1] != -1) {
                        a = amp;
                        mantom? wremove(): bremove();
                        decrem();
                        posit(&out1, a, IGNORE);
                }
                continue;
        }
        if (*sbufp == '\0') {
                pboard();
                continue;
        }
        if ((a = algin()) != 0) {
                mfmt = 1;
                return(a);
        }
        if ((a = _stdin()) != 0) {
                mfmt = 0;
                return(a);
        }
        printf("eh?\n");
    }
}

void prtime(int a, int b) {
    printf("compute time is\n");
    putnumb(a);
    printf("real time is\n");
    putnumb(b);
    pause();
}

void term() {
    exit(0);
}

int main(void) {
    printf("Chess\n");
    itinit();
    lmp = lmbuf;
    amp = ambuf;
    *amp++ = -1;
    *lmp++ = -1;            /* fence */
    // TODO: read book
    //bookf = open("/usr/lib/book", 0);
    //if(bookf > 0)
    //      read(bookf, &bookp, 2);
    for (int i = 64; i--;) {
            dir[i] = (edge[i/8]<<6) | edge[i%8];
    }
    play(0);
}
