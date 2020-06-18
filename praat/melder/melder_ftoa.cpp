/* melder_ftoa.cpp
 *
 * Copyright (C) 1992-2008,2010-2012,2014-2020 Paul Boersma
 *
 * This code is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This code is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this work. If not, see <http://www.gnu.org/licenses/>.
 */

#include "melder.h"

#include <iostream>
#include <locale>

namespace {

struct array_ostreambuf : public std::streambuf {
	array_ostreambuf(char *out, size_t n) {
		setp(out, out + n);
	}
};

bool sftoa_c(char *s, size_t n, double value, unsigned char precision, std::ios_base::fmtflags floatflags = std::ios_base::fmtflags(0)) {
	static auto cLocale = std::locale::classic();
	static auto &cNumput = std::use_facet<std::num_put<char>>(cLocale);
	static std::ios format(nullptr);  // Wake me up when Praat gets thread-safe. But I'd hope that that global array of buffers will also have changed by then.

	array_ostreambuf buffer(s, n);
	format.precision(precision);
	format.setf(floatflags, std::ios_base::floatfield);
	cNumput.put(&buffer, format, ' ', value);

	if (buffer.sputc('\0') == array_ostreambuf::traits_type::eof()) {
		s[n-1] = '\0';
		return false;
	}
	return true;
}

template <size_t N>
inline bool sftoa_c(char (&s)[N], double value, unsigned char precision, std::ios_base::fmtflags floatflags = std::ios_base::fmtflags(0)) {
	return sftoa_c(s, N, value, precision, floatflags);
}

}

/********** NUMBER TO STRING CONVERSION **********/

#define NUMBER_OF_BUFFERS  32
	/* = maximum number of arguments to a function call */
#define MAXIMUM_NUMERIC_STRING_LENGTH  800
	/* = sign + 324 + point + 60 + e + sign + 3 + null byte + ("·10^^" - "e"), times 2, + i, + 7 extra */

static char   buffers8  [NUMBER_OF_BUFFERS] [MAXIMUM_NUMERIC_STRING_LENGTH + 1];
static char32 buffers32 [NUMBER_OF_BUFFERS] [MAXIMUM_NUMERIC_STRING_LENGTH + 1];
static int ibuffer = 0;

#define CONVERT_BUFFER_TO_CHAR32 \
	char32 *q = buffers32 [ibuffer]; \
	while (*p != '\0') \
		* q ++ = (char32) (char8) * p ++; /* change sign before extending (should be unnecessary, because all characters should be below 128) */ \
	*q = U'\0'; \
	return buffers32 [ibuffer];

const char * Melder8_integer (int64 value) noexcept {
	if (++ ibuffer == NUMBER_OF_BUFFERS)
		ibuffer = 0;
	if (sizeof (long_not_integer) == 8) {
		int n = snprintf (buffers8 [ibuffer], MAXIMUM_NUMERIC_STRING_LENGTH + 1, "%ld", (long_not_integer) value);   // cast to identical type, to make compiler happy
		Melder_assert (n > 0);
		Melder_assert (n <= MAXIMUM_NUMERIC_STRING_LENGTH);
	} else if (sizeof (long long) == 8) {
		/*
		 * There are buggy platforms (namely 32-bit Mingw on Windows XP) that support long long and %lld but that convert
		 * the argument to a 32-bit long.
		 * There are also buggy platforms (namely 32-bit gcc on Linux) that support long long and %I64d but that convert
		 * the argument to a 32-bit long.
		 */
		static const char *formatString = nullptr;
		if (! formatString) {
			char tryBuffer [MAXIMUM_NUMERIC_STRING_LENGTH + 1];
			formatString = "%lld";
			sprintf (tryBuffer, formatString, 1000000000000LL);
			if (! strequ (tryBuffer, "1000000000000")) {
				formatString = "%I64d";
				sprintf (tryBuffer, formatString, 1000000000000LL);
				if (! strequ (tryBuffer, "1000000000000"))
					Melder_fatal (U"Found no way to print 64-bit integers on this machine.");
			}
		}
		int n = snprintf (buffers8 [ibuffer], MAXIMUM_NUMERIC_STRING_LENGTH + 1, formatString, value);
		Melder_assert (n > 0);
		Melder_assert (n <= MAXIMUM_NUMERIC_STRING_LENGTH);
	} else {
		Melder_fatal (U"Neither long nor long long is 8 bytes on this machine.");
	}
	return buffers8 [ibuffer];
}
conststring32 Melder_integer (int64 value) noexcept {
	const char *p = Melder8_integer (value);
	CONVERT_BUFFER_TO_CHAR32
}

