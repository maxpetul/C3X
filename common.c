
// common.c: Stores various utility functions available to both the injected code (injected_code.c) and the patcher (ep.c). Also includes everything
// related to text parsing (in fact most of this file is related to parsing).

int
not_below (int lim, int x)
{
	return (x >= lim) ? x : lim;
}

int
not_above (int lim, int x)
{
	return (x <= lim) ? x : lim;
}

int
clamp (int lower_lim, int upper_lim, int x)
{
	if (x >= lower_lim) {
		if (x <= upper_lim)
			return x;
		else
			return upper_lim;
	} else
		return lower_lim;
}

int
square (int x)
{
	return x * x;
}

int
int_abs (int x)
{
	return (x >= 0) ? x : (0 - x);
}

void
free_perfume_config_specs (struct perfume_config_spec * specs, int count_specs)
{
	for (int n = 0; n < count_specs; n++)
		free (specs[n].target_name);
	free (specs);
}

// Writes an integer to a byte buffer. buf need not be aligned but it must have at least four bytes of free space. Written little-endian.
byte *
int_to_bytes (byte * buf, int x)
{
	buf[0] =  x        & 0xFF;
	buf[1] = (x >>  8) & 0xFF;
	buf[2] = (x >> 16) & 0xFF;
	buf[3] = (x >> 24) & 0xFF;
	return buf + 4;
}

// Read a little-endian int from a byte buffer
int
int_from_bytes (byte * buf)
{
	int lo = buf[0] | ((int)buf[1] << 8);
	int hi = buf[2] | ((int)buf[3] << 8);
	return (hi << 16) | lo;
}

// Ensures there's at least one open spot at the end of a dynamic array, reallocating it if necessary.
void
reserve (int item_size, void ** p_items, int * p_capacity, int count)
{
	if (count >= *p_capacity) {
		int new_capacity = not_below (10, *p_capacity * 2);
		void * new_list = malloc (new_capacity * item_size);
		if (count > 0)
			memcpy (new_list, *p_items, item_size * count);
		free (*p_items);
		*p_items = new_list;
		*p_capacity = new_capacity;
	}
}

// =============================================================
// ||                                                         ||
// ||     FUNCTIONS BELOW ARE ALL RELATED TO TEXT PARSING     ||
// ||                                                         ||
// =============================================================

struct string_slice {
	char * str;
	int len;
};

int
is_horiz_space (char c)
{
	return ((c == ' ') || (c == '\t') || (c == '\r'));
}

