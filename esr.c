#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned long u64;

static u64 _esr;
struct bitfield {
	char *name;
	char *long_name;
	size_t start;
	size_t width;
	u64 value;
	char *desc;
	char *indent;
	struct bitfield *next;
	struct bitfield *prev;
};

typedef void (*describe_fn)(struct bitfield *);

static u64 get_bits(u64 reg, size_t start, size_t end)
{
	u64 width = end - start + 1;
	return (reg >> start) & ((1 << width) - 1);
}

static void bitfield_new(u64 reg, char *name, char *long_name, size_t start,
			 size_t end, describe_fn desc, struct bitfield *field)
{
	memset(field, 0, sizeof(struct bitfield));
	field->name = name;
	field->long_name = long_name;
	field->start = start;
	field->width = end - start + 1;
	field->value = get_bits(reg, start, end);
	field->desc = NULL;
	field->indent = "";
	field->next = field;
	field->prev = field;

	if (desc != NULL) {
		desc(field);
	}
}

static void field_append(struct bitfield *head, struct bitfield *new)
{
	new->prev = head->prev;
	new->next = head;
	head->prev->next = new;
	head->prev = new;
}

static void decimal_to_binary(u64 n, size_t width, char *buf)
{
	int i = width - 1;
	int j = 0;

	if (n == 0) {
		for (; j < width; j++) {
			buf[j] = '0';
		}
		buf[j] = '\0';
		return;
	}

	for (; i >= 0; i--) {
		int k = (n >> i);
		buf[j++] = (k & 1) ? '1' : '0';
	}
	buf[j] = '\0';
}

static void field_description(struct bitfield *field)
{
	char binary[128];

	if (field->width == 1) {
		printf("%02ld\t", field->start);
		printf("%s:\t%s", field->name,
		       field->value == 1 ? "true" : "false");
	} else {
		printf("%02ld...%02ld\t", field->start,
		       field->start + field->width - 1);
		decimal_to_binary(field->value, field->width, binary);
		printf("%s:\t0x%02lx 0b%s", field->name, field->value, binary);
	}

	if (field->long_name) {
		printf(" (%s)", field->long_name);
	}

	if (field->desc) {
		printf("\t# %s", field->desc);
	}
	printf("\n");
}

static void bitfield_print(struct bitfield *head)
{
	field_description(head);
	for (struct bitfield *p = head->next; p != head; p = p->next) {
		p->indent = "\t";
		field_description(p);
	}
}

static u64 bitfield_describe(char *name, char *long_name, size_t start,
			     size_t end, describe_fn desc)
{
	struct bitfield field;
	bitfield_new(_esr, name, long_name, start, end, desc, &field);
	bitfield_print(&field);
	if (desc) {
		desc(&field);
	}
	return field.value;
}

static void check_res0(struct bitfield *res0)
{
	if (res0->value != 0) {
		res0->desc = "[ERROR]: Invalid RES0";
	} else {
		res0->desc = "Reserved";
	}
}

static void describe_res0(size_t start, size_t end)
{
	bitfield_describe("RES0", "Reserved", start, end, check_res0);
}

static void describe_il(struct bitfield *field)
{
	if (field->value == 1) {
		field->desc = "32-bit instruction trapped";
	} else {
		field->desc = "16-bit instruction trapped";
	}
}

static void describe_sas(struct bitfield *sas)
{
	switch (sas->value) {
	case 0b00:
		sas->desc = "byte accessed";
		break;
	case 0b01:
		sas->desc = "half word accessed";
		break;
	case 0b10:
		sas->desc = "word accessed";
		break;
	case 0b11:
		sas->desc = "double world accessed";
		break;
	default:
		break;
	}
}

static void describe_ar(struct bitfield *ar)
{
	if (ar->value == 1) {
		ar->desc = "Acquire/Release semantics";
	} else {
		ar->desc = "No Acquire/Release semantics";
	}
}

static void describe_fnv(struct bitfield *fnv)
{
	if (fnv->value == 1) {
		fnv->desc = "FAR is not valid, it holds an unknown value";
	} else {
		fnv->desc = "FAR is valid";
	}
}

static void describe_wnr(struct bitfield *wnr)
{
	if (wnr->value == 1) {
		wnr->desc = "Abort caused by writing to memory";
	} else {
		wnr->desc = "Abort caused by reading from memory";
	}
}

static void describe_fsc(struct bitfield *fsc)
{
	switch (fsc->value) {
	case 0b000000:
		fsc->desc =
			"Address size fault, level 0 of translation or translation table base register.";
		break;
	case 0b000001:
		fsc->desc = "Address size fault, level 1.";
		break;
	case 0b000010:
		fsc->desc = "Address size fault, level 2.";
		break;
	case 0b000011:
		fsc->desc = "Address size fault, level 3.";
		break;
	case 0b000100:
		fsc->desc = "Translation fault, level 0.";
		break;
	case 0b000101:
		fsc->desc = "Translation fault, level 1.";
		break;
	case 0b000110:
		fsc->desc = "Translation fault, level 2.";
		break;
	case 0b000111:
		fsc->desc = "Translation fault, level 3.";
		break;
	case 0b001001:
		fsc->desc = "Access flag fault, level 1.";
		break;
	case 0b001010:
		fsc->desc = "Access flag fault, level 2.";
		break;
	case 0b001011:
		fsc->desc = "Access flag fault, level 3.";
		break;
	case 0b001000:
		fsc->desc = "Access flag fault, level 0.";
		break;
	case 0b001100:
		fsc->desc = "Permission fault, level 0.";
		break;
	case 0b001101:
		fsc->desc = "Permission fault, level 1.";
		break;
	case 0b001110:
		fsc->desc = "Permission fault, level 2.";
		break;
	case 0b001111:
		fsc->desc = "Permission fault, level 3.";
		break;
	case 0b010000:
		fsc->desc =
			"Synchronous External abort, not on translation table walk or hardware update of translation table.";
		break;
	case 0b010001:
		fsc->desc = "Synchronous Tag Check Fault.";
		break;
	case 0b010011:
		fsc->desc =
			"Synchronous External abort on translation table walk or hardware update of translation table, level -1.";
		break;
	case 0b010100:
		fsc->desc =
			"Synchronous External abort on translation table walk or hardware update of translation table, level 0.";
		break;
	case 0b010101:
		fsc->desc =
			"Synchronous External abort on translation table walk or hardware update of translation table, level 1.";
		break;
	case 0b010110:
		fsc->desc =
			"Synchronous External abort on translation table walk or hardware update of translation table, level 2.";
		break;
	case 0b010111:
		fsc->desc =
			"Synchronous External abort on translation table walk or hardware update of translation table, level 3.";
		break;
	case 0b011000:
		fsc->desc =
			"Synchronous parity or ECC error on memory access, not on translation table walk.";
		break;
	case 0b011011:
		fsc->desc =
			"Synchronous parity or ECC error on memory access on translation table walk or hardware update of translation table, level -1.";
		break;
	case 0b011100:
		fsc->desc =
			"Synchronous parity or ECC error on memory access on translation table walk or hardware update of translation table, level 0.";
		break;
	case 0b011101:
		fsc->desc =
			"Synchronous parity or ECC error on memory access on translation table walk or hardware update of translation table, level 1.";
		break;
	case 0b011110:
		fsc->desc =
			"Synchronous parity or ECC error on memory access on translation table walk or hardware update of translation table, level 2.";
		break;

	case 0b011111:
		fsc->desc =
			"Synchronous parity or ECC error on memory access on translation table walk or hardware update of translation table, level 3.";
		break;

	case 0b100001:
		fsc->desc = "Alignment fault.";
		break;
	case 0b101001:
		fsc->desc = "Address size fault, level -1.";
		break;
	case 0b101011:
		fsc->desc = "Translation fault, level -1.";
		break;
	case 0b110000:
		fsc->desc = "TLB conflict abort.";
		break;
	case 0b110001:
		fsc->desc = "Unsupported atomic hardware update fault.";
		break;
	case 0b110100:
		fsc->desc = "IMPLEMENTATION DEFINED fault (Lockdown).";
		break;
	case 0b110101:
		fsc->desc =
			"IMPLEMENTATION DEFINED fault (Unsupported Exclusive or Atomic access).";
		break;
	default:
		fsc->desc = "[ERROR]: INVALID fsc!";
	}
}