const char * Melder8_bigInteger (int64 value) noexcept {
	if (++ ibuffer == NUMBER_OF_BUFFERS)
		ibuffer = 0;
	char *text = buffers8 [ibuffer];
	text [0] = '\0';
	if (value < 0) {
		sprintf (text, "-");
		value = - value;
	}
	int quintillions =  value / 1000000000000000000LL;
	value -=     quintillions * 1000000000000000000LL;
	int quadrillions =  value / 1000000000000000LL;
	value -=     quadrillions * 1000000000000000LL;
	int trillions =     value / 1000000000000LL;
	value -=        trillions * 1000000000000LL;
	int billions =      value / 1000000000LL;
	value -=         billions * 1000000000LL;
	int millions =      value / 1000000LL;
	value -=         millions * 1000000LL;
	int thousands =     value / 1000LL;
	value -=        thousands * 1000LL;
	int units = value;
	bool firstDigitPrinted = false;
	if (quintillions) {
		sprintf (text + strlen (text), firstDigitPrinted ? "%03d," : "%d,", quintillions);
		firstDigitPrinted = true;
	}
	if (quadrillions || firstDigitPrinted) {
		sprintf (text + strlen (text), firstDigitPrinted ? "%03d," : "%d,", quadrillions);
		firstDigitPrinted = true;
	}
	if (trillions || firstDigitPrinted) {
		sprintf (text + strlen (text), firstDigitPrinted ? "%03d," : "%d,", trillions);
		firstDigitPrinted = true;
	}
	if (billions || firstDigitPrinted) {
		sprintf (text + strlen (text), firstDigitPrinted ? "%03d," : "%d,", billions);
		firstDigitPrinted = true;
	}
	if (millions || firstDigitPrinted) {
		sprintf (text + strlen (text), firstDigitPrinted ? "%03d," : "%d,", millions);
		firstDigitPrinted = true;
	}
	if (thousands || firstDigitPrinted) {
		sprintf (text + strlen (text), firstDigitPrinted ? "%03d," : "%d,", thousands);
		firstDigitPrinted = true;
	}
	sprintf (text + strlen (text), firstDigitPrinted ? "%03d" : "%d", units);
	return text;
}
conststring32 Melder_bigInteger (int64 value) noexcept {
	const char *p = Melder8_bigInteger (value);
	CONVERT_BUFFER_TO_CHAR32
}

const char * Melder8_boolean (bool value) noexcept {
	return value ? "yes" : "no";
}
conststring32 Melder_boolean (bool value) noexcept {
	return value ? U"yes" : U"no";
}

const char * Melder8_onoff (bool value) noexcept {
	return value ? "on" : "off";
}
conststring32 Melder_onoff (bool value) noexcept {
	return value ? U"on" : U"off";
}

const char * Melder8_kleenean (kleenean valueK) noexcept {
	return valueK ? "yes" : ! valueK ? "no": "unknown";
}
conststring32 Melder_kleenean (kleenean valueK) noexcept {
	return valueK ? U"yes" : ! valueK ? U"no": U"unknown";
}

/*@praat
	assert string$ (1000000000000) = "1000000000000"
	assert string$ (undefined) = "--undefined--"
@*/
const char * Melder8_double (double value) noexcept {
	if (isundef (value))
		return "--undefined--";
	if (++ ibuffer == NUMBER_OF_BUFFERS)
		ibuffer = 0;
	sftoa_c(buffers8[ibuffer], value, 15); // sprintf (buffers8 [ibuffer], "%.15g", value);
	if (Melder8_strtod (buffers8 [ibuffer], nullptr) != value) {
		sftoa_c(buffers8[ibuffer], value, 16); // sprintf (buffers8 [ibuffer], "%.16g", value);
		if (Melder8_strtod (buffers8 [ibuffer], nullptr) != value)
			sftoa_c(buffers8[ibuffer], value, 17); // sprintf (buffers8 [ibuffer], "%.17g", value);
	}
	return buffers8 [ibuffer];
}
conststring32 Melder_double (double value) noexcept {
	const char *p = Melder8_double (value);
	CONVERT_BUFFER_TO_CHAR32
}
const char * Melder8_double (double value, integer precision, char format /*= 'g'*/) noexcept {
	if (isundef (value))
		return "--undefined--";
	if (++ ibuffer == NUMBER_OF_BUFFERS)
		ibuffer = 0;
	switch (format) {
		case 'e':
			sftoa_c(buffers8 [ibuffer], value, precision, std::ios::scientific);
			break;
		case 'f':
			sftoa_c(buffers8 [ibuffer], value, precision, std::ios::fixed);
			break;
		case 'g':
		default:
			Melder_assert(format == 'g');
			sftoa_c(buffers8 [ibuffer], value, precision);
			break;
	}
	return buffers8 [ibuffer];
}
conststring32 Melder_double (double value, integer precision, char format /*= 'g'*/) noexcept {
	const char *p = Melder8_double (value, precision, format);
	CONVERT_BUFFER_TO_CHAR32
}

