#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <kwrap/stdio.h>

typedef long long int quad_t;
typedef long long unsigned u_quad_t;
//typedef unsigned short uintptr_t;
//typedef unsigned int uintptr_t; //modified by jeah

#define BUF     32  /* Maximum length of numeric string. */
#define strtoq  simple_strtol
#define strtouq simple_strtoll

/*
 * Flags used during conversion.
 */
#define LONG        0x01    /* l: long or double */
#define SHORT       0x04    /* h: short */
#define SUPPRESS    0x08    /* *: suppress assignment */
#define POINTER     0x10    /* p: void * (as hex) */
#define NOSKIP      0x20    /* [ or c: do not skip blanks */
#define LONGLONG    0x400   /* ll: long long (+ deprecated q: quad) */
#define SHORTSHORT  0x4000  /* hh: char */
#define UNSIGNED    0x8000  /* %[oupxX] conversions */

/*
 * The following are used in numeric conversions only:
 * SIGNOK, NDIGITS, DPTOK, and EXPOK are for floating point;
 * SIGNOK, NDIGITS, PFXOK, and NZDIGITS are for integral.
 */
#define SIGNOK      0x40    /* +/- is (still) legal */
#define NDIGITS     0x80    /* no digits detected */

#define DPTOK       0x100   /* (float) decimal point is still legal */
#define EXPOK       0x200   /* (float) exponent (e+3, etc) still legal */

#define PFXOK       0x100   /* 0x prefix is (still) legal */
#define NZDIGITS    0x200   /* no zero digits detected */

/*
 * Conversion types.
 */
#define CT_CHAR     0   /* %c conversion */
#define CT_CCL      1   /* %[...] conversion */
#define CT_STRING   2   /* %s conversion */
#define CT_INT      3   /* %[dioupxX] conversion */
#define CT_FLOAT    4   // %f conversion

static const unsigned char *__sccl(char *, const unsigned char *);

/*
 * Fill in the given table from the scanset at the given format
 * (just after `[').  Return a pointer to the character past the
 * closing `]'.  The table has a 1 wherever characters should be
 * considered part of the scanset.
 */
static const unsigned char *__sccl(char *tab, const unsigned char *fmt)
{
	int c, n, v;

	/* first `clear' the whole table */
	c = *fmt++;     /* first char hat => negated scanset */
	if (c == '^') {
		v = 1;      /* default => accept */
		c = *fmt++; /* get new first char */
	} else {
		v = 0;    /* default => reject */
	}

	/* XXX: Will not work if sizeof(tab*) > sizeof(char) */
	(void) memset(tab, v, 256);

	if (c == 0) {
		return (fmt - 1);    /* format ended before closing ] */
	}

	/*
	 * Now set the entries corresponding to the actual scanset
	 * to the opposite of the above.
	 *
	 * The first character may be ']' (or '-') without being special;
	 * the last character may be '-'.
	 */
	v = 1 - v;
	for (;;) {
		tab[c] = v;     /* take character c */
doswitch:
		n = *fmt++;     /* and examine the next */
		switch (n) {

		case 0:         /* format ended too soon */
			return (fmt - 1);

		case '-':
			/*
			 * A scanset of the form
			 *  [01+-]
			 * is defined as `the digit 0, the digit 1,
			 * the character +, the character -', but
			 * the effect of a scanset such as
			 *  [a-zA-Z0-9]
			 * is implementation defined.  The V7 Unix
			 * scanf treats `a-z' as `the letters a through
			 * z', but treats `a-a' as `the letter a, the
			 * character -, and the letter a'.
			 *
			 * For compatibility, the `-' is not considerd
			 * to define a range if the character following
			 * it is either a close bracket (required by ANSI)
			 * or is not numerically greater than the character
			 * we just stored in the table (c).
			 */
			n = *fmt;
			if (n == ']' || n < c) {
				c = '-';
				break;  /* resume the for(;;) */
			}
			fmt++;
			/* fill in the range */
			do {
				tab[++c] = v;
			} while (c < n);
			c = n;
			/*
			 * Alas, the V7 Unix scanf also treats formats
			 * such as [a-c-e] as `the letters a through e'.
			 * This too is permitted by the standard....
			 */
			goto doswitch;

		case ']':       /* end of scanset */
			return fmt;

		default:        /* just another character */
			c = n;
			break;
		}
	}
	/* NOTREACHED */
}

