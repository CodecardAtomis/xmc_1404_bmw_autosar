#include "bignum.h"

__root DIGIT_T mpAdd(DIGIT_T w[], DIGIT_T u[], DIGIT_T v[], int ndigits)
{
	/*	Calculates w = u + v
		where w, u, v are multiprecision integers of ndigits each
		Returns carry if overflow. Carry = 0 or 1.

		Ref: Knuth Vol 2 Ch 4.3.1 p 266 Algorithm A.
	*/

	DIGIT_T k;
	int j;

	/* Step A1. Initialise */
	k = 0;

	for (j = 0; j < ndigits; j++)
	{
		/*	Step A2. Add digits w_j = (u_j + v_j + k)
			Set k = 1 if carry (overflow) occurs
		*/
		w[j] = u[j] + k;
		if (w[j] < k)
			k = 1;
		else
			k = 0;
		
		w[j] += v[j];
		if (w[j] < v[j])
			k++;

	}	/* Step A3. Loop on j */

	return k;	/* w_n = k */
}


__root DIGIT_T mpSubb(DIGIT_T w[], DIGIT_T u[], DIGIT_T v[], int ndigits)
{
	/*	Calculates w = u - v where u >= v
		w, u, v are multiprecision integers of ndigits each
		Returns 0 if OK, or 1 if v > u.

		Ref: Knuth Vol 2 Ch 4.3.1 p 267 Algorithm S.
	*/

	DIGIT_T k;
	int j;

	/* Step S1. Initialise */
	k = 0;

	for (j = 0; j < ndigits; j++)
	{
		/*	Step S2. Subtract digits w_j = (u_j - v_j - k)
			Set k = 1 if borrow occurs.
		*/
		w[j] = u[j] - k;
		if (w[j] > MAX_DIGIT - k)
			k = 1;
		else
			k = 0;
		
		w[j] -= v[j];
		if (w[j] > MAX_DIGIT - v[j])
			k++;

	}	/* Step S3. Loop on j */

	return k;	/* Should be zero if u >= v */
}

__root int spMultiply(DIGIT_T p[2], DIGIT_T x, DIGIT_T y)
{
	/* Use a 64-bit temp for product */
	unsigned long long t = (unsigned long long)x * (unsigned long long)y;
	/* then split into two parts */
	p[1] = (unsigned long)(t >> 32);
	p[0] = (unsigned long)(t & 0xFFFFFFFF);

	return 0;
}

__root int mpMultiply(DIGIT_T w[], DIGIT_T u[], DIGIT_T v[], int ndigits)
{
	/*	Computes product w = u * v
		where u, v are multiprecision integers of ndigits each
		and w is a multiprecision integer of 2*ndigits

		Ref: Knuth Vol 2 Ch 4.3.1 p 268 Algorithm M.
	*/

	DIGIT_T k, t[2];
	int i, j, m, n;

	m = n = ndigits;

	/* Step M1. Initialise */
	for (i = 0; i < 2 * m; i++)
		w[i] = 0;

	for (j = 0; j < n; j++)
	{
		/* Step M2. Zero multiplier? */
		if (v[j] == 0)
		{
			w[j + m] = 0;
		}
		else
		{
			/* Step M3. Initialise i */
			k = 0;
			for (i = 0; i < m; i++)
			{
				/* Step M4. Multiply and add */
				/* t = u_i * v_j + w_(i+j) + k */
				spMultiply(t, u[i], v[j]);

				t[0] += k;
				if (t[0] < k)
					t[1]++;
				t[0] += w[i+j];
				if (t[0] < w[i+j])
					t[1]++;

				w[i+j] = t[0];
				k = t[1];
			}	
			/* Step M5. Loop on i, set w_(j+m) = k */
			w[j+m] = k;
		}
	}	/* Step M6. Loop on j */

	return 0;
}