void describe_set(struct bitfield *set)
{
	switch (set->value) {
	case 0b00:
		set->desc = "Recoverable state (UER)";
		break;
	case 0b10:
		set->desc = "Uncontainable (UC)";
		break;
	case 0b11:
		set->desc = "Restartable state (UEO)";
		break;
	default:
		set->desc = "[ERROR]: INVALID set!";
	}
}

static void decribe_cv(struct bitfield *cv)
{
	if (cv->value == 1) {
		cv->desc = "COND is valid";
	} else {
		cv->desc = "COND is not valid";
	}
}

static void describe_rv(struct bitfield *rv)
{
	if (rv->value == 1) {
		rv->desc = "Register number is valid";
	} else {
		rv->desc = "Register number is not valid";
	}
}

static void describe_ti(struct bitfield *ti)
{
	switch (ti->value) {
	case 0b00:
		ti->desc = "WFI trapped";
		break;
	case 0b01:
		ti->desc = "WFE trapped";
		break;
	case 0b10:
		ti->desc = "WFIT trapped";
		break;
	case 0b11:
		ti->desc = "WFET trapped";
		break;
	}
}

static void describe_cv(struct bitfield *cv)
{
	if (cv->value == 1) {
		cv->desc = "COND is valid";
	} else {
		cv->desc = "COND is not valid";
	}
}

static void describe_offset(struct bitfield *offset)
{
	if (offset->value) {
		offset->desc = "Add offset";
	} else {
		offset->desc = "Subtract offset";
	}
}

static void describe_mcr_direction(struct bitfield *direction)
{
	if (direction->value == 1) {
		direction->desc = "Read from system register (MRC or VMRS)";
	} else {
		direction->desc = "Write to system reigster (MCR)";
	}
}

static void describe_ldc_direction(struct bitfield *direction)
{
	if (direction->value == 1) {
		direction->desc = "Read from memory (LDC)";
	} else {
		direction->desc = "Write to memory (STC)";
	}
}

static void describe_msr_direction(struct bitfield *direction)
{
	if (direction->value == 1) {
		direction->desc = "Read from system register (MRS)";
	} else {
		direction->desc = "Write to system register (MSR)";
	}
}

static void describe_am(struct bitfield *am)
{
	switch (am->value) {
	case 0b000:
		am->desc = "Immediate unindexed";
		break;
	case 0b001:
		am->desc = "Immediate post-indexed";
		break;
	case 0b010:
		am->desc = "Immediate offset";
		break;
	case 0b011:
		am->desc = "Immediate pre-indexed";
		break;
	case 0b100:
		am->desc = "Reserved for trapped STR or T32 LDC";
		break;
	case 0b110:
		am->desc = "Reserved for trapped STC";
		break;
	default:
		am->desc = "[ERROR]: bad am";
		break;
	}
}

static void describe_iss_ld64b(struct bitfield *iss)
{
	switch (iss->value) {
	case 0b00:
		iss->desc = "ST64BV trapped";
		break;
	case 0b01:
		iss->desc = "ST64BV0 trapped";
		break;
	case 0b10:
		iss->desc = "LD64B or ST64B trapped";
		break;
	default:
		iss->desc = "[ERROR]: bad iss";
	}
}

static void describe_iord(struct bitfield *iord)
{
	if (iord->value == 1) {
		iord->desc = "Data Key";
	} else {
		iord->desc = "Instruction Key";
	}
}

static void describe_aorb(struct bitfield *aorb)
{
	if (aorb->value == 1) {
		aorb->desc = "B Key";
	} else {
		aorb->desc = "A Key";
	}
}

static void describe_smtc(struct bitfield *smtc)
{
	switch (smtc->value) {
	case 0b000:
		smtc->desc =
			"Access to SME functionality trapped as a result of CPACR_EL1.SMEN, CPTR_EL2.SMEN, CPTR_EL2.TSM, or CPTR_EL3.ESM";
		break;
	case 0b001:
		smtc->desc =
			"Advanced SIMD, SVE, or SVE2 instruction trapped because PSTATE.SM is 1";
		break;
	case 0b010:
		smtc->desc = "SME instruction trapped because PSTATE.SM is 0";
		break;
	case 0b011:
		smtc->desc = "SME instruction trapped because PSTATE.ZA is 0";
		break;
	default:
		smtc->desc = "[ERROR]: bad smtc";
	}
}

static void describe_s2ptw(struct bitfield *s2ptw)
{
	if (s2ptw->value == 1) {
		s2ptw->desc = "Fault on a stage 2 translation table walk";
	} else {
		s2ptw->desc = "Fault not on a stage 2 translation table walk";
	}
}

static void describe_ind(struct bitfield *ind)
{
	if (ind->value == 1) {
		ind->desc = "Instruction access";
	} else {
		ind->desc = "Data access";
	}
}

static void describe_gpcsc(struct bitfield *gpcsc)
{
	switch (gpcsc->value) {
	case 0b000000:
		gpcsc->desc = "GPT address size fault at level 0";
		break;
	case 0b000100:
		gpcsc->desc = "GPT walk fault at level 0";
		break;
	case 0b000101:
		gpcsc->desc = "GPT walk fault at level 1";
		break;
	case 0b001100:
		gpcsc->desc = "Granule protection fault at level 0";
		break;
	case 0b001101:
		gpcsc->desc = "Granule protection fault at level 1";
		break;
	case 0b010100:
		gpcsc->desc =
			"Synchronous External abort on GPT fetch at level 0";
		break;
	case 0b010101:
		gpcsc->desc =
			"Synchronous External abort on GPT fetch at level 1";
		break;
	default:
		gpcsc->desc = "[ERROR]: bad gpcsc";
	}
}

static void describe_vncr(struct bitfield *vncr)
{
	if (vncr->value == 1) {
		vncr->desc =
			"The fault was generated by the use of VNCR_EL2, by an MRS or MSR instruction executed at EL1";
	} else {
		vncr->desc =
			"The fault was not generated by the use of VNCR_EL2, by an MRS or MSR instruction executed at EL1";
	}
}

static void describe_cm(struct bitfield *cm)
{
	if (cm->value == 1) {
		cm->desc =
			"The Data Abort was generated by either the execution of a cache maintenance instruction or by a synchronous fault on the execution of an address translation instruction. The DC ZVA, DC GVA, and DC GZVA instructions are not classified as cache maintenance instructions, and therefore their execution cannot cause this field to be set to 1.";
	} else {
		cm->desc =
			"The Data Abort was not generated by the execution of one of the System instructions identified in the description of value 1";
	}
}

static void describe_s1ptw(struct bitfield *s1ptw)
{
	if (s1ptw->value == 1) {
		s1ptw->desc =
			"Fault on the stage 2 translation of an access for a stage 1 translation table walk";
	} else {
		s1ptw->desc =
			"Fault not on a stage 2 translation for a stage 1 translation table walk";
	}
}

