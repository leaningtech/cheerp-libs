
// Adapted from newlib/libc/machine/h8500/divsi3.c

#define divnorm(num, den, sign) 		\
{						\
  if (num < 0) 					\
    {						\
      num = -num;				\
      sign = 1;					\
    }						\
  else 						\
    {						\
      sign = 0;					\
    }						\
						\
  if (den < 0) 					\
    {						\
      den = - den;				\
      sign = 1 - sign;				\
    } 						\
}

unsigned long long
divmodti4(int modwanted, unsigned long long num, unsigned long long den)
{
  unsigned long long int bit = 1;
  long long int res = 0;
  long long prevden;
  while (den < num && bit && !(den & (1LL<<63)))
    {
      den = den << 1;
      bit = bit << 1;
    }
  while (bit)
    {
      if (num >= den)
	{
	  num -= den;
	  res |= bit;
	}
      bit = bit >> 1;
      den = den >> 1;
    }
  if (modwanted)
    return num;
  else
    return res;
}


#define exitdiv(sign, res) if (sign) { res = - res;} return res;

long long
__modti3 (long long numerator, long long denominator)
{
  int sign = 0;
  long long dividend;
  long long modul;


  if (numerator < 0)
    {
      numerator = -numerator;
      sign = 1;
    }
  if (denominator < 0)
    {
      denominator = -denominator;
    }

  modul = divmodti4 (1, numerator, denominator);
  if (sign)
    return - modul;
  return modul;
}


long long
__divti3 (long long numerator, long long denominator)
{
  int sign;
  long long dividend;
  long long modul;
  divnorm (numerator, denominator, sign);

  dividend = divmodti4 (0,  numerator, denominator);
  exitdiv (sign, dividend);
}

long long
__umodti3 (unsigned long long numerator, unsigned long long denominator)
{
  long long dividend;
  long long modul;

  modul = divmodti4 (1,  numerator, denominator);
  return modul;
}

long long
__udivti3 (unsigned long long numerator, unsigned long long denominator)
{
  int sign;
  long long dividend;
  long long modul;
  dividend = divmodti4 (0, numerator, denominator);
  return dividend;
}