/*
    Read formatted data from string into variable argument list.

    Reads data from s and stores them according to parameter format into the locations
    pointed by the elements in the variable argument list identified by arg.

    Internally, the function retrieves arguments from the list identified by arg as if
    va_arg was used on it, and thus the state of arg is likely to be altered by the call.

    In any case, arg should have been initialized by va_start at some point before the call,
    and it is expected to be released by va_end at some point after the call.
*/
int vsscanf_s(const char *inp, char const *fmt0, va_list ap)
{
	int inr;
	const unsigned char *fmt = (const unsigned char *)fmt0;
	int c;              // character from format, or conversion
	size_t width;       // field width, or 0
	char *p;            // points into all kinds of strings
	int n;              // handy integer
	int flags;          // flags as defined above
	char *p0;           // saves original value of p when necessary
	int nassigned;      // number of fields assigned
	int nconversions;   // number of conversions
	int nread;          // number of characters consumed from fp
	int base;           // base argument to conversion function
	char ccltab[256];   // character class table for %[...]
	char buf[BUF];      // buffer for numeric conversions

	// `basefix' is used to avoid `if' tests in the integer scanner
	static short basefix[17] = { 10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };

	inr = strlen(inp);

	nassigned = 0;
	nconversions = 0;
	nread = 0;
	base = 0;

	for (;;) {
		c = *fmt++;

		if (c == 0) {
			return nassigned;
		}

		if (isspace(c)) {
			while (inr > 0 && isspace(*inp)) {
				nread++, inr--, inp++;
			}
			continue;
		}

		if (c != '%') {
			goto literal;
		}

		width = 0;
		flags = 0;
		/*
		 * switch on the format.  continue if done;
		 * break once format type is derived.
		 */
again:
		c = *fmt++;
		switch (c) {
		case '%':
		default:
literal:
			if (inr <= 0) {
				goto input_failure;
			}
			if (*inp != c) {
				goto match_failure;
			}
			inr--, inp++;
			nread++;
			continue;

		// The data is to be read from the stream but not stored in the location pointed by an argument.
		case '*':
			flags |= SUPPRESS;
			goto again;

		// Length modifier, long or long long.
		case 'l':
			if (flags & LONG) {
				flags &= ~LONG;
				flags |= LONGLONG;
			} else {
				flags |= LONG;
			}
			goto again;

		// ??
		case 'q':
			flags |= LONGLONG;  /* not quite */
			goto again;

		// Length modifier, short or char.
		case 'h':
			if (flags & SHORT) {
				flags &= ~SHORT;
				flags |= SHORTSHORT;
			} else {
				flags |= SHORT;
			}
			goto again;

		// Width (in characters)
		case '0' ... '9':
			width = width * 10 + c - '0';
			goto again;

		// Conversion specifier
		// Decimal integer, Any number of decimal digits (0-9), optionally preceded by a sign (+ or -).
		case 'd':
			c       = CT_INT;
			base    = 10;
			break;

		// Integer
		// Any number of digits, optionally preceded by a sign (+ or -). Decimal digits assumed by
		// default (0-9), but a 0 prefix introduces octal digits (0-7), and 0x hexadecimal digits (0-f).
		case 'i':
			c       = CT_INT;
			base    = 0;
			break;

		// OOctal integer
		// Any number of octal digits (0-7), optionally preceded by a sign (+ or -).
		case 'o':
			c       = CT_INT;
			flags  |= UNSIGNED;
			base    = 8;
			break;

		// Unsigned decimal integer.
		case 'u':
			c       = CT_INT;
			flags  |= UNSIGNED;
			base    = 10;
			break;

		// Hexadecimal integer
		// Any number of hexadecimal digits (0-9, a-f, A-F), optionally preceded by 0x or 0X,
		// and all optionally preceded by a sign (+ or -).
		case 'X':
		case 'x':
			flags  |= PFXOK; /* enable 0x prefixing */
			c       = CT_INT;
			flags  |= UNSIGNED;
			base    = 16;
			break;

		// String of characters
		// Any number of non-whitespace characters, stopping at the first whitespace character found.
		// A terminating null character is automatically added at the end of the stored sequence.
		case 's':
			c = CT_STRING;
			break;

		// Scanset
		// Any number of the characters specified between the brackets.
		case '[':
			fmt     = __sccl(ccltab, fmt);
			flags  |= NOSKIP;
			c       = CT_CCL;
			break;

		// Character
		// The next character. If a width other than 1 is specified, the function reads exactly width
		// characters and stores them in the successive locations of the array passed as argument. No
		// null character is appended at the end.
		case 'c':
			flags  |= NOSKIP;
			c       = CT_CHAR;
			break;

		// Pointer address
		// A sequence of characters representing a pointer. The particular format used depends on the
		// system and library implementation, but it is the same as the one used to format %p in fprintf.
		case 'p':   /* pointer format is like hex */
			flags  |= (POINTER | PFXOK | UNSIGNED);
			c       = CT_INT;
			base    = 16;
			break;

		// Floating point
		case 'f':
			c       = CT_FLOAT;
			pr_err("sscanf_s not support float.\n");
			goto input_failure;
		//break;

		// Count
		// No input is consumed. The number of characters written so far is stored in the pointed location.
		case 'n':
			nconversions++;
			if (flags & SUPPRESS) {
				continue;
			}

			if (flags & SHORTSHORT) {
				*va_arg(ap, char *) = nread;
			} else if (flags & SHORT) {
				*va_arg(ap, short *) = nread;
			} else if (flags & LONG) {
				*va_arg(ap, long *) = nread;
			} else if (flags & LONGLONG) {
				*va_arg(ap, long long *) = nread;
			} else {
				*va_arg(ap, int *) = nread;
			}
			continue;
		}

		// We have a conversion that requires input.
		if (inr <= 0) {
			goto input_failure;
		}

		// Consume leading white space, except for formats that suppress this.
		if ((flags & NOSKIP) == 0) {
			while (isspace(*inp)) {
				nread++;
				if (--inr > 0) {
					inp++;
				} else {
					goto input_failure;
				}
			}
			// Note that there is at least one character in the buffer, so conversions
			// that do not set NOSKIP can no longer result in an input failure.
		}

		// Do the coversion
		switch (c) {
		// Character
		default :
//        case CT_CHAR:
			if (width == 0) {
				width = 1;
			}

			if (flags & SUPPRESS) {
				size_t sum = 0;

				n = inr;
				if (n < (int)width) {
					sum += n;
					width -= n;
					inp += n;
					if (sum == 0) {
						goto input_failure;
					}
				} else {
					sum += width;
					inr -= width;
					inp += width;
				}

				nread += sum;
			} else {
				char *c_ptr;
				size_t size;

				c_ptr = va_arg(ap, char *);
				size = va_arg(ap, size_t);
				if (width > size) {
					goto input_failure;
				}
				memcpy((void *)c_ptr, (const void *)inp, width);
				inr -= width;
				inp += width;
				nread += width;
				nassigned++;
			}
			nconversions++;
			break;

		// Scanset
		case CT_CCL:
			if (width == 0) {
				width = (size_t)~0;
			}

			if (flags & SUPPRESS) {
				n = 0;
				while (ccltab[(unsigned char)*inp]) {
					n++, inr--, inp++;
					if (--width == 0) {
						break;
					}
					if (inr <= 0) {
						if (n == 0) {
							goto input_failure;
						}
						break;
					}
				}

				if (n == 0) {
					goto match_failure;
				}
			} else {
				p0 = p = va_arg(ap, char *);
				while (ccltab[(unsigned char)*inp]) {
					inr--;
					*p++ = *inp++;

					if (--width == 0) {
						break;
					}

					if (inr <= 0) {
						if (p == p0) {
							goto input_failure;
						}
						break;
					}
				}
				n = p - p0;
				if (n == 0) {
					goto match_failure;
				}
				*p = 0;
				nassigned++;
			}
			nread += n;
			nconversions++;
			break;

		// String
		case CT_STRING:
			if (width == 0) {
				width = (size_t)~0;
			}

			if (flags & SUPPRESS) {
				n = 0;
				while (!isspace(*inp)) {
					n++, inr--, inp++;
					if (--width == 0) {
						break;
					}
					if (inr <= 0) {
						break;
					}
				}
				nread += n;
			} else {
				size_t size, buf_size;

				p0 = p = va_arg(ap, char *);
				buf_size = va_arg(ap, size_t);
				size = 0;
				while (!isspace(*inp)) {
					inr--;
					size++;
					if (size > buf_size) {
						goto input_failure;
					}
					*p++ = *inp++;
					if (--width == 0) {
						break;
					}
					if (inr <= 0) {
						break;
					}
				}
				size++;
				if (size > buf_size) {
					goto input_failure;
				}
				*p = 0;
				nread += p - p0;
				nassigned++;
			}
			nconversions++;
			break;

		// Simple version float point, only support [-]ddd.ddd type and doesn't support lenth "L" specifier (long double).
		case CT_FLOAT: {
				double  res;
				long    significand, fraction, divider;
				long    negative, period;

				// Width is infinite
				if (width == 0) {
					width = (size_t)~0;
				}

				negative    = 0;
				period      = 0;
				significand = 0;
				fraction    = 0;
				divider     = 1;

				// Sign character
				switch (*inp) {
				case '-':
					negative = 1;
				// fall through
				case '+':
					inr--;
					inp++;
					nread++;
					width--;
					break;

				default:
					break;
				}

				while (width) {
					c = *inp;

					inr--;
					inp++;
					nread++;
					width--;

					switch (c) {
					case '0' ... '9':
						// Fraction part
						if (period) {
							fraction = fraction * 10 + (c - '0');
							divider *= 10;
						}
						// Significand part
						else {
							significand = significand * 10 + (c - '0');
						}
						continue;

					case '.':
						if (period == 0) {
							period = 1;
						} else {
							break;
						}
						continue;

					default:
						break;
					}

					// Not 0 ~ 9 or .
					inr++;
					inp--;
					nread--;
					width++;
					break;
				}

				res = significand + (double)fraction / divider;
				if (negative == 1) {
					res = -res;
				}

				// Store data
				if ((flags & SUPPRESS) == 0) {
#if 1
					// double type
					if (flags & LONG) {
						*va_arg(ap, double *) = res;
					}
					// float type
					else {
						*va_arg(ap, float *) = (float)res;
					}
#endif
					nassigned++;
				}

				nconversions++;
			}
			break;

		case CT_INT:
#if 1
			if (width == 0 || width > sizeof(buf) - 1) {
				width = sizeof(buf) - 1;
			}
#else
			/* size_t is unsigned, hence this optimisation */
			if (--width > sizeof(buf) - 2) {
				width = sizeof(buf) - 2;
			}
			width++;
#endif
			flags |= SIGNOK | NDIGITS | NZDIGITS;

			for (p = buf; width; width--) {
				c = *inp;
				/*
				 * Switch on the character; `goto ok'
				 * if we accept it as a part of number.
				 */
				switch (c) {

				/*
				 * The digit 0 is always legal, but is
				 * special.  For %i conversions, if no
				 * digits (zero or nonzero) have been
				 * scanned (only signs), we will have
				 * base==0.  In that case, we should set
				 * it to 8 and enable 0x prefixing.
				 * Also, if we have not scanned zero digits
				 * before this, do not turn off prefixing
				 * (someone else will turn it off if we
				 * have scanned any nonzero digits).
				 */
				case '0':
					if (base == 0) {
						base = 8;
						flags |= PFXOK;
					}
					if (flags & NZDIGITS) {
						flags &= ~(SIGNOK | NZDIGITS | NDIGITS);
					} else {
						flags &= ~(SIGNOK | PFXOK | NDIGITS);
					}
					goto ok;

				/* 1 through 7 always legal */
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
					base = basefix[base];
					flags &= ~(SIGNOK | PFXOK | NDIGITS);
					goto ok;

				/* digits 8 and 9 ok iff decimal or hex */
				case '8':
				case '9':
					base = basefix[base];
					if (base <= 8) {
						break;    /* not legal here */
					}
					flags &= ~(SIGNOK | PFXOK | NDIGITS);
					goto ok;

				/* letters ok iff hex */
				case 'A':
				case 'B':
				case 'C':
				case 'D':
				case 'E':
				case 'F':
				case 'a':
				case 'b':
				case 'c':
				case 'd':
				case 'e':
				case 'f':
					/* no need to fix base here */
					if (base <= 10) {
						break;    /* not legal here */
					}
					flags &= ~(SIGNOK | PFXOK | NDIGITS);
					goto ok;

				/* sign ok only as first character */
				case '+':
				case '-':
					if (flags & SIGNOK) {
						flags &= ~SIGNOK;
						goto ok;
					}
					break;

				/* x ok iff flag still set & 2nd char */
				case 'x':
				case 'X':
					if (flags & PFXOK && p == buf + 1) {
						base = 16;  /* if %i */
						flags &= ~PFXOK;
						goto ok;
					}
					break;
				}

				/*
				 * If we got here, c is not a legal character
				 * for a number.  Stop accumulating digits.
				 */
				break;
ok:
				/*
				 * c is legal: store it and look at the next.
				 */
				*p++ = c;
				if (--inr > 0) {
					inp++;
				} else {
					break;    /* end of input */
				}
			}
			/*
			 * If we had only a sign, it is no good; push
			 * back the sign.  If the number ends in `x',
			 * it was [sign] '0' 'x', so push back the x
			 * and treat it as [sign] '0'.
			 */
			if (flags & NDIGITS) {
				if (p > buf) {
					inp--;
					inr++;
				}
				goto match_failure;
			}
			c = ((unsigned char *)p)[-1];
			if (c == 'x' || c == 'X') {
				--p;
				inp--;
				inr++;
			}
			if ((flags & SUPPRESS) == 0) {
				u_quad_t res;

				*p = 0;
				res = strtouq(buf, (char **)NULL, base);
				if (flags & POINTER)
					*va_arg(ap, void **) =
						(void *)(uintptr_t)res;
				else if (flags & SHORTSHORT) {
					*va_arg(ap, char *) = res;
				} else if (flags & SHORT) {
					*va_arg(ap, short *) = res;
				} else if (flags & LONG) {
					*va_arg(ap, long *) = res;
				} else if (flags & LONGLONG) {
					*va_arg(ap, long long *) = res;
				} else {
					*va_arg(ap, int *) = res;
				}
				nassigned++;
			}
			nread += p - buf;
			nconversions++;
			break;
		}
	}

input_failure:
	return (nconversions != 0 ? nassigned : -1);

match_failure:
	return nassigned;
}

/*
    Read formatted data from string.

    Reads data from "ibuf" and stores them according to parameter format into the locations
    given by the additional arguments.

    The additional arguments should point to already allocated objects of the type specified
    by their corresponding format specifier within the format string.
*/
int sscanf_s(const char *ibuf, const char *fmt, ...)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	ret = vsscanf_s(ibuf, fmt, ap);
	va_end(ap);
	return ret;
}


/*-----------------------------------------------------------------------------*/
/* Kernel Mode Definiton                                                       */
/*-----------------------------------------------------------------------------*/
EXPORT_SYMBOL(vsscanf_s);
EXPORT_SYMBOL(sscanf_s);