/**********************/
/* BIT-WISE FUNCTIONS */
/**********************/
__root DIGIT_T mpShiftLeft(DIGIT_T a[], DIGIT_T *b, int shift, int ndigits)
{	/* Computes a = b << shift */
	/* [v2.1] Modified to cope with shift > BITS_PERDIGIT */
	int i, y, nw, bits;
	DIGIT_T mask, carry, nextcarry;

	/* Do we shift whole digits? */
	if (shift >= BITS_PER_DIGIT)
	{
		nw = shift / BITS_PER_DIGIT;
		i = ndigits;
		while (i--)
		{
			if (i >= nw)
				a[i] = b[i-nw];
			else
				a[i] = 0;
		}
		/* Call again to shift bits inside digits */
		bits = shift % BITS_PER_DIGIT;
		carry = b[ndigits-nw] << bits;
		if (bits)
			carry |= mpShiftLeft(a, a, bits, ndigits);
		return carry;
	}
	else
	{
		bits = shift;
	}

	/* Construct mask = high bits set */
	mask = ~(~(DIGIT_T)0 >> bits);
	
	y = BITS_PER_DIGIT - bits;
	carry = 0;
	for (i = 0; i < ndigits; i++)
	{
		nextcarry = (b[i] & mask) >> y;
		a[i] = b[i] << bits | carry;
		carry = nextcarry;
	}

	return carry;
}

__root DIGIT_T mpShiftRight(DIGIT_T a[], DIGIT_T b[], int shift, int ndigits)
{	/* Computes a = b >> shift */
	/* [v2.1] Modified to cope with shift > BITS_PERDIGIT */
	int i, y, nw, bits;
	DIGIT_T mask, carry, nextcarry;

	/* Do we shift whole digits? */
	if (shift >= BITS_PER_DIGIT)
	{
		nw = shift / BITS_PER_DIGIT;
		for (i = 0; i < ndigits; i++)
		{
			if ((i+nw) < ndigits)
				a[i] = b[i+nw];
			else
				a[i] = 0;
		}
		/* Call again to shift bits inside digits */
		bits = shift % BITS_PER_DIGIT;
		carry = b[nw-1] >> bits;
		if (bits)
			carry |= mpShiftRight(a, a, bits, ndigits);
		return carry;
	}
	else
	{
		bits = shift;
	}

	/* Construct mask to set low bits */
	/* (thanks to Jesse Chisholm for suggesting this improved technique) */
	mask = ~(~(DIGIT_T)0 << bits);
	
	y = BITS_PER_DIGIT - bits;
	carry = 0;
	i = ndigits;
	while (i--)
	{
		nextcarry = (b[i] & mask) << y;
		a[i] = b[i] >> bits | carry;
		carry = nextcarry;
	}

	return carry;
}

__root DIGIT_T mpSetZero(volatile DIGIT_T a[], int ndigits)
{	/* Sets a = 0 */

	/* Prevent optimiser ignoring this */
	volatile DIGIT_T optdummy;
	volatile DIGIT_T *p = a;

	while (ndigits--)
		a[ndigits] = 0;
	
	optdummy = *p;
	return optdummy;
}

__root int mpSizeof(DIGIT_T a[], int ndigits)
{	/* Returns size of significant digits in a */
	
	while(ndigits--)
	{
		if (a[ndigits] != 0)
			return (++ndigits);
	}
	return 0;
}

__root unsigned long spDivide(unsigned long *pq, unsigned long *pr, unsigned long u[2], unsigned long v)
{
	unsigned long long uu, q;
	uu = (unsigned long long)u[1] << 32 | (unsigned long long)u[0];
	q = uu / (unsigned long long)v;
	//r = uu % (uint64_t)v;
	*pr = (unsigned long)(uu - q * v);
	*pq = (unsigned long)(q & 0xFFFFFFFF);
	return (unsigned long)(q >> 32);
}