const char * Melder8_single (double value) noexcept {
	if (isundef (value))
		return "--undefined--";
	if (++ ibuffer == NUMBER_OF_BUFFERS) ibuffer = 0;
	sftoa_c(buffers8 [ibuffer], value, 9); // sprintf (buffers8 [ibuffer], "%.9g", value);
	return buffers8 [ibuffer];
}
conststring32 Melder_single (double value) noexcept {
	const char *p = Melder8_single (value);
	CONVERT_BUFFER_TO_CHAR32
}

const char * Melder8_half (double value) noexcept {
	if (isundef (value))
		return "--undefined--";
	if (++ ibuffer == NUMBER_OF_BUFFERS)
		ibuffer = 0;
	sftoa_c(buffers8 [ibuffer], value, 4); // sprintf (buffers8 [ibuffer], "%.4g", value);
	return buffers8 [ibuffer];
}
conststring32 Melder_half (double value) noexcept {
	const char *p = Melder8_half (value);
	CONVERT_BUFFER_TO_CHAR32
}

const char * Melder8_fixed (double value, integer precision) noexcept {
	if (isundef (value))
		return "--undefined--";
	if (value == 0.0)
		return "0";
	if (++ ibuffer == NUMBER_OF_BUFFERS)
		ibuffer = 0;
	if (precision > 60)
		precision = 60;
	int minimumPrecision = - (int) floor (log10 (fabs (value)));

	auto res = sftoa_c(buffers8 [ibuffer], value, minimumPrecision > precision ? minimumPrecision : precision, std::ios_base::fixed);
	Melder_assert(res);
	// int n = snprintf (buffers8 [ibuffer], MAXIMUM_NUMERIC_STRING_LENGTH + 1, "%.*f",
	// 	(int) (value, minimumPrecision > precision ? minimumPrecision : precision), value);
	// Melder_assert (n > 0);
	// Melder_assert (n <= MAXIMUM_NUMERIC_STRING_LENGTH);
	return buffers8 [ibuffer];
}
conststring32 Melder_fixed (double value, integer precision) noexcept {
	const char *p = Melder8_fixed (value, precision);
	CONVERT_BUFFER_TO_CHAR32
}

const char * Melder8_fixedExponent (double value, integer exponent, integer precision) noexcept {
	double factor = pow (10.0, exponent);
	if (isundef (value))
		return "--undefined--";
	if (value == 0.0)
		return "0";
	if (++ ibuffer == NUMBER_OF_BUFFERS)
		ibuffer = 0;
	if (precision > 60)
		precision = 60;
	value /= factor;
	int minimumPrecision = - (int) floor (log10 (fabs (value)));

	auto res = sftoa_c(buffers8 [ibuffer], value, minimumPrecision > precision ? minimumPrecision : precision, std::ios_base::fixed);
	Melder_assert(res);
	auto partlen = strlen(buffers8 [ibuffer]);
	int n = partlen + snprintf (buffers8 [ibuffer] + partlen, MAXIMUM_NUMERIC_STRING_LENGTH + 1 - partlen, "E%d", (int) exponent);
	// int n = snprintf (buffers8 [ibuffer], MAXIMUM_NUMERIC_STRING_LENGTH + 1, "%.*fE%d",
	// 	(int) (minimumPrecision > precision ? minimumPrecision : precision), value, (int) exponent);
	Melder_assert (n > 0);
	Melder_assert (n <= MAXIMUM_NUMERIC_STRING_LENGTH + 1);
	return buffers8 [ibuffer];
}
conststring32 Melder_fixedExponent (double value, integer exponent, integer precision) noexcept {
	const char *p = Melder8_fixedExponent (value, exponent, precision);
	CONVERT_BUFFER_TO_CHAR32
}