static void describe_gpc_wnr(struct bitfield *wnr)
{
	if (wnr->value == 1) {
		wnr->desc =
			"Abort caused by an instruction writing to a memory location";
	} else {
		wnr->desc =
			"Abort caused by an instruction reading from a memory location";
	}
}

static void describe_xfsc(struct bitfield *xfsc)
{
	switch (xfsc->value) {
	case 0b100011:
		xfsc->desc =
			"Granule Protection Fault on translation table walk or hardware update of translation table, level -1";
		break;
	case 0b100100:
		xfsc->desc =
			"Granule Protection Fault on translation table walk or hardware update of translation table, level 0";
		break;
	case 0b100101:
		xfsc->desc =
			"Granule Protection Fault on translation table walk or hardware update of translation table, level 1";
		break;
	case 0b100110:
		xfsc->desc =
			"Granule Protection Fault on translation table walk or hardware update of translation table, level 2";
		break;
	case 0b100111:
		xfsc->desc =
			"Granule Protection Fault on translation table walk or hardware update of translation table, level 3";
		break;
	case 0b101000:
		xfsc->desc =
			"Granule Protection Fault, not on translation table walk or hardware update of translation table";
		break;
	default:
		xfsc->desc = "[ERROR]: bad xFSC";
	}
}

static void describe_tfv(struct bitfield *tfv)
{
	if (tfv->value == 1) {
		tfv->desc =
			"One or more floating-point exceptions occurred during an operation performed while executing the reported instruction. The IDF, IXF, UFF, OFF, DZF, and IOF bits indicate trapped floating-point exceptions that occurred";
	} else {
		tfv->desc =
			"The IDF, IXF, UFF, OFF, DZF, and IOF bits do not hold valid information about trapped floating-point exceptions and are UNKNOWN";
	}
}

static void describe_idf(struct bitfield *idf)
{
	if (idf->value == 1) {
		idf->desc = "Input denormal floating-point exception occurred.";
	} else {
		idf->desc =
			"Input denormal floating-point exception did not occur.";
	}
}

static void describe_ixf(struct bitfield *ixf)
{
	if (ixf->value == 1) {
		ixf->desc = "Inexact floating-point exception occurred.";
	} else {
		ixf->desc = "Inexact floating-point exception did not occur.";
	}
}

static void describe_uff(struct bitfield *uff)
{
	if (uff->value == 1) {
		uff->desc = "Underflow floating-point exception occurred.";
	} else {
		uff->desc = "Underflow floating-point exception did not occur.";
	}
}

static void describe_off(struct bitfield *off)
{
	if (off->value == 1) {
		off->desc = "Overflow floating-point exception occurred.";
	} else {
		off->desc = "Overflow floating-point exception did not occur.";
	}
}

static void describe_dzf(struct bitfield *dzf)
{
	if (dzf->value == 1) {
		dzf->desc = "Divide by Zero floating-point exception occurred.";
	} else {
		dzf->desc =
			"Divide by Zero floating-point exception did not occur.";
	}
}

static void describe_iof(struct bitfield *iof)
{
	if (iof->value == 1) {
		iof->desc =
			"Invalid Operation floating-point exception occurred.";
	} else {
		iof->desc =
			"Invalid Operation floating-point exception did not occur.";
	}
}

static void describe_ids(struct bitfield *ids)
{
	if (ids->value == 1) {
		ids->desc =
			"The rest of the ISS is encoded in an implementation-defined format";
	} else {
		ids->desc =
			"The rest of the ISS is encoded according to the platform";
	}
}

static void describe_iesb(struct bitfield *iesb)
{
	if (iesb->value == 1) {
		iesb->desc =
			"The SError interrupt was synchronized by the implicit error synchronization event and taken immediately.";
	} else {
		iesb->desc =
			"The SError interrupt was not synchronized by the implicit error synchronization event or not taken immediately.";
	}
}

static void describe_aet(struct bitfield *aet)
{
	switch (aet->value) {
	case 0b000:
		aet->desc = "Uncontainable (UC)";
		break;
	case 0b001:
		aet->desc = "Unrecoverable state (UEU)";
		break;
	case 0b010:
		aet->desc = "Restartable state (UEO)";
		break;
	case 0b011:
		aet->desc = "Recoverable state (UER)";
		break;
	case 0x110:
		aet->desc = "Corrected (CE)";
		break;
	default:
		aet->desc = "[ERROR]: bad aet";
	}
}

static void describe_serror_dfsc(struct bitfield *dfsc)
{
	switch (dfsc->value) {
	case 0b000000:
		dfsc->desc = "Uncategorized error";
		break;
	case 0b010001:
		dfsc->desc = "Asynchronous SError interrupt";
		break;
	default:
		dfsc->desc = "[ERROR]: bad dfsc";
	}
}

static void describe_debug_fsc(struct bitfield *fsc)
{
	if (fsc->value == 0b100010) {
		fsc->desc = "Debug execution";
	} else {
		fsc->desc = "[ERROR]: bad fsc";
	}
}

static void describe_isv(struct bitfield *isv)
{
	if (isv->value == 1) {
		isv->desc = "EX bit is valid";
	} else {
		isv->desc = "EX bit is RES0";
	}
}

static void describe_ex(struct bitfield *ex)
{
	if (ex->value == 1) {
		ex->desc = "A Load-Exclusive instruction was stepped";
	} else {
		ex->desc =
			"Some instruction other than a Load-Exclusive was stepped";
	}
}

static void describe_wptv(struct bitfield *wptv)
{
	if (wptv->value == 1) {
		wptv->desc =
			"The WPT field is valid, and holds the number of a watchpoint that triggered a Watchpoint exception";
	} else {
		wptv->desc =
			"The WPT field is invalid, and holds an UNKNOWN value";
	}
}

static void describe_wpf(struct bitfield *wpf)
{
	if (wpf->value == 1) {
		wpf->desc =
			"The watchpoint matched an access or set of contiguous accesses where the lowest accessed address was rounded down to the nearest multiple of 16 bytes and the highest accessed address was rounded up to the nearest multiple of 16 bytes minus 1, but the watchpoint might not have matched the original access or set of contiguous accesses";
	} else {
		wpf->desc =
			"The watchpoint matched the original access or set of contiguous accesses";
	}
}

static void describe_fnp(struct bitfield *fnp)
{
	if (fnp->value == 1) {
		fnp->desc =
			"The FAR holds any address within the smallest implemented translation granule that contains the virtual address of an access or set of contiguous accesses that triggered a Watchpoint exception";
	} else {
		fnp->desc =
			"If the FnV field is 0, the FAR holds the virtual address of an access or set of contiguous accesses that triggered a Watchpoint exception";
	}
}

static void describe_wp_vncr(struct bitfield *vncr)
{
	if (vncr->value == 1) {
		vncr->desc =
			"The watchpoint was generated by the use of VNCR_EL2 by EL1 code";
	} else {
		vncr->desc =
			"The watchpoint was not generated by the use of VNCR_EL2 by EL1 code";
	}
}

static void describe_wp_fnv(struct bitfield *fnv)
{
	if (fnv->value == 1) {
		fnv->desc = "The FAR is invalid, and holds an UNKNOWN value";
	} else {
		fnv->desc =
			"The FAR is valid, and its value is as described by the FnP field";
	}
}

static void describe_wp_cm(struct bitfield *cm)
{
	if (cm->value == 1) {
		cm->desc =
			"The Watchpoint exception was generated by either the execution of a cache maintenance instruction or by a synchronous Watchpoint exception on the execution of an address translation instruction.";
	} else {
		cm->desc =
			"The Watchpoint exception was not generated by the execution of one of the System instructions identified in the description of value 1";
	}
}

