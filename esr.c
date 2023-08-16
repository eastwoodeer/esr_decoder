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

void decimal_to_binary(u64 n, char *buf)
{
	size_t len = sizeof(u64) * 8 - 1;
	int ignore = 1;
	int i = len;
	int j = 0;

	if (n == 0) {
		for (; j < 8; j++) {
			buf[j] = '0';
		}
		buf[j] = '\0';
		return;
	}

	for (; i >= 0; i--) {
		int k = (n >> i);
		if (ignore && (!k)) {
			continue;
		}

		ignore = 0;
		buf[j++] = (k & 1) ? '1' : '0';
	}
	buf[j] = '\0';
}

void field_description(struct bitfield *field)
{
	char binary[128];

	if (field->width == 1) {
		printf("%s%02ld\t", field->indent, field->start);
		printf("%s%s:\t%s", field->indent, field->name,
		       field->value == 1 ? "true" : "false");
	} else {
		printf("%s%02ld...%02ld\t", field->indent, field->start,
		       field->start + field->width - 1);
		decimal_to_binary(field->value, binary);
		printf("%s%s:\t0x%02lx 0b%s", field->indent, field->name,
		       field->value, binary);
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

void bitfield_describe(char *name, char *long_name, size_t start, size_t end,
		       describe_fn desc)
{
	struct bitfield field;
	bitfield_new(_esr, name, long_name, start, end, desc, &field);
	bitfield_print(&field);
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
		fnv->desc = "Far is valid";
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

void describe_direction(struct bitfield *direction)
{
	if (direction->value == 1) {
		direction->desc = "Read from system register (MRC or VMRS)";
	} else {
		direction->desc = "Write to system reigster (MCR)";
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

void decode_iss_data_abort(struct bitfield *iss)
{
	struct bitfield isv;
	struct bitfield sas;
	struct bitfield sse;
	struct bitfield srt;
	struct bitfield sf;
	struct bitfield ar;
	struct bitfield res0;
	struct bitfield vncr;
	struct bitfield fnv;
	struct bitfield ea;
	struct bitfield cm;
	struct bitfield s1ptw;
	struct bitfield wnr;
	struct bitfield dfsc;
	struct bitfield set;
	struct bitfield res1;

	bitfield_new(iss->value, "ISV", "Instruction Syndrome Valid", 24, 24,
		     NULL, &isv);
	field_append(iss, &isv);

	if (isv.value == 1) {
		bitfield_new(iss->value, "SAS", "Syndrome Access Size", 22, 23,
			     describe_sas, &sas);
		field_append(iss, &sas);

		bitfield_new(iss->value, "SSE", "Syndrome Sign Extend", 21, 21,
			     NULL, &sse);
		field_append(iss, &sse);

		bitfield_new(iss->value, "SRT", "Syndrome Register Transfer",
			     16, 20, NULL, &srt);
		field_append(iss, &srt);

		bitfield_new(iss->value, "SF", "Sixty-Four", 15, 15, NULL, &sf);
		field_append(iss, &sf);

		bitfield_new(iss->value, "AR", "Acquire/Release", 14, 14,
			     describe_ar, &ar);
		field_append(iss, &ar);
	} else {
		bitfield_new(iss->value, "RES0", "Reserved", 14, 23, check_res0,
			     &res0);
		field_append(iss, &res0);
	}

	bitfield_new(iss->value, "VNCR", "", 13, 13, NULL, &vncr);
	field_append(iss, &vncr);

	bitfield_new(iss->value, "FnV", "FAR not Valid", 10, 10, describe_fnv,
		     &fnv);
	bitfield_new(iss->value, "EA", "External Abort type", 9, 9, NULL, &ea);
	bitfield_new(iss->value, "CM", "Cache Maintenance", 8, 8, NULL, &cm);
	bitfield_new(iss->value, "S1PTW", "Stage-1 translation table walk", 7,
		     7, NULL, &s1ptw);
	bitfield_new(iss->value, "WnR", "Write not Read", 6, 6, describe_wnr,
		     &wnr);
	bitfield_new(iss->value, "DFSC", "Data Faule Status Code", 0, 5,
		     describe_dfsc, &dfsc);

	if (dfsc.value == 0b010000) {
		bitfield_new(iss->value, "SET", "Synchronous Error Type", 11,
			     12, describe_set, &set);
		field_append(iss, &set);
	} else {
		bitfield_new(iss->value, "RES1", "Reserved", 11, 12, NULL,
			     &res1);
		field_append(iss, &res1);
	}
	field_append(iss, &fnv);
	field_append(iss, &ea);
	field_append(iss, &cm);
	field_append(iss, &s1ptw);
	field_append(iss, &wnr);
	field_append(iss, &dfsc);

	bitfield_print(iss);
}

void decode_iss_res0(struct bitfield *iss)
{
	struct bitfield res0;
	bitfield_new(iss->value, "RES0", "Reserved", 0, 24, check_res0, &res0);
	field_append(iss, &res0);
	bitfield_print(iss);
}

void decode_iss_wf(struct bitfield *iss)
{
	struct bitfield cv;
	struct bitfield cond;
	struct bitfield res0a;
	struct bitfield rn;
	struct bitfield res0b;
	struct bitfield rv;
	struct bitfield ti;

	bitfield_new(iss->value, "CV", "Condition code valid", 24, 24,
		     decribe_cv, &cv);
	bitfield_new(iss->value, "COND",
		     "Condition code of the trapped instruction", 20, 23, NULL,
		     &cond);
	bitfield_new(iss->value, "RES0", "Reserved", 10, 19, check_res0,
		     &res0a);
	bitfield_new(iss->value, "RN", "Register Number", 5, 9, NULL, &rn);
	bitfield_new(iss->value, "RES0", "Reserved", 3, 4, check_res0, &res0b);
	bitfield_new(iss->value, "RV", "Register valid", 2, 2, describe_rv,
		     &rv);
	bitfield_new(iss->value, "TI", "Trapped Instruction", 0, 1, describe_ti,
		     &ti);

	field_append(iss, &cv);
	field_append(iss, &cond);
	field_append(iss, &res0a);
	field_append(iss, &rn);
	field_append(iss, &res0b);
	field_append(iss, &rv);
	field_append(iss, &ti);

	bitfield_print(iss);
}

void decode_iss_mcr(struct bitfield *iss)
{
	struct bitfield cv;
	struct bitfield cond;
	struct bitfield opc2;
	struct bitfield opc1;
	struct bitfield crn;
	struct bitfield rt;
	struct bitfield crm;
	struct bitfield direction;

	bitfield_new(iss->value, "CV", "Condition code valid", 24, 24,
		     decribe_cv, &cv);
	bitfield_new(iss->value, "COND",
		     "Condition code of the trapped instruction", 20, 23, NULL,
		     &cond);
	bitfield_new(iss->value, "Opc2", NULL, 17, 19, NULL, &opc2);
	bitfield_new(iss->value, "Opc1", NULL, 14, 17, NULL, &opc1);
	bitfield_new(iss->value, "Crn", NULL, 10, 13, NULL, &crn);
	bitfield_new(iss->value, "Rt", NULL, 5, 9, NULL, &rt);
	bitfield_new(iss->value, "CRm", NULL, 1, 4, NULL, &crm);
	bitfield_new(iss->value, "Direction",
		     "Direction of the trapped instruction", 0, 0,
		     describe_direction, &direction);

	field_append(iss, &cv);
	field_append(iss, &cond);
	field_append(iss, &opc2);
	field_append(iss, &opc1);
	field_append(iss, &crn);
	field_append(iss, &rt);
	field_append(iss, &crm);
	field_append(iss, &direction);

	bitfield_print(iss);
}

void decode_iss_mcrr(struct bitfield *iss)
{
	struct bitfield cv;
	struct bitfield cond;
	struct bitfield opc1;
	struct bitfield res0;
	struct bitfield rt2;
	struct bitfield rt;
	struct bitfield crm;
	struct bitfield direction;

	bitfield_new(iss->value, "CV", "Condition code valid", 24, 24,
		     describe_cv, &cv);
	bitfield_new(iss->value, "COND",
		     "Condition code of the trapped instruction", 20, 23, NULL,
		     &cond);
	bitfield_new(iss->value, "Opc1", NULL, 16, 19, NULL, &opc1);
	bitfield_new(iss->value, "Res0", NULL, 15, 15, check_res0, &res0);
	bitfield_new(iss->value, "Rt2", NULL, 10, 14, NULL, &rt2);
	bitfield_new(iss->value, "Rt", NULL, 5, 9, NULL, &rt);
	bitfield_new(iss->value, "CRm", NULL, 1, 4, NULL, &crm);
	bitfield_new(iss->value, "Direction",
		     "Direction of the trapped instruction", 0, 0,
		     describe_direction, &direction);

	field_append(iss, &cv);
	field_append(iss, &cond);
	field_append(iss, &opc1);
	field_append(iss, &res0);
	field_append(iss, &rt2);
	field_append(iss, &rt);
	field_append(iss, &crm);
	field_append(iss, &direction);

	bitfield_print(iss);
}

void decode_iss_ldc(struct bitfield *iss)
{
	struct bitfield cv;
	struct bitfield cond;
	struct bitfield imm8;
	struct bitfield res0;
	struct bitfield rn;
	struct bitfield offset;
	struct bitfield am;
	struct bitfield direction;

	bitfield_new(iss->value, "CV", "Condition code valid", 24, 24,
		     describe_cv, &cv);
	bitfield_new(iss->value, "COND",
		     "Condition code of the trapped instruction", 20, 23, NULL,
		     &cond);
	bitfield_new(iss->value, "imm8",
		     "Immediate value of the trapped instruction", 12, 19, NULL,
		     &imm8);
	bitfield_new(iss->value, "RES0", "Reserved", 10, 11, check_res0, &res0);
	bitfield_new(
		iss->value, "Rn",
		"General-purpose register number of the trapped instruction", 5,
		9, NULL, &rn);
	bitfield_new(iss->value, "Offset",
		     "Whether the offset is added or substracted", 4, 4, NULL,
		     &offset);
	bitfield_new(iss->value, "AM", "Addressing Mode", 1, 3, describe_am,
		     &am);
	bitfield_new(iss->value, "Direction",
		     "Direction of the trapped instruction", 0, 0,
		     describe_direction, &direction);

	field_append(iss, &cv);
	field_append(iss, &cond);
	field_append(iss, &imm8);
	field_append(iss, &res0);
	field_append(iss, &rn);
	field_append(iss, &offset);
	field_append(iss, &am);
	field_append(iss, &direction);

	bitfield_print(iss);
}

void decode_iss_sve(struct bitfield *iss)
{
	struct bitfield cv;
	struct bitfield cond;
	struct bitfield res0;

	bitfield_new(iss->value, "CV", "Condition code valid", 24, 24,
		     describe_cv, &cv);
	bitfield_new(iss->value, "COND",
		     "Condition code of the trapped instruction", 20, 23, NULL,
		     &cond);
	bitfield_new(iss->value, "RES0", "Reserved", 0, 19, check_res0, &res0);

	field_append(iss, &cv);
	field_append(iss, &cond);
	field_append(iss, &res0);

	bitfield_print(iss);
}

void decode_iss_default(struct bitfield *iss)
{
	iss->desc = "[ERROR]: bad iss";
	bitfield_print(iss);
}

decode_iss_fn decode_ec()
{
	struct bitfield ec;

	bitfield_new(_esr, "EC", "Exception Class", 26, 31, NULL, &ec);

	decode_iss_fn iss_decoder;

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
	case 0b00111:
		ec.desc =
			"Trapped access to SVE, Advanced SIMD or floating point";
		iss_decoder = decode_iss_sve;
		break;
	case 0b100101:
		ec.desc =
			"Data Abort taken without a change in Exception level";
		iss_decoder = decode_iss_data_abort;
		break;
	default:
		ec.desc = "[ERROR]: bad ec";
		iss_decoder = decode_iss_default;
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

	bitfield_new(esr, "ISS", "Instruction Specific Syndrome", 0, 24, NULL,
		     &iss);
	iss_decoder(&iss);
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