const char * Melder8_percent (double value, integer precision) noexcept {
	if (isundef (value))
		return "--undefined--";
	if (value == 0.0)
		return "0";
	if (++ ibuffer == NUMBER_OF_BUFFERS)
		ibuffer = 0;
	if (precision > 60)
		precision = 60;
	value *= 100.0;
	int minimumPrecision = - (int) floor (log10 (fabs (value)));

	auto res = sftoa_c(buffers8 [ibuffer], value, minimumPrecision > precision ? minimumPrecision : precision, std::ios_base::fixed);
	Melder_assert(res);
	auto partlen = strlen(buffers8 [ibuffer]);
	Melder_assert (partlen < MAXIMUM_NUMERIC_STRING_LENGTH);
	if (partlen < MAXIMUM_NUMERIC_STRING_LENGTH) {
		buffers8[ibuffer][partlen] = '%';
		buffers8[ibuffer][partlen + 1] = '\0';
	}
	// int n = snprintf (buffers8 [ibuffer], MAXIMUM_NUMERIC_STRING_LENGTH + 1, "%.*f%%",
	// 	(int) (minimumPrecision > precision ? minimumPrecision : precision), value);
	// Melder_assert (n > 0);
	// Melder_assert (n <= MAXIMUM_NUMERIC_STRING_LENGTH);
	return buffers8 [ibuffer];
}
conststring32 Melder_percent (double value, integer precision) noexcept {
	const char *p = Melder8_percent (value, precision);
	CONVERT_BUFFER_TO_CHAR32
}

const char * Melder8_hexadecimal (integer value, integer precision) noexcept {
	if (value < 0)
		return "--undefined--";
	if (++ ibuffer == NUMBER_OF_BUFFERS)
		ibuffer = 0;
	if (precision > 60)
		precision = 60;
	integer integerValue = Melder_iround (value);
	int n = snprintf (buffers8 [ibuffer], MAXIMUM_NUMERIC_STRING_LENGTH + 1, "%.*llX",
		(int) precision, (unsigned long long) integerValue);
	Melder_assert (n > 0);
	Melder_assert (n <= MAXIMUM_NUMERIC_STRING_LENGTH);
	return buffers8 [ibuffer];
}
conststring32 Melder_hexadecimal (integer value, integer precision) noexcept {
	const char *p = Melder8_hexadecimal (value, precision);
	CONVERT_BUFFER_TO_CHAR32
}

const char * Melder8_dcomplex (dcomplex value) noexcept {
	if (isundef (value.real()) || isundef (value.imag()))
		return "--undefined--";
	if (++ ibuffer == NUMBER_OF_BUFFERS)
		ibuffer = 0;
	sftoa_c(buffers8 [ibuffer], value.real(), 15); // sprintf (buffers8 [ibuffer], "%.15g", value.real());
	if (Melder8_strtod (buffers8 [ibuffer], nullptr) != value.real()) {
		sftoa_c(buffers8 [ibuffer], value.real(), 16); // sprintf (buffers8 [ibuffer], "%.16g", value.real());
		if (Melder8_strtod (buffers8 [ibuffer], nullptr) != value.real())
			sftoa_c(buffers8 [ibuffer], value.real(), 17); // sprintf (buffers8 [ibuffer], "%.17g", value.real());
	}
	char *p = buffers8 [ibuffer] + strlen (buffers8 [ibuffer]);
	*p = ( value.imag() < 0.0 ? '-' : '+' );
	value. imag (fabs (value.imag()));
	++ p;
	auto s = MAXIMUM_NUMERIC_STRING_LENGTH + 1 - (p - buffers8 [ibuffer]);
	sftoa_c(p, s, value.imag(), 15); // sprintf (p, "%.15g", value.imag());
	if (Melder8_strtod (p, nullptr) != value.imag()) {
		sftoa_c(p, s, value.imag(), 16); // sprintf (p, "%.16g", value.imag());
		if (Melder8_strtod (p, nullptr) != value.imag())
			sftoa_c(p, s, value.imag(), 17); // sprintf (p, "%.17g", value.imag());
	}
	strcat (buffers8 [ibuffer], "i");
	return buffers8 [ibuffer];
}
conststring32 Melder_dcomplex (dcomplex value) noexcept {
	const char *p = Melder8_dcomplex (value);
	CONVERT_BUFFER_TO_CHAR32
}