__root int mpDivide(DIGIT_T q[], DIGIT_T r[], DIGIT_T u[], int udigits, DIGIT_T v[], int vdigits)
{	/*	Computes quotient q = u / v and remainder r = u mod v
		where q, r, u are multiple precision digits
		all of udigits and the divisor v is vdigits.

		Ref: Knuth Vol 2 Ch 4.3.1 p 272 Algorithm D.

		Do without extra storage space, i.e. use r[] for
		normalised u[], unnormalise v[] at end, and cope with
		extra digit Uj+n added to u after normalisation.

		WARNING: this trashes q and r first, so cannot do
		u = u / v or v = u mod v.
		It also changes v temporarily so cannot make it const.
	*/
	int shift;
	int n, m, j;
	DIGIT_T bitmask, overflow;
	DIGIT_T qhat, rhat, t[2];
	DIGIT_T *uu, *ww;
	int qhatOK, cmp;

	/* Clear q and r */
	mpSetZero(q, udigits);
	mpSetZero(r, udigits);

	/* Work out exact sizes of u and v */
	n = (int)mpSizeof(v, vdigits);
	m = (int)mpSizeof(u, udigits);
	m -= n;

	/* Catch special cases */
	if (n == 0)
		return -1;	/* Error: divide by zero */

	if (n == 1)
	{	/* Use short division instead */
		r[0] = mpShortDiv(q, u, v[0], udigits);
		return 0;
	}

	if (m < 0)
	{	/* v > u, so just set q = 0 and r = u */
		mpSetEqual(r, u, udigits);
		return 0;
	}

	if (m == 0)
	{	/* u and v are the same length */
		cmp = mpCompare(u, v, (int)n);
		if (cmp < 0)
		{	/* v > u, as above */
			mpSetEqual(r, u, udigits);
			return 0;
		}
		else if (cmp == 0)
		{	/* v == u, so set q = 1 and r = 0 */
			mpSetDigit(q, 1, udigits);
			return 0;
		}
	}

	/*	In Knuth notation, we have:
		Given
		u = (Um+n-1 ... U1U0)
		v = (Vn-1 ... V1V0)
		Compute
		q = u/v = (QmQm-1 ... Q0)
		r = u mod v = (Rn-1 ... R1R0)
	*/

	/*	Step D1. Normalise */
	/*	Requires high bit of Vn-1
		to be set, so find most signif. bit then shift left,
		i.e. d = 2^shift, u' = u * d, v' = v * d.
	*/
	bitmask = HIBITMASK;
	for (shift = 0; shift < BITS_PER_DIGIT; shift++)
	{
		if (v[n-1] & bitmask)
			break;
		bitmask >>= 1;
	}

	/* Normalise v in situ - NB only shift non-zero digits */
	overflow = mpShiftLeft(v, v, shift, n);

	/* Copy normalised dividend u*d into r */
	overflow = mpShiftLeft(r, u, shift, n + m);
	uu = r;	/* Use ptr to keep notation constant */

	t[0] = overflow;	/* Extra digit Um+n */

	/* Step D2. Initialise j. Set j = m */
	for (j = m; j >= 0; j--)
	{
		/* Step D3. Set Qhat = [(b.Uj+n + Uj+n-1)/Vn-1]
		   and Rhat = remainder */
		qhatOK = 0;
		t[1] = t[0];	/* This is Uj+n */
		t[0] = uu[j+n-1];
		overflow = spDivide(&qhat, &rhat, t, v[n-1]);

		/* Test Qhat */
		if (overflow)
		{	/* Qhat == b so set Qhat = b - 1 */
			qhat = MAX_DIGIT;
			rhat = uu[j+n-1];
			rhat += v[n-1];
			if (rhat < v[n-1])	/* Rhat >= b, so no re-test */
				qhatOK = 1;
		}
		/* [VERSION 2: Added extra test "qhat && "] */
		if (qhat && !qhatOK && QhatTooBig(qhat, rhat, v[n-2], uu[j+n-2]))
		{	/* If Qhat.Vn-2 > b.Rhat + Uj+n-2
			   decrease Qhat by one, increase Rhat by Vn-1
			*/
			qhat--;
			rhat += v[n-1];
			/* Repeat this test if Rhat < b */
			if (!(rhat < v[n-1]))
				if (QhatTooBig(qhat, rhat, v[n-2], uu[j+n-2]))
					qhat--;
		}


		/* Step D4. Multiply and subtract */
		ww = &uu[j];
		overflow = mpMultSub(t[1], ww, v, qhat, (int)n);

		/* Step D5. Test remainder. Set Qj = Qhat */
		q[j] = qhat;
		if (overflow)
		{	/* Step D6. Add back if D4 was negative */
			q[j]--;
			overflow = mpAdd(ww, ww, v, (int)n);
		}

		t[0] = uu[j+n-1];	/* Uj+n on next round */

	}	/* Step D7. Loop on j */

	/* Clear high digits in uu */
	for (j = n; j < m+n; j++)
		uu[j] = 0;

	/* Step D8. Unnormalise. */

	mpShiftRight(r, r, shift, n);
	mpShiftRight(v, v, shift, n);

	return 0;
}

