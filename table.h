// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 SASANO Takayoshi <uaa@uaa.org.uk>

struct morse_table {
	const char *code;
	const char *result;
};

extern const struct morse_table table_en[];
extern const int table_en_entry;

extern const struct morse_table table_jp[];
extern const int table_jp_entry;

const char *decode_code(const struct morse_table *, char *);
