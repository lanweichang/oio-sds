/*
 * Copyright (C) 2013 AtoS Worldline
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "metautils.h"
#include "expr.h"

void
expr_clean(struct expr_s *pE)
{
	if (!pE)
		return;

	CHK_TYPE(pE->type, return);

	switch (pE->type) {
	case VAL_STR_ET:
		if (pE->expr.str)
			free(pE->expr.str);
		break;
	case VAL_NUM_ET:
		break;
	case UN_NUMSUP_ET:
	case UN_NUMINF_ET:
	case UN_NUMNOT_ET:
	case UN_STRNUM_ET:
	case UN_STRLEN_ET:
		if (pE->expr.unary)
			expr_clean(pE->expr.unary);
		break;
	case BIN_STRCMP_ET:
	case BIN_NUMCMP_ET:
	case BIN_NUMEQ_ET:
	case BIN_NUMNEQ_ET:
	case BIN_NUMLT_ET:
	case BIN_NUMLE_ET:
	case BIN_NUMGT_ET:
	case BIN_NUMGE_ET:
	case BIN_NUMADD_ET:
	case BIN_NUMSUB_ET:
	case BIN_NUMMUL_ET:
	case BIN_NUMDIV_ET:
	case BIN_NUMMOD_ET:
	case BIN_NUMAND_ET:
	case BIN_ROOT_ET:
	case BIN_NUMXOR_ET:
	case BIN_NUMOR_ET:
		if (pE->expr.bin.p1)
			expr_clean(pE->expr.bin.p1);
		if (pE->expr.bin.p2)
			expr_clean(pE->expr.bin.p2);
		break;
	case ACC_ET:
		if (pE->expr.acc.base)
			free(pE->expr.acc.base);
		if (pE->expr.acc.field)
			free(pE->expr.acc.field);
		break;
	case NB_ET:
		break;
	}

	free(pE);
}