__root int QhatTooBig(DIGIT_T qhat, DIGIT_T rhat,DIGIT_T vn2, DIGIT_T ujn2)
{	/*	Returns true if Qhat is too big
		i.e. if (Qhat * Vn-2) > (b.Rhat + Uj+n-2)
	*/
	DIGIT_T t[2];

	spMultiply(t, qhat, vn2);
	if (t[1] < rhat)
		return 0;
	else if (t[1] > rhat)
		return 1;
	else if (t[0] > ujn2)
		return 1;

	return 0;
}

__root DIGIT_T mpMultSub(DIGIT_T wn, DIGIT_T w[], DIGIT_T v[],DIGIT_T q, int n)
{	/*	Compute w = w - qv
		where w = (WnW[n-1]...W[0])
		return modified Wn.
	*/
	DIGIT_T k, t[2];
	int i;

	if (q == 0)	/* No change */
		return wn;

	k = 0;

	for (i = 0; i < n; i++)
	{
		spMultiply(t, q, v[i]);
		w[i] -= k;
		if (w[i] > MAX_DIGIT - k)
			k = 1;
		else
			k = 0;
		w[i] -= t[0];
		if (w[i] > MAX_DIGIT - t[0])
			k++;
		k += t[1];
	}

	/* Cope with Wn not stored in array w[0..n-1] */
	wn -= k;

	return wn;
}

__root void mpSetDigit(DIGIT_T a[], DIGIT_T d, int ndigits)
{	/* Sets a = d where d is a single digit */
	int i;
	
	for (i = 1; i < ndigits; i++)
	{
		a[i] = 0;
	}
	a[0] = d;
}

__root int mpCompare(DIGIT_T a[], DIGIT_T b[], int ndigits)
{
	/*	Returns sign of (a - b)
	*/

	if (ndigits == 0) return 0;

	while (ndigits--)
	{
		if (a[ndigits] > b[ndigits])
			return 1;	/* GT */
		if (a[ndigits] < b[ndigits])
			return -1;	/* LT */
	}

	return 0;	/* EQ */
}

__root void mpSetEqual(DIGIT_T a[], DIGIT_T b[], int ndigits)
{	/* Sets a = b */
	int i;
	
	for (i = 0; i < ndigits; i++)
	{
		a[i] = b[i];
	}
}

__root DIGIT_T mpShortDiv(DIGIT_T q[], DIGIT_T u[], DIGIT_T v, int ndigits)
{
	/*	Calculates quotient q = u div v
		Returns remainder r = u mod v
		where q, u are multiprecision integers of ndigits each
		and r, v are single precision digits.

		Makes no assumptions about normalisation.
		
		Ref: Knuth Vol 2 Ch 4.3.1 Exercise 16 p625
	*/
	int j;
	DIGIT_T t[2], r;
	int shift;
	DIGIT_T bitmask, overflow, *uu;

	if (ndigits == 0) return 0;
	if (v == 0)	return 0;	/* Divide by zero error */

	/*	Normalise first */
	/*	Requires high bit of V
		to be set, so find most signif. bit then shift left,
		i.e. d = 2^shift, u' = u * d, v' = v * d.
	*/
	bitmask = HIBITMASK;
	for (shift = 0; shift < BITS_PER_DIGIT; shift++)
	{
		if (v & bitmask)
			break;
		bitmask >>= 1;
	}

	v <<= shift;
	overflow = mpShiftLeft(q, u, shift, ndigits);
	uu = q;
	
	/* Step S1 - modified for extra digit. */
	r = overflow;	/* New digit Un */
	j = ndigits;
	while (j--)
	{
		/* Step S2. */
		t[1] = r;
		t[0] = uu[j];
		overflow = spDivide(&q[j], &r, t, v);
	}

	/* Unnormalise */
	r >>= shift;
	
	return r;
}

