
int
is_horiz_space (char c)
{
	return ((c == ' ') || (c == '\t') || (c == '\r'));
}

void
skip_horiz_space (char ** p_cursor)
{
	char * cur = *p_cursor;
	while (is_horiz_space (*cur))
		cur++;
	*p_cursor = cur;
}

void
skip_to_line_end (char ** p_cursor)
{
	char * cur = *p_cursor;
	while ((*cur != '\n') && (*cur != '\0'))
		cur++;
	*p_cursor = cur;
}

void
skip_line (char ** p_cursor)
{
	skip_to_line_end (p_cursor);
	if (**p_cursor == '\n')
		*p_cursor += 1;
}

void
trim_string_slice (char ** str, int * str_len, int remove_quotes)
{
	while ((*str_len > 0) && (is_horiz_space (**str))) {
		(*str)++;
		(*str_len)--;
	}
	while ((*str_len > 0) && (is_horiz_space ((*str)[*str_len-1])))
		(*str_len)--;
	if (remove_quotes &&
	    (*str_len >= 2) &&
	    ((**str == '"') && ((*str)[*str_len-1] == '"'))) {
		(*str)++;
		(*str_len) -= 2;
	}
}

char *
extract_slice (char * str, int str_len)
{
	trim_string_slice (&str, &str_len, 1);
	char * tr = malloc (str_len + 1);
	for (int n = 0; n < str_len; n++)
		tr[n] = str[n];
	tr[str_len] = '\0';
	return tr;
}

int
read_int (char * str, int str_len, int * out_val)
{
	trim_string_slice (&str, &str_len, 1);
	if ((str_len > 0) && (*str == '-') || ((*str >= '0') && (*str <= '9'))) {
		char * end;
		int base = 10;
		if ((str[0] == '0') && ((str[1] == 'x') || (str[1] == 'X'))) {
			base = 16;
			str += 2;
			str_len -= 2;
		}
		int res = strtol (str, &end, base);
		if (end == str + str_len) {
			*out_val = res;
			return 1;
		} else
			return 0;
	} else if ((str_len == 4) &&
		   ((0 == strncmp (str, "true", 4)) ||
		    (0 == strncmp (str, "True", 4)) ||
		    (0 == strncmp (str, "TRUE", 4)))) {
		*out_val = 1;
		return 1;
	} else if ((str_len == 5) &&
		   ((0 == strncmp (str, "false", 5)) ||
		    (0 == strncmp (str, "False", 5)) ||
		    (0 == strncmp (str, "FALSE", 5)))) {
		*out_val = 0;
		return 1;
	} else
		return 0;
}

// What an ugly function. I hate writing parsers.
int
parse_key_value_pair (char ** p_cursor, char ** out_key, int * out_key_len, char ** out_value, int * out_value_len)
{
	char * cur = *p_cursor;
	char * key, * value;
	int key_len, value_len;
	key = cur;
	while (1) {
		char c = *cur;
		if ((c == '_') || ((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')) || ((c >= '0') && (c <= '9')))
			cur++;
		else
			break;
	}
	key_len = cur - key;
	skip_horiz_space (&cur);
	if (*cur != '=')
		return 0;
	cur++;
	value = cur;
	skip_to_line_end (&cur);
	value_len = cur - value;
	trim_string_slice (&value, &value_len, 1);
	if ((key_len <= 0) || (value_len <= 0))
		return 0;
	*out_key = key;
	*out_key_len = key_len;
	*out_value = value;
	*out_value_len = value_len;
	*p_cursor = cur;
	return 1;
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
read_object_job (char * str, int str_len, enum object_job * out)
{
	trim_string_slice (&str, &str_len, 1);
	if      (0 == strncmp ("define"   , str, str_len)) *out = OJ_DEFINE;
	else if (0 == strncmp ("inlead"   , str, str_len)) *out = OJ_INLEAD;
	else if (0 == strncmp ("repl vptr", str, str_len)) *out = OJ_REPL_VPTR;
	else if (0 == strncmp ("repl call", str, str_len)) *out = OJ_REPL_CALL;
	else if (0 == strncmp ("ignore"   , str, str_len)) *out = OJ_IGNORE;
	else
		return 0;
	return 1;
}

int
parse_civ_prog_object (char ** p_cursor, struct civ_prog_object * out)
{
	char * cursor = *p_cursor;
	char * vals[5];
	int lens[5];
	for (int n = 0; n < 5; n++) {
		if (parse_csv_value (&cursor, &vals[n], &lens[n])) {
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
	if (read_object_job (vals[0], lens[0], &tr.job) &&
	    read_int (vals[1], lens[1], &tr.gog_addr) &&
	    read_int (vals[2], lens[2], &tr.steam_addr)) {
		tr.name = extract_slice (vals[3], lens[3]);
		tr.type = extract_slice (vals[4], lens[4]);
		*out = tr;
		*p_cursor = cursor;
		return 1;
	} else
		return 0;

}
