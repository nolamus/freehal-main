/*
 * This file is part of FreeHAL 2010.
 *
 * Copyright(c) 2006, 2007, 2008, 2009, 2010 Tobias Schulz and contributors.
 * http://freehal.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#define NOT_HEADER_ONLY 1

#include "hal2009.h"


void execute_perl6(char* filename) {
    fprintf(output(), "Perl6 (Parrot) is not compiled into this FreeHAL. Sorry.\n");
}

int convert_to_perl6_convert_file(char* filename) {
    fprintf(output(), "Perl6 (Parrot) is not compiled into this FreeHAL. Sorry.\n");
    return 0;
}