static void describe_wp_wnr(struct bitfield *wnr)
{
	if (wnr->value == 1) {
		wnr->desc =
			"Watchpoint exception caused by an instruction writing to a memory location";
	} else {
		wnr->desc =
			"Watchpoint exception caused by an instruction reading from a memory location";
	}
}

static void decode_iss_data_abort(struct bitfield *iss)
{
	struct bitfield fsc;

	u64 isv = bitfield_describe("ISV", "Instruction Syndrome Valid", 24, 24,
				    NULL);

	if (isv == 1) {
		bitfield_describe("SAS", "Syndrome Access Size", 22, 23,
				  describe_sas);
		bitfield_describe("SSE", "Syndrome Sign Extend", 21, 21, NULL);
		bitfield_describe("SRT", "Syndrome Register Transfer", 16, 20,
				  NULL);
		bitfield_describe("SF", "Sixty-Four", 15, 15, NULL);
		bitfield_describe("AR", "Acquire/Release", 14, 14, describe_ar);
	} else {
		describe_res0(14, 23);
	}

	bitfield_describe("VNCR", NULL, 13, 13, NULL);

	bitfield_new(iss->value, "DFSC", "Data Faule Status Code", 0, 5,
		     describe_fsc, &fsc);

	if (fsc.value == 0b010000) {
		bitfield_describe("SET", "Synchronous Error Type", 11, 12,
				  describe_set);
	} else {
		describe_res0(11, 12);
	}

	bitfield_describe("FnV", "FAR not Valid", 10, 10, describe_fnv);
	bitfield_describe("EA", "External Abort type", 9, 9, NULL);
	bitfield_describe("CM", "Cache Maintenance", 8, 8, NULL);
	bitfield_describe("S1PTW", "Stage-1 translation table walk", 7, 7,
			  describe_s1ptw);
	bitfield_describe("WnR", "Write not Read", 6, 6, describe_wnr);
	bitfield_print(&fsc);
}

static void decode_iss_res0(struct bitfield *iss)
{
	describe_res0(0, 24);
}

static void decode_iss_wf(struct bitfield *iss)
{
	bitfield_describe("CV", "Condition code valid", 24, 24, decribe_cv);
	bitfield_describe("COND", "Condition code of the trapped instruction",
			  20, 23, NULL);
	describe_res0(10, 19);
	bitfield_describe("RN", "Register Number", 5, 9, NULL);
	describe_res0(3, 4);
	bitfield_describe("RV", "Register valid", 2, 2, describe_rv);
	bitfield_describe("TI", "Trapped Instruction", 0, 1, describe_ti);
}

static void decode_iss_mcr(struct bitfield *iss)
{
	bitfield_describe("CV", "Condition code valid", 24, 24, decribe_cv);
	bitfield_describe("COND", "Condition code of the trapped instruction",
			  20, 23, NULL);
	bitfield_describe("Opc2", NULL, 17, 19, NULL);
	bitfield_describe("Opc1", NULL, 14, 17, NULL);
	bitfield_describe("Crn", NULL, 10, 13, NULL);
	bitfield_describe("Rt", NULL, 5, 9, NULL);
	bitfield_describe("CRm", NULL, 1, 4, NULL);
	bitfield_describe("Dir", "Direction of the trapped instruction", 0, 0,
			  describe_mcr_direction);
}

static void decode_iss_mcrr(struct bitfield *iss)
{
	bitfield_describe("CV", "Condition code valid", 24, 24, describe_cv);
	bitfield_describe("COND", "Condition code of the trapped instruction",
			  20, 23, NULL);
	bitfield_describe("Opc1", NULL, 16, 19, NULL);
	describe_res0(15, 15);
	bitfield_describe("Rt2", NULL, 10, 14, NULL);
	bitfield_describe("Rt", NULL, 5, 9, NULL);
	bitfield_describe("CRm", NULL, 1, 4, NULL);
	bitfield_describe("Dir", "Direction of the trapped instruction", 0, 0,
			  describe_mcr_direction);
}

static void decode_iss_ldc(struct bitfield *iss)
{
	bitfield_describe("CV", "Condition code valid", 24, 24, describe_cv);
	bitfield_describe("COND", "Condition code of the trapped instruction",
			  20, 23, NULL);
	bitfield_describe("imm8", "Immediate value of the trapped instruction",
			  12, 19, NULL);
	describe_res0(10, 11);
	bitfield_describe(
		"Rn",
		"General-purpose register number of the trapped instruction", 5,
		9, NULL);
	bitfield_describe("Offset",
			  "Whether the offset is added or substracted", 4, 4,
			  describe_offset);
	bitfield_describe("AM", "Addressing Mode", 1, 3, describe_am);
	bitfield_describe("Dir", "Direction of the trapped instruction", 0, 0,
			  describe_ldc_direction);
}

static void decode_iss_sve(struct bitfield *iss)
{
	bitfield_describe("CV", "Condition code valid", 24, 24, describe_cv);
	bitfield_describe("COND", "Condition code of the trapped instruction",
			  20, 23, NULL);
	describe_res0(0, 19);
}

static void decode_iss_ld64b(struct bitfield *iss)
{
	bitfield_describe("ISS", NULL, 0, 24, describe_iss_ld64b);
}

static void decode_iss_bti(struct bitfield *iss)
{
	describe_res0(2, 24);
	bitfield_describe("BTYPE", "PSTATE.BTYPE value", 0, 1, NULL);
}

static void decode_iss_hvc(struct bitfield *iss)
{
	describe_res0(16, 24);
	bitfield_describe("imm16", "Value of the immediate field", 0, 15, NULL);
}

#define SYSREG_INDEX(op0, crn, op1, crm, op2) \
	((op0 << 20) | (crn << 10) | (op1 << 14) | (crm << 1) | (op2 << 17))

