#include "user.h"
#include "types.h"

#define PRECISION 2
#define NUMLEN 16

#define strlen strlen__

float
fabs(const float f)
{
	return f < 0 ? -1 * f : f;
}

int
iabs(const int n)
{
	return n < 0 ? -1 * n : n;
}

int
pow(int n, int p)
{
	int result;

	if(n == 0)
	{
		return 0;
	}

	result = 1;
	while(p)
	{
		result *= n;
		p -= 1;
	}

	return result;
}

int
strlen(const char *str)
{
	int l;

	if(!str)
	{
		return 0;
	}

	l = 0;
	while(str[l] != '\0')
	{
		++l;
	}

	return l;
}

int
isdigit(const char c)
{
	switch(c)
	{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			return 0;
		default:
			return -1;
	}
	return -1;
}

int
strtoint(const char *str, int *n)
{
	int res = 0;
	int sign = 1;
	int i = 0;

	switch(str[i])
	{
		case '+':
			sign *= +1;
			i += 1;
			break;
		case '-':
			sign *= -1;
			i += 1;
			break;
	}

	while (str[i] != '\0')
	{
		if(isdigit(str[i]) != 0)
		{
			printf(1, "strtoint(): Please input integers only\n");
			return -1;
		}
		res = res * 10 + (str[i] - '0');
		i++;
	}

	*n = res * sign;
	return 0;
}

int
inttostr(const int num, char *str)
{
	int i = 0, start = 0;
	int n = iabs(num);
	int l, r, tmp;

	if(n == 0)
	{
		str[0] = '0';
		return 0;
	}

	if(num < 0)
	{
		str[i] = '-';
		i += 1;
		start = i;

	}

	while(n)
	{
		str[i] = '0' + n % 10;
		i += 1;
		n /= 10;
	}

	l = start, r = i - 1;
	while(l < r)
	{
		tmp = str[l];
		str[l] = str[r];
		str[r] = tmp;

		l += 1;
		r -= 1;
	}

	return 0;
}

int 
floattostr(const float num, char *str)
{
	float n = fabs(num);
	int i_n = (int) n;
	int f_n = (n - i_n) * pow(10,PRECISION);
	int i = 0;

	if(num < 0)
	{
		str[i] = '-';
		i += 1;
	}

	if(inttostr(i_n, str + i) != 0)
	{
		printf(1, "floattostr(): int to str failed\n");
		return -1;
	}

	i = strlen(str);
	str[i] = '.';
	i += 1;

	if(inttostr(f_n, str + i) != 0)
	{
		printf(1, "floattostr(): int to str failed\n");
		return -1;
	}

	return 0;
}

int
devide(const char *s_numerator, const char *s_denominator)
{
	int i_numerator;
	int i_denominator;
	char s_result[NUMLEN];
	int result;
	int i, l;

	if(strtoint(s_numerator, &i_numerator) != 0)
	{
		printf(1, "devide(): str to int failed\n");
		return -1;
	}

	if(strtoint(s_denominator, &i_denominator) != 0)
	{
		printf(1, "devide(): str to int failed\n");
		return -1;
	}

	if(i_denominator == 0)
	{
		printf(1, "devide(): Devison by zero is not permitted\n");
		return -1;
	}

	result =   (i_numerator * pow(10,PRECISION))  / i_denominator;

	for(i = 0; i < NUMLEN; ++i)
	{
		s_result[i] = '\0';
	}

	if(inttostr(result, s_result) != 0)
	{
		printf(1, "devide(): int to str failed\n");
		return -1;
	}

	l = strlen(s_result);
	for(i = 0; i < PRECISION; ++i)
	{
		s_result[l-i] = s_result[l-i-1];
	}
	s_result[l-PRECISION] = '.';

	printf(1, "%s/%s = %s\n", s_numerator, s_denominator, s_result); 
	return 0;
}

int main(int argc, char * argv[])
{
	if(argc != 3)
	{
		printf(1, "%s <numenator_integer> <denominator_integer>", argv[0]);
		exit();
	}
	devide(argv[1], argv[2]);
	exit();
}