int
is_alpha_num (char c)
{
	return (c == '_') || ((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')) || ((c >= '0') && (c <= '9'));
}

int
skip_horiz_space (char ** p_cursor)
{
	char * cur = *p_cursor;
	while (is_horiz_space (*cur))
		cur++;
	*p_cursor = cur;
	return 1;
}

int
skip_to_line_end (char ** p_cursor)
{
	char * cur = *p_cursor;
	while ((*cur != '\n') && (*cur != '\0'))
		cur++;
	*p_cursor = cur;
	return 1;
}

int
skip_line (char ** p_cursor)
{
	skip_to_line_end (p_cursor);
	if (**p_cursor == '\n')
		*p_cursor += 1;
	return 1;
}

int
skip_punctuation (char ** p_cursor, char c)
{
	char * cur = *p_cursor;
	skip_horiz_space (&cur);
	if (*cur == c) {
		*p_cursor = cur + 1;
		return 1;
	} else
		return 0;
}

struct string_slice
trim_string_slice (struct string_slice const * s, int remove_quotes)
{
	char * str = s->str;
	int len = s->len;

	while ((len > 0) && (is_horiz_space (*str))) {
		str++;
		len--;
	}
	while ((len > 0) && (is_horiz_space (str[len-1])))
		len--;
	if (remove_quotes &&
	    (len >= 2) &&
	    ((*str == '"') && (str[len-1] == '"'))) {
		str++;
		len -= 2;
	}

	return (struct string_slice) { .str = str, .len = len };
}

char *
extract_slice (struct string_slice const * s)
{
	char * tr = malloc (s->len + 1);
	for (int n = 0; n < s->len; n++)
		tr[n] = s->str[n];
	tr[s->len] = '\0';
	return tr;
}

int
read_int (struct string_slice const * s, int * out_val)
{
	struct string_slice trimmed = trim_string_slice (s, 1);
	char * str = trimmed.str;
	int len = trimmed.len;

	if ((len > 0) && (*str == '-') || ((*str >= '0') && (*str <= '9'))) {
		char * end;
		int base = 10;
		if ((str[0] == '0') && ((str[1] == 'x') || (str[1] == 'X'))) {
			base = 16;
			str += 2;
			len -= 2;
		}
		int res = strtol (str, &end, base);
		if (end == str + len) {
			*out_val = res;
			return 1;
		} else
			return 0;
	} else if ((len == 4) &&
		   ((0 == strncmp (str, "true", 4)) ||
		    (0 == strncmp (str, "True", 4)) ||
		    (0 == strncmp (str, "TRUE", 4)))) {
		*out_val = 1;
		return 1;
	} else if ((len == 5) &&
		   ((0 == strncmp (str, "false", 5)) ||
		    (0 == strncmp (str, "False", 5)) ||
		    (0 == strncmp (str, "FALSE", 5)))) {
		*out_val = 0;
		return 1;
	} else
		return 0;
}

int
parse_string (char ** p_cursor, struct string_slice * out)
{
	char * cur = *p_cursor;
	skip_horiz_space (&cur);
	int quoted = *cur == '"';
	char * str_start;
	if (quoted) {
		cur++;
		str_start = cur;
		while ((*cur != '"') && (*cur != '\0'))
			cur++;
	} else {
		str_start = cur;
		while (is_alpha_num (*cur) || (*cur == '-') || (*cur == '.'))
			cur++;
	}
	int str_len = cur - str_start;
	if ((str_len == 0) && (! quoted))
		return 0;
	out->str = str_start;
	out->len = str_len;
	*p_cursor = cur + (quoted && (*cur == '"'));
	return 1;
}

int
parse_perfume_spec (char ** p_cursor, struct perfume_config_spec * out)
{
	char * cur = *p_cursor;
	struct string_slice name, amount_str;
	int amount;
	if (parse_string (&cur, &name) &&
	    skip_punctuation (&cur, ':') &&
	    parse_string (&cur, &amount_str) &&
	    read_int (&amount_str, &amount)) {
		out->target_name = extract_slice (&name);
		out->amount = amount;
		*p_cursor = cur;
		return 1;
	} else
		return 0;
}

int
read_perfume_specs (struct string_slice const * s, struct perfume_config_spec ** out_perfume_specs, int * out_count_perfume_specs)
{
	if (s->len <= 0) {
		*out_perfume_specs = NULL;
		*out_count_perfume_specs = 0;
		return 1;
	}
	char * extracted_slice = extract_slice (s);
	char * cursor = extracted_slice;

	int success = 0;
	struct perfume_config_spec * specs = NULL;
	int count_specs = 0;
	int specs_capacity = 0;

	while (1) {
		struct perfume_config_spec temp_spec;
		if (parse_perfume_spec (&cursor, &temp_spec)) {
			reserve (sizeof specs[0], (void **)&specs, &specs_capacity, count_specs);
			specs[count_specs++] = temp_spec;

			if (skip_punctuation (&cursor, ','))
				continue;
			else if (skip_horiz_space (&cursor) && (*cursor == '\0')) {
				success = 1;
				break;
			} else
				break;
		} else
			break;
	}

	if (success) {
		*out_perfume_specs = specs;
		*out_count_perfume_specs = count_specs;
	} else
		free_perfume_config_specs (specs, count_specs);
	free (extracted_slice);
	return success;
}

int
parse_bracketed_block (char ** p_cursor, struct string_slice * out)
{
	char * cur = *p_cursor;
	struct string_slice unused;
	if (skip_punctuation (&cur, '[')) {
		char * str_start = cur;
		while (1) {
			if (*cur == ']') {
				cur++;
				break;
			} else if (*cur == '"') {
				if (! parse_string (&cur, &unused))
					return 0;
			} else if (*cur == '\0')
				return 0;
			else
				cur++;
		}
		out->str = str_start;
		out->len = cur - str_start - 1;
		*p_cursor = cur;
		return 1;
	} else
		return 0;
}

int
parse_key_value_pair (char ** p_cursor, struct string_slice * out_key, struct string_slice * out_value)
{
	char * cur = *p_cursor;
	struct string_slice key, value;
	if (parse_string (&cur, &key) &&
	    skip_punctuation (&cur, '=') &&
	    (parse_string (&cur, &value) ||
	     parse_bracketed_block (&cur, &value))) {
		*out_key = key;
		*out_value = value;
		skip_to_line_end (&cur);
		*p_cursor = cur;
		return 1;
	} else
		return 0;
}

int
parse_csv_value (char ** p_cursor, char ** out_val, int * out_len)
{
	char * tr_val = *p_cursor,
	     * cursor = *p_cursor;
	int quoted = 0;
	while (1) {
		char c = *cursor;
		if ((! quoted) && (c == ','))
			break;
		else if ((c == '\n') || (c == '\0'))
			break;
		else if (quoted && (c == '\\') && (cursor[1] == '"'))
			cursor += 2;
		else if (c == '"') {
			quoted = ! quoted;
			cursor++;
		} else
			cursor++;
	}
	*p_cursor = cursor;
	*out_val = tr_val;
	*out_len = cursor - tr_val;
	return 1;
}

int
read_object_job (struct string_slice const * s, enum object_job * out)
{
	struct string_slice trimmed = trim_string_slice (s, 1);

	if      (0 == strncmp ("define"   , trimmed.str, trimmed.len)) *out = OJ_DEFINE;
	else if (0 == strncmp ("inlead"   , trimmed.str, trimmed.len)) *out = OJ_INLEAD;
	else if (0 == strncmp ("repl vptr", trimmed.str, trimmed.len)) *out = OJ_REPL_VPTR;
	else if (0 == strncmp ("repl call", trimmed.str, trimmed.len)) *out = OJ_REPL_CALL;
	else if (0 == strncmp ("ignore"   , trimmed.str, trimmed.len)) *out = OJ_IGNORE;
	else
		return 0;
	return 1;
}

char *
trim_and_extract_slice (struct string_slice const * s, int remove_quotes)
{
	struct string_slice trimmed = trim_string_slice (s, remove_quotes);
	return extract_slice (&trimmed);
}

int
parse_civ_prog_object (char ** p_cursor, struct civ_prog_object * out)
{
	char * cursor = *p_cursor;
	struct string_slice columns[5];
	for (int n = 0; n < 5; n++) {
		if (parse_csv_value (&cursor, &columns[n].str, &columns[n].len)) {
			char c = *cursor; // Check character after value for errors & to skip if necessary
			if ((n < 4) && (c == ',')) // Before the last column, the value must end in a comma. Skip it if it's present.
				cursor++;
			else if ((n == 4) && (c == '\n')) // If the last column ends in a new line, skip it.
				cursor++;
			else if ((n == 4) && (c == '\0')) // If it ends in a null terminator that's OK but don't skip it.
				;
			else // Anything else is an error
				return 0;
		} else
			return 0;
	}

	struct civ_prog_object tr;
	if (read_object_job (&columns[0], &tr.job) &&
	    read_int (&columns[1], &tr.gog_addr) &&
	    read_int (&columns[2], &tr.steam_addr)) {
		tr.name = trim_and_extract_slice (&columns[3], 1);
		tr.type = trim_and_extract_slice (&columns[4], 1);
		*out = tr;
		*p_cursor = cursor;
		return 1;
	} else
		return 0;

}