char *sysreg_name(u64 op0, u64 op1, u64 op2, u64 crn, u64 crm)
{
	switch (SYSREG_INDEX(op0, crn, op1, crm, op2)) {
	case SYSREG_INDEX(3, 1, 0, 0, 1):
		return "ACTLR_EL1";
	case SYSREG_INDEX(3, 1, 4, 0, 1):
		return "ACTLR_EL2";
	case SYSREG_INDEX(3, 1, 6, 0, 1):
		return "ACTLR_EL3";
	case SYSREG_INDEX(3, 0, 1, 0, 7):
		return "AIDR_EL1";
	case SYSREG_INDEX(3, 5, 0, 1, 0):
		return "AFSR0_EL1";
	case SYSREG_INDEX(3, 5, 4, 1, 0):
		return "AFSR0_EL2";
	case SYSREG_INDEX(3, 5, 6, 1, 0):
		return "AFSR0_EL3";
	case SYSREG_INDEX(3, 5, 0, 1, 1):
		return "AFSR1_EL1";
	case SYSREG_INDEX(3, 5, 4, 1, 1):
		return "AFSR1_EL2";
	case SYSREG_INDEX(3, 5, 6, 1, 1):
		return "AFSR1_EL3";
	case SYSREG_INDEX(3, 10, 0, 3, 0):
		return "AMAIR_EL1";
	case SYSREG_INDEX(3, 10, 4, 3, 0):
		return "AMAIR_EL2";
	case SYSREG_INDEX(3, 10, 6, 3, 0):
		return "AMAIR_EL3";
	case SYSREG_INDEX(3, 0, 1, 0, 0):
		return "CCSIDR_EL1";
	case SYSREG_INDEX(3, 0, 1, 0, 1):
		return "CLIDR_EL1";
	case SYSREG_INDEX(3, 1, 0, 0, 2):
		return "CPACR_EL1";
	case SYSREG_INDEX(3, 1, 4, 1, 2):
		return "CPTR_EL2";
	case SYSREG_INDEX(3, 1, 6, 1, 2):
		return "CPTR_EL3";
	case SYSREG_INDEX(3, 0, 2, 0, 0):
		return "CSSELR_EL1";
	case SYSREG_INDEX(3, 0, 3, 0, 1):
		return "CTR_EL0";
	case SYSREG_INDEX(3, 12, 0, 1, 1):
		return "DISR_EL1";
	case SYSREG_INDEX(3, 5, 0, 3, 0):
		return "ERRIDR_EL1";
	case SYSREG_INDEX(3, 5, 0, 3, 1):
		return "ERRSELR_EL1";
	case SYSREG_INDEX(3, 5, 0, 4, 3):
		return "ERXADDR_EL1";
	case SYSREG_INDEX(3, 5, 0, 4, 1):
		return "ERXCTLR_EL1";
	case SYSREG_INDEX(3, 5, 0, 4, 0):
		return "ERXFR_EL1";
	case SYSREG_INDEX(3, 5, 0, 5, 0):
		return "ERXMISC0_EL1";
	case SYSREG_INDEX(3, 5, 0, 5, 1):
		return "ERXMISC1_EL1";
	case SYSREG_INDEX(3, 5, 0, 4, 2):
		return "ERXSTATUS_EL1";
	case SYSREG_INDEX(3, 5, 0, 2, 0):
		return "ESR_EL1";
	case SYSREG_INDEX(3, 5, 4, 2, 0):
		return "ESR_EL2";
	case SYSREG_INDEX(3, 5, 6, 2, 0):
		return "ESR_EL3";
	case SYSREG_INDEX(3, 1, 4, 1, 7):
		return "HACR_EL2";
	case SYSREG_INDEX(3, 1, 4, 1, 0):
		return "HCR_EL2";
	case SYSREG_INDEX(3, 0, 0, 1, 3):
		return "ID_AFR0_EL1";
	case SYSREG_INDEX(3, 0, 0, 1, 2):
		return "ID_DFR0_EL1";
	case SYSREG_INDEX(3, 0, 0, 2, 0):
		return "ID_ISAR0_EL1";
	case SYSREG_INDEX(3, 0, 0, 2, 1):
		return "ID_ISAR1_EL1";
	case SYSREG_INDEX(3, 0, 0, 2, 2):
		return "ID_ISAR2_EL1";
	case SYSREG_INDEX(3, 0, 0, 2, 3):
		return "ID_ISAR3_EL1";
	case SYSREG_INDEX(3, 0, 0, 2, 4):
		return "ID_ISAR4_EL1";
	case SYSREG_INDEX(3, 0, 0, 2, 5):
		return "ID_ISAR5_EL1";
	case SYSREG_INDEX(3, 0, 0, 2, 7):
		return "ID_ISAR6_EL1";
	case SYSREG_INDEX(3, 0, 0, 1, 4):
		return "ID_MMFR0_EL1";
	case SYSREG_INDEX(3, 0, 0, 1, 5):
		return "ID_MMFR1_EL1";
	case SYSREG_INDEX(3, 0, 0, 1, 6):
		return "ID_MMFR2_EL1";
	case SYSREG_INDEX(3, 0, 0, 1, 7):
		return "ID_MMFR3_EL1";
	case SYSREG_INDEX(3, 0, 0, 2, 6):
		return "ID_MMFR4_EL1";
	case SYSREG_INDEX(3, 0, 0, 1, 0):
		return "ID_PFR0_EL1";
	case SYSREG_INDEX(3, 0, 0, 1, 1):
		return "ID_PFR1_EL1";
	case SYSREG_INDEX(3, 0, 0, 3, 4):
		return "ID_PFR2_EL1";
	case SYSREG_INDEX(3, 0, 0, 5, 0):
		return "ID_AA64DFR0_EL1";
	case SYSREG_INDEX(3, 0, 0, 6, 0):
		return "ID_AA64ISAR0_EL1";
	case SYSREG_INDEX(3, 0, 0, 6, 1):
		return "ID_AA64ISAR1_EL1";
	case SYSREG_INDEX(3, 0, 0, 7, 0):
		return "ID_AA64MMFR0_EL1";
	case SYSREG_INDEX(3, 0, 0, 7, 1):
		return "ID_AA64MMFR1_EL1";
	case SYSREG_INDEX(3, 0, 0, 7, 2):
		return "ID_AA64MMFR2_EL1";
	case SYSREG_INDEX(3, 0, 0, 4, 0):
		return "ID_AA64PFR0_EL1";
	case SYSREG_INDEX(3, 5, 4, 0, 1):
		return "IFSR32_EL2";
	case SYSREG_INDEX(3, 10, 0, 4, 3):
		return "LORC_EL1";
	case SYSREG_INDEX(3, 10, 0, 4, 7):
		return "LORID_EL1";
	case SYSREG_INDEX(3, 10, 0, 4, 2):
		return "LORN_EL1";
	case SYSREG_INDEX(3, 1, 6, 3, 1):
		return "MDCR_EL3";
	case SYSREG_INDEX(3, 0, 0, 0, 0):
		return "MIDR_EL1";
	case SYSREG_INDEX(3, 0, 0, 0, 5):
		return "MPIDR_EL1";
	case SYSREG_INDEX(3, 7, 0, 4, 0):
		return "PAR_EL1";
	case SYSREG_INDEX(3, 12, 6, 0, 1):
		return "RVBAR_EL3";
	case SYSREG_INDEX(3, 0, 0, 0, 6):
		return "REVIDR_EL1";
	case SYSREG_INDEX(3, 1, 0, 0, 0):
		return "SCTLR_EL1";
	case SYSREG_INDEX(3, 1, 6, 0, 0):
		return "SCTLR_EL3";
	case SYSREG_INDEX(3, 2, 0, 0, 2):
		return "TCR_EL1";
	case SYSREG_INDEX(3, 2, 4, 0, 2):
		return "TCR_EL2";
	case SYSREG_INDEX(3, 2, 6, 0, 2):
		return "TCR_EL3";
	case SYSREG_INDEX(3, 2, 0, 0, 0):
		return "TTBR0_EL1";
	case SYSREG_INDEX(3, 2, 4, 0, 0):
		return "TTBR0_EL2";
	case SYSREG_INDEX(3, 2, 6, 0, 0):
		return "TTBR0_EL3";
	case SYSREG_INDEX(3, 2, 0, 0, 1):
		return "TTBR1_EL1";
	case SYSREG_INDEX(3, 2, 4, 0, 1):
		return "TTBR1_EL2";
	case SYSREG_INDEX(3, 12, 4, 1, 1):
		return "VDISR_EL2";
	case SYSREG_INDEX(3, 5, 4, 2, 3):
		return "VSESR_EL2";
	case SYSREG_INDEX(3, 2, 4, 1, 2):
		return "VTCR_EL2";
	case SYSREG_INDEX(3, 2, 4, 1, 0):
		return "VTTBR_EL2";
	case SYSREG_INDEX(3, 5, 5, 1, 0):
		return "AFSR0_EL12";
	case SYSREG_INDEX(3, 5, 5, 1, 1):
		return "AFSR1_EL12";
	case SYSREG_INDEX(3, 10, 5, 3, 0):
		return "AMAIR_EL12";
	case SYSREG_INDEX(3, 14, 3, 0, 0):
		return "CNTFRQ_EL0";
	case SYSREG_INDEX(3, 14, 4, 1, 0):
		return "CNTHCTL_EL2";
	case SYSREG_INDEX(3, 14, 4, 2, 1):
		return "CNTHP_CTL_EL2";
	case SYSREG_INDEX(3, 14, 4, 2, 2):
		return "CNTHP_CVAL_EL2";
	case SYSREG_INDEX(3, 14, 4, 2, 0):
		return "CNTHP_TVAL_EL2";
	case SYSREG_INDEX(3, 14, 4, 3, 1):
		return "CNTHV_CTL_EL2";
	case SYSREG_INDEX(3, 14, 4, 3, 2):
		return "CNTHV_CVAL_EL2";
	case SYSREG_INDEX(3, 14, 4, 3, 0):
		return "CNTHV_TVAL_EL2";
	case SYSREG_INDEX(3, 14, 0, 1, 0):
		return "CNTKCTL_EL1";
	case SYSREG_INDEX(3, 14, 5, 1, 0):
		return "CNTKCTL_EL12";
	case SYSREG_INDEX(3, 14, 3, 2, 1):
		return "CNTP_CTL_EL0";
	case SYSREG_INDEX(3, 14, 5, 2, 1):
		return "CNTP_CTL_EL02";
	case SYSREG_INDEX(3, 14, 3, 2, 2):
		return "CNTP_CVAL_EL0";
	case SYSREG_INDEX(3, 14, 5, 2, 2):
		return "CNTP_CVAL_EL02";
	case SYSREG_INDEX(3, 14, 3, 2, 0):
		return "CNTP_TVAL_EL0";
	case SYSREG_INDEX(3, 14, 5, 2, 0):
		return "CNTP_TVAL_EL02";
	case SYSREG_INDEX(3, 14, 3, 0, 1):
		return "CNTPCT_EL0";
	case SYSREG_INDEX(3, 14, 7, 2, 1):
		return "CNTPS_CTL_EL1";
	case SYSREG_INDEX(3, 14, 7, 2, 2):
		return "CNTPS_CVAL_EL1";
	case SYSREG_INDEX(3, 14, 7, 2, 0):
		return "CNTPS_TVAL_EL1";
	case SYSREG_INDEX(3, 14, 3, 3, 1):
		return "CNTV_CTL_EL0";
	case SYSREG_INDEX(3, 14, 5, 3, 1):
		return "CNTV_CTL_EL02";
	case SYSREG_INDEX(3, 14, 3, 3, 2):
		return "CNTV_CVAL_EL0";
	case SYSREG_INDEX(3, 14, 5, 3, 2):
		return "CNTV_CVAL_EL02";
	case SYSREG_INDEX(3, 14, 3, 3, 0):
		return "CNTV_TVAL_EL0";
	case SYSREG_INDEX(3, 14, 5, 3, 0):
		return "CNTV_TVAL_EL02";
	case SYSREG_INDEX(3, 14, 3, 0, 2):
		return "CNTVCT_EL0";
	case SYSREG_INDEX(3, 14, 4, 0, 3):
		return "CNTVOFF_EL2";
	case SYSREG_INDEX(3, 13, 0, 0, 1):
		return "CONTEXTIDR_EL1";
	case SYSREG_INDEX(3, 13, 5, 0, 1):
		return "CONTEXTIDR_EL12";
	case SYSREG_INDEX(3, 13, 4, 0, 1):
		return "CONTEXTIDR_EL2";
	case SYSREG_INDEX(3, 1, 5, 0, 2):
		return "CPACR_EL12";
	case SYSREG_INDEX(3, 3, 4, 0, 0):
		return "DACR32_EL2";
	case SYSREG_INDEX(3, 5, 5, 2, 0):
		return "ESR_EL12";
	case SYSREG_INDEX(3, 6, 0, 0, 0):
		return "FAR_EL1";
	case SYSREG_INDEX(3, 6, 5, 0, 0):
		return "FAR_EL12";
	case SYSREG_INDEX(3, 6, 4, 0, 0):
		return "FAR_EL2";
	case SYSREG_INDEX(3, 6, 6, 0, 0):
		return "FAR_EL3";
	case SYSREG_INDEX(3, 5, 4, 3, 0):
		return "FPEXC32_EL2";
	case SYSREG_INDEX(3, 6, 4, 0, 4):
		return "HPFAR_EL2";
	case SYSREG_INDEX(3, 1, 4, 1, 3):
		return "HSTR_EL2";
	case SYSREG_INDEX(3, 0, 0, 5, 4):
		return "ID_AA64AFR0_EL1";
	case SYSREG_INDEX(3, 0, 0, 5, 5):
		return "ID_AA64AFR1_EL1";
	case SYSREG_INDEX(3, 0, 0, 5, 1):
		return "ID_AA64DFR1_EL1";
	case SYSREG_INDEX(3, 0, 0, 4, 1):
		return "ID_AA64PFR1_EL1";
	case SYSREG_INDEX(3, 12, 0, 1, 0):
		return "ISR_EL1";
	case SYSREG_INDEX(3, 10, 0, 4, 1):
		return "LOREA_EL1";
	case SYSREG_INDEX(3, 10, 0, 4, 0):
		return "LORSA_EL1";
	case SYSREG_INDEX(3, 10, 0, 2, 0):
		return "MAIR_EL1";
	case SYSREG_INDEX(3, 10, 5, 2, 0):
		return "MAIR_EL12";
	case SYSREG_INDEX(3, 10, 4, 2, 0):
		return "MAIR_EL2";
	case SYSREG_INDEX(3, 10, 6, 2, 0):
		return "MAIR_EL3";
	case SYSREG_INDEX(3, 1, 4, 1, 1):
		return "MDCR_EL2";
	case SYSREG_INDEX(3, 0, 0, 3, 0):
		return "MVFR0_EL1";
	case SYSREG_INDEX(3, 0, 0, 3, 1):
		return "MVFR1_EL1";
	case SYSREG_INDEX(3, 0, 0, 3, 2):
		return "MVFR2_EL1";
	case SYSREG_INDEX(3, 12, 6, 0, 2):
		return "RMR_EL3";
	case SYSREG_INDEX(3, 1, 6, 1, 0):
		return "SCR_EL3";
	case SYSREG_INDEX(3, 1, 5, 0, 0):
		return "SCTLR_EL12";
	case SYSREG_INDEX(3, 1, 4, 0, 0):
		return "SCTLR_EL2";
	case SYSREG_INDEX(3, 1, 6, 1, 1):
		return "SDER32_EL3";
	case SYSREG_INDEX(3, 2, 5, 0, 2):
		return "TCR_EL12";
	case SYSREG_INDEX(3, 13, 3, 0, 2):
		return "TPIDR_EL0";
	case SYSREG_INDEX(3, 13, 0, 0, 4):
		return "TPIDR_EL1";
	case SYSREG_INDEX(3, 13, 4, 0, 2):
		return "TPIDR_EL2";
	case SYSREG_INDEX(3, 13, 6, 0, 2):
		return "TPIDR_EL3";
	case SYSREG_INDEX(3, 13, 3, 0, 3):
		return "TPIDRRO_EL0";
	case SYSREG_INDEX(3, 2, 5, 0, 0):
		return "TTBR0_EL12";
	case SYSREG_INDEX(3, 2, 5, 0, 1):
		return "TTBR1_EL12";
	case SYSREG_INDEX(3, 12, 0, 0, 0):
		return "VBAR_EL1";
	case SYSREG_INDEX(3, 12, 5, 0, 0):
		return "VBAR_EL12";
	case SYSREG_INDEX(3, 12, 4, 0, 0):
		return "VBAR_EL2";
	case SYSREG_INDEX(3, 12, 6, 0, 0):
		return "VBAR_EL3";
	case SYSREG_INDEX(3, 0, 4, 0, 5):
		return "VMPIDR_EL2";
	case SYSREG_INDEX(3, 0, 4, 0, 0):
		return "VPIDR_EL2";
	default:
		return "unknown";
	}
}

