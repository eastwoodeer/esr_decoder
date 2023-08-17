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
typedef void (*decode_iss_fn)(struct bitfield *);

u64 get_bits(u64 reg, size_t start, size_t end)
{
	u64 width = end - start + 1;
	return (reg >> start) & ((1 << width) - 1);
}

void bitfield_new(u64 reg, char *name, char *long_name, size_t start,
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

void field_append(struct bitfield *head, struct bitfield *new)
{
	new->prev = head->prev;
	new->next = head;
	head->prev->next = new;
	head->prev = new;
}

void decimal_to_binary(u64 n, size_t width, char *buf)
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

void field_description(struct bitfield *field)
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

void bitfield_print(struct bitfield *head)
{
	field_description(head);
	for (struct bitfield *p = head->next; p != head; p = p->next) {
		p->indent = "\t";
		field_description(p);
	}
}

u64 bitfield_describe(char *name, char *long_name, size_t start, size_t end,
		      describe_fn desc)
{
	struct bitfield field;
	bitfield_new(_esr, name, long_name, start, end, desc, &field);
	bitfield_print(&field);
	if (desc) {
		desc(&field);
	}
	return field.value;
}

void describe_il(struct bitfield *field)
{
	if (field->value == 1) {
		field->desc = "32-bit instruction trapped";
	} else {
		field->desc = "16-bit instruction trapped";
	}
}

void describe_sas(struct bitfield *sas)
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

void describe_ar(struct bitfield *ar)
{
	if (ar->value == 1) {
		ar->desc = "Acquire/Release semantics";
	} else {
		ar->desc = "No Acquire/Release semantics";
	}
}

void describe_fnv(struct bitfield *fnv)
{
	if (fnv->value == 1) {
		fnv->desc = "FAR is not valid, it holds an unknown value";
	} else {
		fnv->desc = "FAR is valid";
	}
}

void describe_wnr(struct bitfield *wnr)
{
	if (wnr->value == 1) {
		wnr->desc = "Abort caused by writing to memory";
	} else {
		wnr->desc = "Abort caused by reading from memory";
	}
}

void describe_dfsc(struct bitfield *dfsc)
{
	switch (dfsc->value) {
	case 0b000000:
		dfsc->desc =
			"Address size fault, level 0 of translation or translation table base register.";
		break;
	case 0b000001:
		dfsc->desc = "Address size fault, level 1.";
		break;
	case 0b000010:
		dfsc->desc = "Address size fault, level 2.";
		break;
	case 0b000011:
		dfsc->desc = "Address size fault, level 3.";
		break;
	case 0b000100:
		dfsc->desc = "Translation fault, level 0.";
		break;
	case 0b000101:
		dfsc->desc = "Translation fault, level 1.";
		break;
	case 0b000110:
		dfsc->desc = "Translation fault, level 2.";
		break;
	case 0b000111:
		dfsc->desc = "Translation fault, level 3.";
		break;
	case 0b001001:
		dfsc->desc = "Access flag fault, level 1.";
		break;
	case 0b001010:
		dfsc->desc = "Access flag fault, level 2.";
		break;
	case 0b001011:
		dfsc->desc = "Access flag fault, level 3.";
		break;
	case 0b001000:
		dfsc->desc = "Access flag fault, level 0.";
		break;
	case 0b001100:
		dfsc->desc = "Permission fault, level 0.";
		break;
	case 0b001101:
		dfsc->desc = "Permission fault, level 1.";
		break;
	case 0b001110:
		dfsc->desc = "Permission fault, level 2.";
		break;
	case 0b001111:
		dfsc->desc = "Permission fault, level 3.";
		break;
	case 0b010000:
		dfsc->desc =
			"Synchronous External abort, not on translation table walk or hardware update of translation table.";
		break;
	case 0b010001:
		dfsc->desc = "Synchronous Tag Check Fault.";
		break;
	case 0b010011:
		dfsc->desc =
			"Synchronous External abort on translation table walk or hardware update of translation table, level -1.";
		break;
	case 0b010100:
		dfsc->desc =
			"Synchronous External abort on translation table walk or hardware update of translation table, level 0.";
		break;
	case 0b010101:
		dfsc->desc =
			"Synchronous External abort on translation table walk or hardware update of translation table, level 1.";
		break;
	case 0b010110:
		dfsc->desc =
			"Synchronous External abort on translation table walk or hardware update of translation table, level 2.";
		break;
	case 0b010111:
		dfsc->desc =
			"Synchronous External abort on translation table walk or hardware update of translation table, level 3.";
		break;
	case 0b011000:
		dfsc->desc =
			"Synchronous parity or ECC error on memory access, not on translation table walk.";
		break;
	case 0b011011:
		dfsc->desc =
			"Synchronous parity or ECC error on memory access on translation table walk or hardware update of translation table, level -1.";
		break;
	case 0b011100:
		dfsc->desc =
			"Synchronous parity or ECC error on memory access on translation table walk or hardware update of translation table, level 0.";
		break;
	case 0b011101:
		dfsc->desc =
			"Synchronous parity or ECC error on memory access on translation table walk or hardware update of translation table, level 1.";
		break;
	case 0b011110:
		dfsc->desc =
			"Synchronous parity or ECC error on memory access on translation table walk or hardware update of translation table, level 2.";
		break;

	case 0b011111:
		dfsc->desc =
			"Synchronous parity or ECC error on memory access on translation table walk or hardware update of translation table, level 3.";
		break;

	case 0b100001:
		dfsc->desc = "Alignment fault.";
		break;
	case 0b101001:
		dfsc->desc = "Address size fault, level -1.";
		break;
	case 0b101011:
		dfsc->desc = "Translation fault, level -1.";
		break;
	case 0b110000:
		dfsc->desc = "TLB conflict abort.";
		break;
	case 0b110001:
		dfsc->desc = "Unsupported atomic hardware update fault.";
		break;
	case 0b110100:
		dfsc->desc = "IMPLEMENTATION DEFINED fault (Lockdown).";
		break;
	case 0b110101:
		dfsc->desc =
			"IMPLEMENTATION DEFINED fault (Unsupported Exclusive or Atomic access).";
		break;
	default:
		dfsc->desc = "[ERROR]: INVALID dfsc!";
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

void check_res0(struct bitfield *res0)
{
	if (res0->value != 0) {
		res0->desc = "[ERROR]: Invalid RES0";
	} else {
		res0->desc = "Reserved";
	}
}

void decribe_cv(struct bitfield *cv)
{
	if (cv->value == 1) {
		cv->desc = "COND is valid";
	} else {
		cv->desc = "COND is not valid";
	}
}

void describe_rv(struct bitfield *rv)
{
	if (rv->value == 1) {
		rv->desc = "Register number is valid";
	} else {
		rv->desc = "Register number is not valid";
	}
}

void describe_ti(struct bitfield *ti)
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

void describe_cv(struct bitfield *cv)
{
	if (cv->value == 1) {
		cv->desc = "COND is valid";
	} else {
		cv->desc = "COND is not valid";
	}
}

void describe_offset(struct bitfield *offset)
{
	if (offset->value) {
		offset->desc = "Add offset";
	} else {
		offset->desc = "Subtract offset";
	}
}

void describe_mcr_direction(struct bitfield *direction)
{
	if (direction->value == 1) {
		direction->desc = "Read from system register (MRC or VMRS)";
	} else {
		direction->desc = "Write to system reigster (MCR)";
	}
}

void describe_ldc_direction(struct bitfield *direction)
{
	if (direction->value == 1) {
		direction->desc = "Read from memory (LDC)";
	} else {
		direction->desc = "Write to memory (STC)";
	}
}

void describe_am(struct bitfield *am)
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

void describe_iss_ld64b(struct bitfield *iss)
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

void decode_iss_data_abort(struct bitfield *iss)
{
	struct bitfield dfsc;

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
		bitfield_describe("RES0", "Reserved", 14, 23, check_res0);
	}

	bitfield_describe("VNCR", NULL, 13, 13, NULL);

	bitfield_new(iss->value, "DFSC", "Data Faule Status Code", 0, 5,
		     describe_dfsc, &dfsc);

	if (dfsc.value == 0b010000) {
		bitfield_describe("SET", "Synchronous Error Type", 11, 12,
				  describe_set);
	} else {
		bitfield_describe("RES1", "Reserved", 11, 12, NULL);
	}

	bitfield_describe("FnV", "FAR not Valid", 10, 10, describe_fnv);
	bitfield_describe("EA", "External Abort type", 9, 9, NULL);
	bitfield_describe("CM", "Cache Maintenance", 8, 8, NULL);
	bitfield_describe("S1PTW", "Stage-1 translation table walk", 7, 7,
			  NULL);
	bitfield_describe("WnR", "Write not Read", 6, 6, describe_wnr);
	bitfield_print(&dfsc);
}

void decode_iss_res0(struct bitfield *iss)
{
	bitfield_describe("RES0", "Reserved", 0, 24, check_res0);
}

void decode_iss_wf(struct bitfield *iss)
{
	bitfield_describe("CV", "Condition code valid", 24, 24, decribe_cv);
	bitfield_describe("COND", "Condition code of the trapped instruction",
			  20, 23, NULL);
	bitfield_describe("RES0", "Reserved", 10, 19, check_res0);
	bitfield_describe("RN", "Register Number", 5, 9, NULL);
	bitfield_describe("RES0", "Reserved", 3, 4, check_res0);
	bitfield_describe("RV", "Register valid", 2, 2, describe_rv);
	bitfield_describe("TI", "Trapped Instruction", 0, 1, describe_ti);
}

void decode_iss_mcr(struct bitfield *iss)
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

void decode_iss_mcrr(struct bitfield *iss)
{
	bitfield_describe("CV", "Condition code valid", 24, 24, describe_cv);
	bitfield_describe("COND", "Condition code of the trapped instruction",
			  20, 23, NULL);
	bitfield_describe("Opc1", NULL, 16, 19, NULL);
	bitfield_describe("Res0", NULL, 15, 15, check_res0);
	bitfield_describe("Rt2", NULL, 10, 14, NULL);
	bitfield_describe("Rt", NULL, 5, 9, NULL);
	bitfield_describe("CRm", NULL, 1, 4, NULL);
	bitfield_describe("Dir", "Direction of the trapped instruction", 0, 0,
			  describe_mcr_direction);
}

void decode_iss_ldc(struct bitfield *iss)
{
	bitfield_describe("CV", "Condition code valid", 24, 24, describe_cv);
	bitfield_describe("COND", "Condition code of the trapped instruction",
			  20, 23, NULL);
	bitfield_describe("imm8", "Immediate value of the trapped instruction",
			  12, 19, NULL);
	bitfield_describe("RES0", "Reserved", 10, 11, check_res0);
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

void decode_iss_sve(struct bitfield *iss)
{
	bitfield_describe("CV", "Condition code valid", 24, 24, describe_cv);
	bitfield_describe("COND", "Condition code of the trapped instruction",
			  20, 23, NULL);
	bitfield_describe("RES0", "Reserved", 0, 19, check_res0);
}

void decode_iss_ld64b(struct bitfield *iss)
{
	bitfield_describe("ISS", NULL, 0, 24, describe_iss_ld64b);
}

void decode_iss_bti(struct bitfield *iss)
{
	bitfield_describe("RES0", "Reserved", 2, 24, check_res0);
	bitfield_describe("BTYPE", "PSTATE.BTYPE value", 0, 1, NULL);
}

void decode_iss_hvc(struct bitfield *iss)
{
	bitfield_describe("RES0", "Reserved", 16, 24, check_res0);
	bitfield_describe("imm16", "Value of the immediate field", 0, 15, NULL);
}

void decode_iss_default(struct bitfield *iss)
{
	iss->desc = "[ERROR]: bad iss";
	bitfield_print(iss);
}

decode_iss_fn decode_ec()
{
	struct bitfield ec;
	decode_iss_fn iss_decoder = decode_iss_default;

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
	case 0b100101:
		ec.desc =
			"Data Abort taken without a change in Exception level";
		iss_decoder = decode_iss_data_abort;
		break;
	default:
		ec.desc = "[ERROR]: bad ec";
		break;
	}

	bitfield_print(&ec);

	return iss_decoder;
}

void decode(u64 esr)
{
	struct bitfield res0;
	struct bitfield iss2;
	struct bitfield iss;

	_esr = esr;

	bitfield_describe("RES0", "Reserved", 37, 63, check_res0);
	bitfield_describe("ISS2", "Instruction Specific Syndrome 2", 32, 36,
			  NULL);

	decode_iss_fn iss_decoder = decode_ec();

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
