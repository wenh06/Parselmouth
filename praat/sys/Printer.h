#ifndef _Printer_h_
#define _Printer_h_
/* Printer.h
 *
 * Copyright (C) 1992-2011,2012,2015,2017 Paul Boersma
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

#include "Graphics.h"

/* When changing the following structure, update its initialization in Printer.cpp */
struct Printer {
	kGraphicsPostscript_spots spots;
	kGraphicsPostscript_paperSize paperSize;
	kGraphicsPostscript_orientation orientation;
	bool postScript, allowDirectPostScript;
	kGraphicsPostscript_fontChoiceStrategy fontChoiceStrategy;
	long resolution, paperWidth, paperHeight;
	double magnification;
};

extern struct Printer thePrinter;

#if defined (macintosh) || defined (_WIN32)
	// int Printer_postScript_printf (void *stream, const char *format, ... );
	int Printer_postScript_printf (FILE *stream, const char *format, fmt::printf_args);
#endif
#ifdef _WIN32
	HDC Printer_getDC ();
#endif
int Printer_pageSetup ();
int Printer_postScriptSettings ();
int Printer_print (void (*draw) (void *void_me, Graphics g), void *void_me);
void Printer_nextPage ();

void Printer_prefs ();

/* End of file Printer.h */
#endif