static void decode_iss_msr(struct bitfield *iss)
{
	describe_res0(22, 24);
	u64 op0 = bitfield_describe("Op0", NULL, 20, 21, NULL);
	u64 op2 = bitfield_describe("Op2", NULL, 17, 19, NULL);
	u64 op1 = bitfield_describe("Op1", NULL, 14, 16, NULL);
	u64 crn = bitfield_describe("CRn", NULL, 10, 13, NULL);
	u64 rt = bitfield_describe(
		"Rt",
		"General-purpose register number of the trapped instruction", 5,
		9, NULL);
	u64 crm = bitfield_describe("CRm", NULL, 1, 4, NULL);
	u64 dir = bitfield_describe("Dir",
				    "Direction of the trapped instruction", 0,
				    0, describe_msr_direction);
	if (dir) {
		printf("# MRS x%lu, %s\n", rt,
		       sysreg_name(op0, op1, op2, crn, crm));
	} else {
		printf("# MSR %s, x%lu\n", sysreg_name(op0, op1, op2, crn, crm),
		       rt);
	}
}

static void decode_iss_tstart(struct bitfield *iss)
{
	describe_res0(10, 24);
	bitfield_describe(
		"Rd",
		"General-purpose register number used for the destination", 5,
		9, NULL);
	describe_res0(0, 4);
}

