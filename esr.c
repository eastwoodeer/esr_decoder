#include <stdio.h>
#include <string.h>

typedef unsigned long u64;

struct field {
	char *name;
	char *long_name;
	size_t start;
	size_t width;
	u64 value;
	char *desc;
	size_t subfields_length;
	char *indent;
	struct field *subfields[32];
};

u64 get_bits(u64 reg, size_t start, size_t end)
{
	u64 width = end - start + 1;
	return (reg >> start) & ((1 << width) - 1);
}

typedef void (*describe_fn)(struct field *);

void create_field_info(u64 reg, char *name, char *long_name, size_t start,
		       size_t end, describe_fn fn, struct field *field)
{
	memset(field, 0, sizeof(struct field));
	field->name = name;
	field->long_name = long_name;
	field->start = start;
	field->width = end - start + 1;
	field->value = get_bits(reg, start, end);
	field->desc = NULL;
	field->indent = "";
	field->subfields_length = 0;

	if (fn != NULL) {
		fn(field);
	}
}

void field_add_subfield(struct field *field, struct field *subfield)
{
	field->subfields[field->subfields_length++] = subfield;
}

void print_field_info(struct field *field)
{
	if (field->width == 1) {
		printf("%s%02ld\t", field->indent, field->start);
		printf("%s%s:\t%s", field->indent, field->name,
		       field->value == 1 ? "true" : "false");
	} else {
		printf("%s%02ld...%02ld\t", field->indent, field->start,
		       field->start + field->width - 1);
		printf("%s%s:\t0x%02lx 0b%08lb", field->indent, field->name,
		       field->value, field->value);
	}
	if (field->desc) {
		printf("\t# %s", field->desc);
	}
	printf("\n");

	if (field->subfields_length != 0) {
		for (int i = 0; i < field->subfields_length; i++) {
			field->subfields[i]->indent = "\t";
			print_field_info(field->subfields[i]);
		}
	}
}

void describe_il(struct field *field)
{
	if (field->value == 1) {
		field->desc = "32-bit instruction trapped";
	} else {
		field->desc = "16-bit instruction trapped";
	}
}

void describe_sas(struct field *sas)
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

void describe_ar(struct field *ar)
{
	if (ar->value == 1) {
		ar->desc = "Acquire/Release semantics";
	} else {
		ar->desc = "No Acquire/Release semantics";
	}
}

void describe_fnv(struct field *fnv)
{
	if (fnv->value == 1) {
		fnv->desc = "FAR is not valid, it holds an unknown value";
	} else {
		fnv->desc = "Far is valid";
	}
}

void describe_wnr(struct field *wnr)
{
	if (wnr->value == 1) {
		wnr->desc = "Abort caused by writing to memory";
	} else {
		wnr->desc = "Abort caused by reading from memory";
	}
}

void describe_dfsc(struct field *dfsc)
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

void describe_set(struct field *set)
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

void decode_iss_data_abort(struct field *iss)
{
	struct field isv;
	struct field sas;
	struct field sse;
	struct field srt;
	struct field sf;
	struct field ar;
	struct field res0;
	struct field vncr;
	struct field fnv;
	struct field ea;
	struct field cm;
	struct field s1ptw;
	struct field wnr;
	struct field dfsc;
	struct field set;
	struct field res1;

	create_field_info(iss->value, "ISV", "Instruction Syndrome Valid", 24,
			  24, NULL, &isv);
	field_add_subfield(iss, &isv);

	if (isv.value == 1) {
		create_field_info(iss->value, "SAS", "Syndrome Access Size", 22,
				  23, describe_sas, &sas);
		field_add_subfield(iss, &sas);

		create_field_info(iss->value, "SSE", "Syndrome Sign Extend", 21,
				  21, NULL, &sse);
		field_add_subfield(iss, &sse);

		create_field_info(iss->value, "SRT",
				  "Syndrome Register Transfer", 16, 20, NULL,
				  &srt);
		field_add_subfield(iss, &srt);

		create_field_info(iss->value, "SF", "Sixty-Four", 15, 15, NULL,
				  &sf);
		field_add_subfield(iss, &sf);

		create_field_info(iss->value, "AR", "Acquire/Release", 14, 14,
				  describe_ar, &ar);
		field_add_subfield(iss, &ar);
	} else {
		create_field_info(iss->value, "RES0", "Reserved", 14, 23, NULL,
				  &res0);
		field_add_subfield(iss, &res0);
	}

	create_field_info(iss->value, "VNCR", "", 13, 13, NULL, &vncr);
	field_add_subfield(iss, &vncr);

	create_field_info(iss->value, "FnV", "FAR not Valid", 10, 10,
			  describe_fnv, &fnv);
	create_field_info(iss->value, "EA", "External Abort type", 13, 13, NULL,
			  &ea);
	create_field_info(iss->value, "CM", "Cache Maintenance", 8, 8, NULL,
			  &cm);
	create_field_info(iss->value, "S1PTW", "Stage-1 translation table walk",
			  7, 7, NULL, &s1ptw);
	create_field_info(iss->value, "WnR", "Write not Read", 6, 6,
			  describe_wnr, &wnr);
	create_field_info(iss->value, "DFSC", "Data Faule Status Code", 0, 5,
			  describe_dfsc, &dfsc);

	if (dfsc.value == 0b010000) {
		create_field_info(iss->value, "SET", "Synchronous Error Type",
				  11, 12, describe_set, &set);
		field_add_subfield(iss, &set);
	} else {
		create_field_info(iss->value, "RES1", "Reserved", 11, 12, NULL,
				  &res1);
		field_add_subfield(iss, &res1);
	}
	field_add_subfield(iss, &fnv);
	field_add_subfield(iss, &ea);
	field_add_subfield(iss, &cm);
	field_add_subfield(iss, &s1ptw);
	field_add_subfield(iss, &wnr);
	field_add_subfield(iss, &dfsc);

	print_field_info(iss);
}

void decode_iss_res0(struct field *iss)
{
}

void decode_ec(struct field *ec, struct field *iss)
{
	print_field_info(ec);
	switch (ec->value) {
	case 0b000000:
		ec->desc = "Unknown reason";
		decode_iss_res0(iss);
		break;
	case 0b100101:
		ec->desc =
			"Data Abort taken without a change in Exception level";
		decode_iss_data_abort(iss);
		break;
	}
}

void decode(u64 esr)
{
	struct field res0;
	struct field iss2;
	struct field ec;
	struct field il;
	struct field iss;

	create_field_info(esr, "RES0", "Instruction Specific Syndrome", 37, 63,
			  NULL, &res0);
	create_field_info(esr, "ISS2", "Instruction Specific Syndrome 2", 32,
			  36, NULL, &iss2);
	create_field_info(esr, "EC", "Exception Class", 26, 31, NULL, &ec);
	create_field_info(esr, "IL", "Instruction Length", 25, 25, describe_il,
			  &il);
	create_field_info(esr, "ISS", "Instruction Specific Syndrome", 0, 24,
			  NULL, &iss);
	print_field_info(&res0);
	print_field_info(&iss2);
	print_field_info(&ec);
	print_field_info(&il);

	decode_ec(&ec, &iss);
}

int main(int argc, char *argv[])
{
	decode(0x96000050);
	return 0;
}