const char * Melder8_scomplex (dcomplex value) noexcept {
	if (isundef (value.real()) || isundef (value.imag()))
		return "--undefined--";
	if (++ ibuffer == NUMBER_OF_BUFFERS)
		ibuffer = 0;
	sftoa_c(buffers8 [ibuffer], value.real(), 9); // sprintf (buffers8 [ibuffer], "%.9g", value.real());
	char *p = buffers8 [ibuffer] + strlen (buffers8 [ibuffer]);
	*p = ( value.imag() < 0.0 ? '-' : '+' );
	++p;
	auto s = MAXIMUM_NUMERIC_STRING_LENGTH + 1 - (p - buffers8 [ibuffer]);
	sftoa_c(p, s, fabs (value.imag()), 9); // sprintf (++ p, "%.9g", fabs (value.imag()));
	strcat (buffers8 [ibuffer], "i");
	return buffers8 [ibuffer];
}
conststring32 Melder_scomplex (dcomplex value) noexcept {
	const char *p = Melder8_scomplex (value);
	CONVERT_BUFFER_TO_CHAR32
}

conststring32 Melder_float (conststring32 number) noexcept {
	if (++ ibuffer == NUMBER_OF_BUFFERS)
		ibuffer = 0;
	if (! str32chr (number, 'e')) {
		str32cpy (buffers32 [ibuffer], number);
	} else {
		char32 *b = buffers32 [ibuffer];
		const char32 *n = & number [0];
		while (*n != U'e')
			*(b++) = *(n++);
		*b = U'\0';
		if (number [0] == '1' && number [1] == 'e') {
			str32cpy (buffers32 [ibuffer], U"10^^");
			b = buffers32 [ibuffer] + 4;
		} else {
			str32cat (buffers32 [ibuffer], U"·10^^");
			b += 5;
		}
		Melder_assert (*n == U'e');
		if (*++n == U'+')
			n ++;   // ignore leading plus sign in exponent
		if (*n == U'-')
			*(b++) = *(n++);   // copy sign of negative exponent
		while (*n == U'0')
			n ++;   // ignore leading zeroes in exponent
		while (*n >= U'0' && *n <= U'9')
			*(b++) = *(n++);
		*(b++) = U'^';
		while (*n != U'\0')
			*(b++) = *(n++);
		*b = U'\0';
	}
	return buffers32 [ibuffer];
}

const char * Melder8_naturalLogarithm (double lnNumber) noexcept {
	//if (lnNumber == -INFINITY) return "0";   // this would have been nice, but cannot be relied upon
	if (isundef (lnNumber))
		return "--undefined--";
	double log10Number = lnNumber * NUMlog10e;
	if (log10Number < -41.0) {
		if (++ ibuffer == NUMBER_OF_BUFFERS) ibuffer = 0;
		long_not_integer ceiling = (long_not_integer) ceil (log10Number);
		double remainder = log10Number - ceiling;
		double remainder10 = pow (10.0, remainder);
		while (remainder10 < 1.0) {
			remainder10 *= 10.0;
			ceiling --;
		}
		sftoa_c(buffers8 [ibuffer], remainder10, 15); // sprintf (buffers8 [ibuffer], "%.15g", remainder10);
		if (Melder8_strtod (buffers8 [ibuffer], nullptr) != remainder10) {
			sftoa_c(buffers8 [ibuffer], remainder10, 16); // sprintf (buffers8 [ibuffer], "%.16g", remainder10);
			if (Melder8_strtod (buffers8 [ibuffer], nullptr) != remainder10)
				sftoa_c(buffers8 [ibuffer], remainder10, 17); // sprintf (buffers8 [ibuffer], "%.17g", remainder10);
		}
		sprintf (buffers8 [ibuffer] + strlen (buffers8 [ibuffer]), "e-%ld", (long_not_integer) ceiling);
	} else {
		return Melder8_double (exp (lnNumber));
	}
	return buffers8 [ibuffer];
}
conststring32 Melder_naturalLogarithm (double lnNumber) noexcept {
	const char *p = Melder8_naturalLogarithm (lnNumber);
	CONVERT_BUFFER_TO_CHAR32
}