static void decode_iss_pauth(struct bitfield *iss)
{
	describe_res0(2, 24);
	bitfield_describe("IorD", "Instruction key or Data key", 1, 1,
			  describe_iord);
	bitfield_describe("AorB", "A key or B key", 0, 0, describe_aorb);
}

static void decode_iss_sme(struct bitfield *iss)
{
	describe_res0(3, 24);
	bitfield_describe("SMTC", "SME Trap Code", 0, 2, describe_smtc);
}

static void decode_iss_gpc(struct bitfield *iss)
{
	describe_res0(22, 24);
	bitfield_describe("S2PTW", "Stage-2 translation table walk", 21, 21,
			  describe_s2ptw);
	u64 ind = bitfield_describe("InD", "Instruction or Data access", 20, 20,
				    describe_ind);
	bitfield_describe("GPCSC", "Granule Protection Check Status Code", 14,
			  19, describe_gpcsc);
	bitfield_describe("VNCR", NULL, 13, 13, describe_vncr);
	describe_res0(11, 12);
	describe_res0(9, 10);
	bitfield_describe("CM", "Cache maintenance", 8, 8, describe_cm);
	bitfield_describe("S1PTW", "Stage-1 translation table walk", 7, 7,
			  describe_s1ptw);
	if (ind == 1) {
		describe_res0(6, 6);
	} else {
		bitfield_describe("WnR", "Write or Read", 6, 6,
				  describe_gpc_wnr);
	}
	bitfield_describe("xFSC", "Instruction or Data Fault Status Code", 0, 5,
			  describe_xfsc);
}

static void decode_iss_default(struct bitfield *iss)
{
	iss->desc = "[ERROR]: bad iss";
	bitfield_print(iss);
}

static void decode_iss_instruction_abort(struct bitfield *iss)
{
	struct bitfield fsc;

	describe_res0(13, 24);
	bitfield_new(iss->value, "IFSC", "Instruction Fault Status Code", 0, 5,
		     describe_fsc, &fsc);
	if (fsc.value == 0b010000) {
		bitfield_describe("SET", "Synchronous Error Type", 11, 12,
				  describe_set);
	} else {
		describe_res0(11, 12);
	}
	bitfield_describe("FnV", "FAR not Valid", 10, 10, describe_fnv);
	bitfield_describe("EA", "External About type", 9, 9, NULL);
	describe_res0(8, 8);
	bitfield_describe("S1PTW", "Stage-1 translation table walk", 7, 7,
			  describe_s1ptw);
	describe_res0(6, 6);
	bitfield_print(&fsc);
}

static void decode_iss_fp(struct bitfield *iss)
{
	describe_res0(24, 24);
	bitfield_describe("TFV", "Trapped Fault Valid", 23, 23, describe_tfv);
	describe_res0(11, 22);
	bitfield_describe("VECITR", "RES1 or UNKNOWN", 8, 10, NULL);
	bitfield_describe("IDF", "Input Denomal", 7, 7, describe_idf);
	describe_res0(5, 6);
	bitfield_describe("IXF", "Inexact", 4, 4, describe_ixf);
	bitfield_describe("UFF", "Underflow", 3, 3, describe_uff);
	bitfield_describe("OFF", "Overflow", 2, 2, describe_off);
	bitfield_describe("DZF", "Divide by Zero", 1, 1, describe_dzf);
	bitfield_describe("IOF", "Invalid Operation", 0, 0, describe_iof);
}

static void decode_iss_serror(struct bitfield *iss)
{
	struct bitfield dfsc;
	u64 ids = bitfield_describe("IDS", "Implementation Defined Syndrome",
				    24, 24, describe_ids);
	if (ids == 1) {
		bitfield_describe("IMPDEF", "Implementation defined", 0, 23,
				  NULL);
		return;
	}

	describe_res0(14, 23);
	bitfield_new(iss->value, "DFSC", "Data Fault Status Code", 0, 5,
		     describe_serror_dfsc, &dfsc);
	if (dfsc.value == 0b010001) {
		bitfield_describe("IESB",
				  "Implicit Error Synchronisation event", 13,
				  13, describe_iesb);
	} else {
		describe_res0(13, 13);
	}
	bitfield_describe("AET", "Asynchronous Error Type", 10, 12,
			  describe_aet);
	if (dfsc.value == 0b010001) {
		bitfield_describe("EA", "External Abort type", 9, 9, NULL);
	} else {
		describe_res0(9, 9);
	}
	describe_res0(6, 8);
	bitfield_print(&dfsc);
}

static void decode_iss_breakpoint_vector_catch(struct bitfield *iss)
{
	describe_res0(6, 24);
	bitfield_describe("IFSC", "Instruction Fault Status Code", 0, 5,
			  describe_debug_fsc);
}

static void decode_iss_software_step(struct bitfield *iss)
{
	u64 isv = bitfield_describe("ISV", "Instruction Syndrome Valid", 24, 24,
				    describe_isv);
	describe_res0(7, 23);
	if (isv == 1) {
		bitfield_describe("EX", "Exclusive operation", 6, 6,
				  describe_ex);
	} else {
		describe_res0(6, 6);
	}
	bitfield_describe("IFSC", "Instruction Fault Status Code", 0, 5,
			  describe_debug_fsc);
}

static void decode_iss_watchpoint(struct bitfield *iss)
{
	describe_res0(24, 24);
	bitfield_describe("WPT", "Watchpoint number", 18, 23, NULL);
	bitfield_describe("WPTV", "Watchpoint number Valid", 17, 17,
			  describe_wptv);
	bitfield_describe("WPF", "Watchpoint might be false-positive", 16, 16,
			  describe_wpf);
	bitfield_describe("FnP", "FAR not Precise", 15, 15, describe_fnp);
	describe_res0(14, 14);
	bitfield_describe("VNCR", NULL, 13, 13, describe_wp_vncr);
	describe_res0(11, 12);
	bitfield_describe("FnV", "FAR not Valid", 10, 10, describe_wp_fnv);
	describe_res0(9, 9);
	bitfield_describe("CM", "Cache Maintenance", 8, 8, describe_wp_cm);
	describe_res0(7, 7);
	bitfield_describe("WnR", "Write not Read", 6, 6, describe_wp_wnr);
	bitfield_describe("DFSC", "Data Fault Status Code", 0, 5,
			  describe_debug_fsc);
}