/***************************/
/* NUMBER THEORY FUNCTIONS */
/***************************/
__root int mpModulo(DIGIT_T r[], DIGIT_T u[], int udigits, DIGIT_T v[], int vdigits)
{
	/*	Computes r = u mod v
		where r, v are multiprecision integers of length vdigits
		and u is a multiprecision integer of length udigits.
		r may overlap v.

		Note that r here is only vdigits long, 
		whereas in mpDivide it is udigits long.

		Use remainder from mpDivide function.
	*/

	int nn = udigits;

        if (vdigits > udigits)
            nn = vdigits;
        
	DIGIT_T qq[MAX_FIXED_DIGITS];
	DIGIT_T rr[MAX_FIXED_DIGITS];

        mpSetZero(qq,nn);
        mpSetZero(rr,nn);

	/* rr[nn] = u mod v */
	mpDivide(qq, rr, u, udigits, v, vdigits);

	/* Final r is only vdigits long */
	mpSetEqual(r, rr, vdigits);

	return 0;
}

__root int mpShortCmp(const DIGIT_T a[], DIGIT_T d, int ndigits)
{
	/* Returns sign of (a - d) where d is a single digit */

	int i;

	/* Changed in [v2.0] to cope with zero-length a */
	if (ndigits == 0) return (d ? -1 : 0);

	for (i = 1; i < ndigits; i++)
	{
		if (a[i] != 0)
			return 1;	/* GT */
	}

	if (a[0] < d)
		return -1;		/* LT */
	else if (a[0] > d)
		return 1;		/* GT */

	return 0;	/* EQ */
}

__root int mpCubeRoot(DIGIT_T s[], DIGIT_T n[], int ndigits)
	/* Computes integer cube root s = floor(cuberoot(n)) i.e. 
	the largest integer whose cube is less than or equal to n */
	/* [Added v2.3] */
{
        int cmp_ret;
        
	DIGIT_T x[MAX_FIXED_DIGITS];
	DIGIT_T y[MAX_FIXED_DIGITS];
	DIGIT_T q[MAX_FIXED_DIGITS];
	DIGIT_T r[MAX_FIXED_DIGITS];


	/* if (n <= 1) return n */
	if (mpShortCmp(n, 1, ndigits) <= 0)
	{
		mpSetEqual(s, n, ndigits);
		goto done;
	}

	/* 1. [Initialize] Set x = n */
	mpSetEqual(x, n, ndigits);

	while (1)
	{
		/* 2. [Newtonian step] Set y = [2x + [n/x^2]]/3 */
		mpDivide(y, r, n, ndigits, x, ndigits);
		mpDivide(q, r, y, ndigits, x, ndigits);
		mpAdd(y, q, x, ndigits);
		mpAdd(q, y, x, ndigits);
		mpShortDiv(y, q, 3, ndigits);

		/* 3a. [Finished?] If y < x set x = y and go to step 2 */
                cmp_ret = mpCompare(y, x, ndigits);
		if (cmp_ret >= 0)
			break;

		mpSetEqual(x, y, ndigits);
	}

	/* 3b. Otherwise output x and stop. */
	mpSetEqual(s, x, ndigits);

done:

	return 0;
}

__root unsigned long icbrt1(unsigned long x) {
   int s = 30;
   unsigned long y, b;

   y = 0;
   for (s = 30; s >= 0; s = s - 3)
   {
      y <<= 1;
      b = ((3 * y) * (y + 1) + 1) << s;
      if (x >= b) {
         x = x - b;
         y = y + 1;
      }

   }
   return y;
}