const char * Melder8_pointer (const void *pointer) noexcept {
	if (++ ibuffer == NUMBER_OF_BUFFERS) ibuffer = 0;
	sprintf (buffers8 [ibuffer], "%p", pointer);
	return buffers8 [ibuffer];
}
conststring32 Melder_pointer (const void *pointer) noexcept {
	const char *p = Melder8_pointer (pointer);
	CONVERT_BUFFER_TO_CHAR32
}

conststring32 Melder_character (char32 kar) noexcept {
	if (++ ibuffer == NUMBER_OF_BUFFERS) ibuffer = 0;
	buffers32 [ibuffer] [0] = kar;
	buffers32 [ibuffer] [1] = U'\0';
	return buffers32 [ibuffer];
}

const char * Melder8_colour (MelderColour colour) noexcept {
	if (isundef (colour.red) || isundef (colour.green) || isundef (colour.blue))
		return "{--undefined--,--undefined--,--undefined--}";
	if (++ ibuffer == NUMBER_OF_BUFFERS)
		ibuffer = 0;
	char *p = & buffers8 [ibuffer] [0];
	strcpy (p, "{");
	p ++;
	auto s = MAXIMUM_NUMERIC_STRING_LENGTH + 1 - (p - buffers8 [ibuffer]);
	sftoa_c(p, s, colour.red, 15); // sprintf (p, "%.15g", colour.red);
	if (Melder8_strtod (p, nullptr) != colour.red) {
		sftoa_c(p, s, colour.red, 16); // sprintf (p, "%.16g", colour.red);
		if (Melder8_strtod (p, nullptr) != colour.red)
			sftoa_c(p, s, colour.red, 17); // sprintf (p, "%.17g", colour.red);
	}
	p += strlen (p);
	strcpy (p, ",");
	p ++;
	s = MAXIMUM_NUMERIC_STRING_LENGTH + 1 - (p - buffers8 [ibuffer]);
	sftoa_c(p, s, colour.green, 15); // sprintf (p, "%.15g", colour.green);
	if (Melder8_strtod (p, nullptr) != colour.green) {
		sftoa_c(p, s, colour.green, 16); // sprintf (p, "%.16g", colour.green);
		if (Melder8_strtod (p, nullptr) != colour.green)
			sftoa_c(p, s, colour.green, 17); // sprintf (p, "%.17g", colour.green);
	}
	p += strlen (p);
	strcpy (p, ",");
	p ++;
	s = MAXIMUM_NUMERIC_STRING_LENGTH + 1 - (p - buffers8 [ibuffer]);
	sftoa_c(p, s, colour.blue, 15); // sprintf (p, "%.15g", colour.blue);
	if (Melder8_strtod (p, nullptr) != colour.blue) {
		sftoa_c(p, s, colour.blue, 16); // sprintf (p, "%.16g", colour.blue);
		if (Melder8_strtod (p, nullptr) != colour.blue)
			sftoa_c(p, s, colour.blue, 17); // sprintf (p, "%.17g", colour.blue);
	}
	p += strlen (p);
	strcpy (p, "}");
	return buffers8 [ibuffer];
}
conststring32 Melder_colour (MelderColour colour) noexcept {
	const char *p = Melder8_colour (colour);
	CONVERT_BUFFER_TO_CHAR32
}

/********** TENSOR TO STRING CONVERSION **********/

#define NUMBER_OF_TENSOR_BUFFERS  3
static MelderString theTensorBuffers [NUMBER_OF_TENSOR_BUFFERS];
static int iTensorBuffer { 0 };

conststring32 Melder_VEC (constVECVU const& value) {
	if (++ iTensorBuffer == NUMBER_OF_TENSOR_BUFFERS)
		iTensorBuffer = 0;
	MelderString *string = & theTensorBuffers [iTensorBuffer];
	MelderString_empty (string);
	if (! NUMisEmpty (value))
		for (integer i = 1; i <= value.size; i ++)
			MelderString_append (string, value [i], U'\n');
	return string -> string;
}
conststring32 Melder_MAT (constMATVU const& value) {
	if (++ iTensorBuffer == NUMBER_OF_TENSOR_BUFFERS)
		iTensorBuffer = 0;
	MelderString *string = & theTensorBuffers [iTensorBuffer];
	MelderString_empty (string);
	if (! NUMisEmpty (value)) {
		for (integer irow = 1; irow <= value.nrow; irow ++) {
			for (integer icol = 1; icol <= value.ncol; icol ++) {
				MelderString_append (string, value [irow] [icol]);
				if (icol < value.ncol)
					MelderString_appendCharacter (string, U' ');
			}
			if (irow < value.nrow)
				MelderString_appendCharacter (string, U'\n');
		}
	}
	return string -> string;
}