static void decode_iss_breakpoint(struct bitfield *iss)
{
	describe_res0(16, 24);
	bitfield_describe("Comment",
			  "Instruction comment field or immediate field", 0, 15,
			  NULL);
}

describe_fn decode_ec()
{
	struct bitfield ec;
	describe_fn iss_decoder = decode_iss_default;

	bitfield_new(_esr, "EC", "Exception Class", 26, 31, NULL, &ec);

	switch (ec.value) {
	case 0b000000:
		ec.desc = "Unknown reason";
		iss_decoder = decode_iss_res0;
		break;
	case 0b000001:
		ec.desc = "Wrapped WF* instruction execution";
		iss_decoder = decode_iss_wf;
		break;
	case 0b000011:
		ec.desc = "Trapped MCR or MRC access with coproc = 0b1111";
		iss_decoder = decode_iss_mcr;
		break;
	case 0b000100:
		ec.desc = "Trapped MCRR or MRRC access with coproc = 0b1111";
		iss_decoder = decode_iss_mcrr;
		break;
	case 0b000101:
		ec.desc = "Trapped MCR or MRC access with coproc = 0b1110";
		iss_decoder = decode_iss_mcr;
		break;
	case 0b000110:
		ec.desc = "Trapped LDC or STC access";
		iss_decoder = decode_iss_ldc;
		break;
	case 0b000111:
		ec.desc =
			"Trapped access to SVE, Advanced SIMD or floating point";
		iss_decoder = decode_iss_sve;
		break;
	case 0b001010:
		ec.desc =
			"Trapped execution of an LD64B, ST64B, ST64BV, or ST64BV0 instruction";
		iss_decoder = decode_iss_ld64b;
		break;
	case 0b001100:
		ec.desc = "Trapped MRRC access with coproc == 0b1110";
		iss_decoder = decode_iss_mcrr;
		break;
	case 0b001101:
		ec.desc = "Branch Target Exception";
		iss_decoder = decode_iss_bti;
		break;
	case 0b001110:
		ec.desc = "Illegal Execution state";
		iss_decoder = decode_iss_res0;
		break;
	case 0b010001:
		ec.desc = "SVC instruction execution in AArch32 state";
		iss_decoder = decode_iss_hvc;
		break;
	case 0b010101:
		ec.desc = "SVC instruction execution in AArch64 state";
		iss_decoder = decode_iss_hvc;
		break;
	case 0b010110:
		ec.desc = "HVC instruction execution in AArch64 state";
		iss_decoder = decode_iss_hvc;
		break;
	case 0b010111:
		ec.desc = "SMC instruction execution in AArch64 state";
		iss_decoder = decode_iss_hvc;
		break;
	case 0b011000:
		ec.desc =
			"Trapped MSR, MRS or System instruction execution in AArch64 state";
		iss_decoder = decode_iss_msr;
		break;
	case 0b011001:
		ec.desc =
			"Access to SVE functionality trapped as a result of CPACR_EL1.ZEN, CPTR_EL2.ZEN, CPTR_EL2.TZ, or CPTR_EL3.EZ";
		iss_decoder = decode_iss_res0;
		break;
	case 0b011011:
		ec.desc =
			"Exception from an access to a TSTART instruction at EL0 when SCTLR_EL1.TME0 == 0, EL0 when SCTLR_EL2.TME0 == 0, at EL1 when SCTLR_EL1.TME == 0, at EL2 when SCTLR_EL2.TME == 0 or at EL3 when SCTLR_EL3.TME == 0";
		iss_decoder = decode_iss_tstart;
		break;
	case 0b011100:
		ec.desc =
			"Exception from a Pointer Authentication instruction authentication failure";
		iss_decoder = decode_iss_pauth;
		break;
	case 0b011101:
		ec.desc =
			"Access to SME functionality trapped as a result of CPACR_EL1.SMEN, CPTR_EL2.SMEN, CPTR_EL2.TSM, CPTR_EL3.ESM, or an attempted execution of an instruction that is illegal because of the value of PSTATE.SM or PSTATE.ZA";
		iss_decoder = decode_iss_sme;
		break;
	case 0b011110:
		ec.desc = "Exception from a Granule Protection Check";
		iss_decoder = decode_iss_gpc;
		break;
	case 0b100000:
		ec.desc = "Instruction Abort from a lower Exception level";
		iss_decoder = decode_iss_instruction_abort;
		break;
	case 0b100001:
		ec.desc =
			"Instruction Abort taken without a change in Exception level";
		iss_decoder = decode_iss_instruction_abort;
		break;
	case 0b100010:
		ec.desc = "PC alignment fault exception";
		iss_decoder = decode_iss_res0;
		break;
	case 0b100100:
		ec.desc = "Data Abort from a lower Exception level";
		iss_decoder = decode_iss_data_abort;
	case 0b100101:
		ec.desc =
			"Data Abort taken without a change in Exception level";
		iss_decoder = decode_iss_data_abort;
		break;
	case 0b100110:
		ec.desc = "SP alignment fault exception";
		iss_decoder = decode_iss_res0;
		break;
	case 0b101000:
		ec.desc =
			"Trapped floating-ppint exception taken from AArch32 state";
		iss_decoder = decode_iss_fp;
		break;
	case 0b101100:
		ec.desc =
			"Trapped floating-ppint exception taken from AArch64 state";
		iss_decoder = decode_iss_fp;
		break;
	case 0b101111:
		ec.desc = "SError interrupt";
		iss_decoder = decode_iss_serror;
		break;
	case 0b110000:
		ec.desc = "Breakpoint execution from a lower Exception level";
		iss_decoder = decode_iss_breakpoint_vector_catch;
		break;
	case 0b110001:
		ec.desc =
			"Breakpoint exception taken without a change in Exception level";
		iss_decoder = decode_iss_breakpoint_vector_catch;
		break;
	case 0b110010:
		ec.desc =
			"Software Step exception from a lower Exception level";
		iss_decoder = decode_iss_software_step;
		break;
	case 0b110011:
		ec.desc =
			"Software Step exception taken without a change in Exception level";
		iss_decoder = decode_iss_software_step;
		break;
	case 0b110100:
		ec.desc = "Watchpoint exception from a lower Exception level";
		iss_decoder = decode_iss_watchpoint;
		break;
	case 0b110101:
		ec.desc =
			"Watchpoint exception taken without a change in Exception level";
		iss_decoder = decode_iss_watchpoint;
		break;
	case 0b111000:
		ec.desc = "BKPT instruction execution in AArch32 state";
		iss_decoder = decode_iss_breakpoint;
		break;
	case 0b111100:
		ec.desc = "BRK instruction execution in AArch64 state";
		iss_decoder = decode_iss_breakpoint;
		break;
	default:
		ec.desc = "[ERROR]: bad ec";
		break;
	}

	bitfield_print(&ec);

	return iss_decoder;
}

static void decode(u64 esr)
{
	struct bitfield res0;
	struct bitfield iss2;
	struct bitfield iss;

	_esr = esr;

	describe_res0(37, 63);
	bitfield_describe("ISS2", "Instruction Specific Syndrome 2", 32, 36,
			  NULL);

	describe_fn iss_decoder = decode_ec();

	bitfield_describe("IL", "Instruction Length", 25, 25, describe_il);

	bitfield_new(esr, "ISS", "Instruction Specific Syndrome", 0, 24,
		     iss_decoder, &iss);
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("bad input\n");
		exit(1);
	}

	for (int i = 1; i < argc; i++) {
		printf("ESR: %s\n", argv[i]);
		u64 reg;
		sscanf(argv[i], "%lx", &reg);
		decode(reg);
		printf("\n");
	}

	return 0;
}
