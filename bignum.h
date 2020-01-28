
typedef unsigned long DIGIT_T;
typedef unsigned short int HALF_DIGIT_T;

/* Sizes to match */
#define MAX_DIGIT 0xffffffffUL
#define MAX_HALF_DIGIT 0xffffUL	/* NB 'L' */
#define BITS_PER_DIGIT 32
#define HIBITMASK 0x80000000UL
#define MAX_FIXED_DIGITS 0x40

#define BITS_PER_HALF_DIGIT (BITS_PER_DIGIT / 2)
#define BYTES_PER_DIGIT (BITS_PER_DIGIT / 8)

/* Useful macros */
#define LOHALF(x) ((DIGIT_T)((x) & MAX_HALF_DIGIT))
#define HIHALF(x) ((DIGIT_T)((x) >> BITS_PER_HALF_DIGIT & MAX_HALF_DIGIT))
#define TOHIGH(x) ((DIGIT_T)((x) << BITS_PER_HALF_DIGIT))

#define ISODD(x) ((x) & 0x1)
#define ISEVEN(x) (!ISODD(x))

DIGIT_T mpAdd(DIGIT_T w[], DIGIT_T u[], DIGIT_T v[], int ndigits);
DIGIT_T mpSubb(DIGIT_T w[], DIGIT_T u[], DIGIT_T v[], int ndigits);
int spMultiply(DIGIT_T p[2], DIGIT_T x, DIGIT_T y);
int mpMultiply(DIGIT_T w[], DIGIT_T u[], DIGIT_T v[], int ndigits);
unsigned long spDivide(unsigned long *pq, unsigned long *pr, unsigned long u[2], unsigned long v);
DIGIT_T mpShiftLeft(DIGIT_T a[], DIGIT_T *b, int shift, int ndigits);
DIGIT_T mpShiftRight(DIGIT_T a[], DIGIT_T b[], int shift, int ndigits);
int mpDivide(DIGIT_T q[], DIGIT_T r[], DIGIT_T u[], int udigits, DIGIT_T v[], int vdigits);
DIGIT_T mpSetZero(volatile DIGIT_T a[], int ndigits);
int mpSizeof(DIGIT_T a[], int ndigits);
DIGIT_T mpShortDiv(DIGIT_T q[], DIGIT_T u[], DIGIT_T v, int ndigits);
void mpSetEqual(DIGIT_T a[], DIGIT_T b[], int ndigits);
int mpCompare(DIGIT_T a[], DIGIT_T b[], int ndigits);
void mpSetDigit(DIGIT_T a[], DIGIT_T d, int ndigits);
DIGIT_T mpMultSub(DIGIT_T wn, DIGIT_T w[], DIGIT_T v[],DIGIT_T q, int n);
int QhatTooBig(DIGIT_T qhat, DIGIT_T rhat,DIGIT_T vn2, DIGIT_T ujn2);
int mpModulo(DIGIT_T r[], DIGIT_T u[], int udigits, DIGIT_T v[], int vdigits);
int mpShortCmp(const DIGIT_T a[], DIGIT_T d, int ndigits);
int mpCubeRoot(DIGIT_T s[], DIGIT_T n[], int ndigits);