/********** STRING TO STRING CONVERSION **********/

static MelderString thePadBuffers [NUMBER_OF_BUFFERS];
static int iPadBuffer { 0 };

conststring32 Melder_pad (int64 width, conststring32 string) {
	if (++ iPadBuffer == NUMBER_OF_BUFFERS)
		iPadBuffer = 0;
	int64 length = str32len (string);
	int64 tooShort = width - length;
	if (tooShort <= 0)
		return string;
	MelderString_empty (& thePadBuffers [iPadBuffer]);
	for (int64 i = 0; i < tooShort; i ++)
		MelderString_appendCharacter (& thePadBuffers [iPadBuffer], U' ');
	MelderString_append (& thePadBuffers [iPadBuffer], string);
	return thePadBuffers [iPadBuffer]. string;
}

conststring32 Melder_pad (conststring32 string, int64 width) {
	if (++ iPadBuffer == NUMBER_OF_BUFFERS)
		iPadBuffer = 0;
	int64 length = str32len (string);
	int64 tooShort = width - length;
	if (tooShort <= 0)
		return string;
	MelderString_copy (& thePadBuffers [iPadBuffer], string);
	for (int64 i = 0; i < tooShort; i ++)
		MelderString_appendCharacter (& thePadBuffers [iPadBuffer], U' ');
	return thePadBuffers [iPadBuffer]. string;
}

conststring32 Melder_truncate (int64 width, conststring32 string) {
	if (++ iPadBuffer == NUMBER_OF_BUFFERS)
		iPadBuffer = 0;
	int64 length = str32len (string);
	int64 tooLong = length - width;
	if (tooLong <= 0)
		return string;
	MelderString_ncopy (& thePadBuffers [iPadBuffer], string + tooLong, width);
	return thePadBuffers [iPadBuffer]. string;
}

conststring32 Melder_truncate (conststring32 string, int64 width) {
	if (++ iPadBuffer == NUMBER_OF_BUFFERS)
		iPadBuffer = 0;
	int64 length = str32len (string);
	int64 tooLong = length - width;
	if (tooLong <= 0)
		return string;
	MelderString_ncopy (& thePadBuffers [iPadBuffer], string, width);
	return thePadBuffers [iPadBuffer]. string;
}

conststring32 Melder_padOrTruncate (int64 width, conststring32 string) {
	if (++ iPadBuffer == NUMBER_OF_BUFFERS)
		iPadBuffer = 0;
	int64 length = str32len (string);
	int64 tooLong = length - width;
	if (tooLong == 0)
		return string;
	if (tooLong < 0) {
		int64 tooShort = - tooLong;
		MelderString_empty (& thePadBuffers [iPadBuffer]);
		for (int64 i = 0; i < tooShort; i ++)
			MelderString_appendCharacter (& thePadBuffers [iPadBuffer], U' ');
		MelderString_append (& thePadBuffers [iPadBuffer], string);
	} else {
		MelderString_ncopy (& thePadBuffers [iPadBuffer], string + tooLong, width);
	}
	return thePadBuffers [iPadBuffer]. string;
}

conststring32 Melder_padOrTruncate (conststring32 string, int64 width) {
	if (++ iPadBuffer == NUMBER_OF_BUFFERS)
		iPadBuffer = 0;
	int64 length = str32len (string);
	int64 tooLong = length - width;
	if (tooLong == 0)
		return string;
	if (tooLong < 0) {
		int64 tooShort = - tooLong;
		MelderString_copy (& thePadBuffers [iPadBuffer], string);
		for (int64 i = 0; i < tooShort; i ++)
			MelderString_appendCharacter (& thePadBuffers [iPadBuffer], U' ');
	} else {
		MelderString_ncopy (& thePadBuffers [iPadBuffer], string, width);
	}
	return thePadBuffers [iPadBuffer]. string;
}

/* End of file melder_ftoa.cpp */
