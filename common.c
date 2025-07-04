
// common.c: Stores various utility functions available to both the injected code (injected_code.c) and the patcher (ep.c). Also includes most code
// related to text parsing, only the parsing funcs that depend on BIC data are put in injected_code.c instead.

#define ARRAY_LEN(a) ((sizeof a) / (sizeof a[0]))

struct string_slice {
	char * str;
	int len;
};

int
slice_matches_str (struct string_slice const * slice, char const * str)
{
	return (strncmp (str, slice->str, slice->len) == 0) && (str[slice->len] == '\0');
}

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

int
gcd (int a, int b)
{
	while (b != 0) {
		int t = b;
		b = a % b;
		a = t;
	}
	return a;
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

byte *
buffer_allocate (struct buffer * b, int size)
{
	size = not_below (0, size);
	if ((b->contents != NULL) && (b->length + size <= b->capacity)) {
		byte * tr = b->contents + b->length;
		b->length += size;
		return tr;
	} else {
		b->capacity = b->length + size + 1000;
		b->contents = realloc (b->contents, b->capacity);
		return buffer_allocate (b, size);
	}
}

void
buffer_deinit (struct buffer * b)
{
	free (b->contents);
	memset (b, 0, sizeof *b);
}

// An i31b represents a 31-bit signed integer and a boolean packed into a single int.
typedef int i31b;

i31b
i31b_pack (int int_val, bool bool_val)
{
	return (clamp (INT_MIN/2, INT_MAX/2, int_val) << 1) | (bool_val ? 1 : 0);
}

void
i31b_unpack (i31b packed_val, int * out_int_val, bool * out_bool_val)
{
	*out_int_val = packed_val >> 1;
	*out_bool_val = packed_val & 1;
}

// ========================================================
// ||                                                    ||
// ||     SIMPLE HASH TABLES FOR INT AND STRING KEYS     ||
// ||                                                    ||
// ========================================================

void
table_deinit (struct table * t)
{
	free (t->block);
	memset (t, 0, sizeof *t);
}

size_t
table_capacity (struct table const * t)
{
	return (t->block != NULL) ? (size_t)1 << t->capacity_exponent : 0;
}

#define TABLE__BASE(table) ((int *)((char *)(table)->block + (table_capacity (table) / 8)))
#define TABLE__OCCUPANCIES(table) ((unsigned char *)(table)->block)

int
table__is_occupied (struct table const * t, size_t index)
{
    int b = (TABLE__OCCUPANCIES (t))[index >> 3];
    int mask = 1 << (index & 7);
    return b & mask;
}

void
table__set_occupation (struct table * t, size_t index, int occupied)
{
    unsigned char * b = &((TABLE__OCCUPANCIES (t))[index >> 3]);
    unsigned char mask = 1 << (index & 7);
    *b = occupied ? (*b | mask) : (*b & ~mask);
}

// Finds an appropriate index for a given key in the table. If the key is already present in the table, the index of its entry will be returned. If
// it's not present, the index of where it would be is returned. This function assumes that the table is not full and that it has been allocated.
size_t
table__place (struct table const * t, int (* compare_keys) (int, int), int key, size_t hash)
{
	size_t mask = ((size_t)1 << t->capacity_exponent) - 1;
	size_t index = hash & mask;
	int * base = TABLE__BASE (t);
	while (1) {
		int * entry = &base[2*index];
		if ((! table__is_occupied (t, index)) || compare_keys (key, entry[0]))
			return index;
		else
			index = (index + 1) & mask;
	}
}

void
table__expand (struct table * t, size_t (* hash) (int), int (* compare_keys) (int, int))
{
	size_t new_capacity_exponent = (t->capacity_exponent > 0) ? (t->capacity_exponent + 1) : 5;
	void * new_block; {
		size_t new_capacity = (size_t)1 << new_capacity_exponent;
		size_t new_block_size = new_capacity/8 + new_capacity*2*sizeof(int);
		new_block = malloc (new_block_size);
		memset (new_block, 0, new_block_size); // Clear new block to zero (this is important)
	}

	struct table new_table = { .block = new_block, .capacity_exponent = new_capacity_exponent, .len = 0 };

	// Copy contents of the old table into the new one
	size_t old_capacity = table_capacity (t);
	int * old_base = TABLE__BASE (t);
	for (size_t n = 0; n < old_capacity; n++)
		if (table__is_occupied (t, n)) {
			int * old_entry = &old_base[2*n];
			size_t new_index = table__place (&new_table, compare_keys, old_entry[0], hash (old_entry[0]));
			int * new_entry = &((int *)TABLE__BASE (&new_table))[2*new_index];
			new_entry[0] = old_entry[0]; // copy key
			new_entry[1] = old_entry[1]; // copy value
			table__set_occupation (&new_table, new_index, 1);
			++new_table.len;
		}

	table_deinit (t);
	*t = new_table;
}

int
table_get_by_index (struct table const * t, size_t index, int * out_value)
{
	if ((t->len > 0) && table__is_occupied (t, index)) {
		int * entry = &((int *)TABLE__BASE (t))[2*index];
		*out_value = entry[1];
		return 1;
	}
	return 0;
}

int compare_int_keys (int a, int b) { return a == b; }
size_t hash_int (int key) { return key; }

void
itable_insert (struct table * t, int key, int value)
{
	if (t->len >= table_capacity (t) / 2)
		table__expand (t, hash_int, compare_int_keys);

	size_t index = table__place (t, compare_int_keys, key, hash_int (key));
	int * entry = &((int *)TABLE__BASE (t))[2*index];
	entry[0] = key;
	entry[1] = value;
	if (! table__is_occupied (t, index)) {
		table__set_occupation (t, index, 1);
		++t->len;
	}
}

// Deletes a table entry, if present. Returns whether or not the entry was present.
int
itable_remove (struct table * t, int key)
{
	if (t->len == 0)
		return 0;

	size_t index = table__place (t, compare_int_keys, key, hash_int (key));
	if (! table__is_occupied (t, index))
		return 0;

	table__set_occupation (t, index, 0);
	t->len--;

	// Re-insert any elements in the probe sequence
	size_t index_mask = ((size_t)1 << t->capacity_exponent) - 1;
	size_t current = (index + 1) & index_mask;
	while (table__is_occupied (t, current)) {
		int * entry = &((int *)TABLE__BASE (t))[2 * current];
		int cur_key = entry[0],
		    cur_value = entry[1];
		table__set_occupation (t, current, 0);

		size_t new_index = table__place (t, compare_int_keys, cur_key, hash_int (cur_key));
		int * new_entry = &((int *)TABLE__BASE (t))[2 * new_index];
		new_entry[0] = cur_key;
		new_entry[1] = cur_value;
		table__set_occupation (t, new_index, 1);

		current = (current + 1) & index_mask;
	}

	return 1;
}

int
itable_look_up (struct table const * t, int key, int * out_value)
{
	size_t index = (t->len > 0) ? table__place (t, compare_int_keys, key, hash_int (key)) : 0;
	return table_get_by_index (t, index, out_value);
}

// Returns the associated value if the key is present in the table, otherwise returns default_value
int
itable_look_up_or (struct table const * t, int key, int default_value)
{
	int value;
	int got_value = itable_look_up (t, key, &value);
	return got_value ? value : default_value;
}

int compare_str_keys (int str_ptr_a, int str_ptr_b) { return strcmp ((char const *)str_ptr_a, (char const *)str_ptr_b) == 0; }

int
compare_slice_and_str_keys (int slice_ptr, int str_ptr)
{
	struct string_slice const * slice = (struct string_slice const *)slice_ptr;
	char const * str = (char const *)str_ptr;
	return (strncmp (slice->str, str, slice->len) == 0) && (str[slice->len] == '\0');
}

size_t
hash_str (int str_ptr)
{
	size_t tr = 0;
	for (char const * str = (char const *)str_ptr; *str != '\0'; str++)
		tr = (tr<<5 ^ tr>>2) + *str;
	return tr;
}

size_t
hash_slice (int slice_ptr)
{
	struct string_slice const * slice = (struct string_slice const *)slice_ptr;
	size_t tr = 0;
	for (int n = 0; n < slice->len; n++)
		tr = (tr<<5 ^ tr>>2) + slice->str[n];
	return tr;

}

// Like regular deinit but also frees all keys
void
stable_deinit (struct table * t)
{
	if (t->len > 0) {
		size_t capacity = table_capacity (t);
		for (size_t n = 0; n < capacity; n++)
			if (table__is_occupied (t, n)) {
				int * entry = &((int *)TABLE__BASE (t))[2*n];
				free ((void *)entry[0]);
			}
	}
	table_deinit (t);
}

void
stable_insert (struct table * t, char const * key, int value)
{
	if (t->len >= table_capacity (t) / 2)
		table__expand (t, hash_str, compare_str_keys);

	size_t index = table__place (t, compare_str_keys, (int)key, hash_str ((int)key));
	int * entry = &((int *)TABLE__BASE (t))[2*index];
	if (entry[0] == 0)
		entry[0] = (int)strdup (key);
	entry[1] = value;
	if (! table__is_occupied (t, index)) {
		table__set_occupation (t, index, 1);
		++t->len;
	}
}

int
stable_look_up (struct table const * t, char const * key, int * out_value)
{
	size_t index = (t->len > 0) ? table__place (t, compare_str_keys, (int)key, hash_str ((int)key)) : 0;
	return table_get_by_index (t, index, out_value);
}

int
stable_look_up_slice (struct table const * t, struct string_slice const * key, int * out_value)
{
	size_t index = (t->len > 0) ? table__place (t, compare_slice_and_str_keys, (int)key, hash_slice ((int)key)) : 0;
	return table_get_by_index (t, index, out_value);
}

struct table_entry_iter {
	struct table const * t;
	size_t index;
	size_t capacity;
	int key;
	int value;
};

void
tei_next (struct table_entry_iter * tei)
{
	size_t new_index = tei->index + 1;
	while ((new_index < tei->capacity) && (! table__is_occupied (tei->t, new_index)))
		new_index++;
	if (new_index < tei->capacity) {
		int * entry = &((int *)TABLE__BASE (tei->t))[2*new_index];
		tei->key = entry[0];
		tei->value = entry[1];
	}
	tei->index = new_index;
}

struct table_entry_iter
tei_init (struct table const * t)
{
	struct table_entry_iter tr = {0};
	tr.t = t;
	tr.capacity = table_capacity (t);
	if (t->len > 0) {
		tr.index = -1;
		tei_next (&tr);
	}
	return tr;
}

#define FOR_TABLE_ENTRIES(tei_name, p_table) for (struct table_entry_iter tei_name = tei_init (p_table); tei_name.index < tei_name.capacity; tei_next (&tei_name))

// Writes the contents of the table to the buffer as an array of key-value pairs. Returns the number of bytes written. Assumes the buffer contents is
// four-byte aligned.
int
itable_serialize (struct table const * t, struct buffer * b)
{
	int tr = sizeof(int) + t->len * 2 * sizeof(int);
	int * out = (int *)buffer_allocate (b, tr);
	*out++ = t->len;
	FOR_TABLE_ENTRIES (tei, t) {
		*out++ = tei.key;
		*out++ = tei.value;
	}
	return tr;
}

// Reads table entries from a buffer, inserting them into the given table. Will not read beyond buf_end. Returns the number of bytes read if
// successful or 0 if there was an error.
int
itable_deserialize (byte const * buf, byte const * buf_end, struct table * t)
{
	int const * ints = (int const *)buf;

	if (((int)buf & 3) || (buf_end - buf < 4))
		return 0;
	int count = *ints++;
	if (buf + (count * 2 * sizeof(int)) > buf_end)
		return 0;

	for (int n = 0; n < count; n++)
		itable_insert (t, ints[2*n], ints[2*n + 1]);

	return (2*count + 1) * sizeof(int);
}



// ===============================
// ||                           ||
// ||     PARSING FUNCTIONS     ||
// ||                           ||
// ===============================

int
is_horiz_space (char c)
{
	return ((c == ' ') || (c == '\t') || (c == '\r'));
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
skip_white_space (char ** p_cursor)
{
	char * cur = *p_cursor;
	while (is_horiz_space (*cur) || (*cur == '\n'))
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
read_i31b (struct string_slice const * s, i31b * out_i31b_val)
{
	struct string_slice trimmed = trim_string_slice (s, 1);
	if (trimmed.len > 0) {
		bool percent = trimmed.str[trimmed.len - 1] == '%';
		if (percent)
			trimmed.len -= 1;

		int int_val;
		if (read_int (&trimmed, &int_val)) {
			*out_i31b_val = i31b_pack (int_val, percent);
			return 1;
		} else
			return 0;
	} else
		return 0;
}

char const windows1252_alpha_nums[256] = {
	0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0, // control chars
	0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0, // control chars
	0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0, //   ! " #   $ % & '   ( ) * +   , - . /
	1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 0, 0,   0, 0, 0, 0, // 0 1 2 3   4 5 6 7   8 9 : ;   < = > ?
	0, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1, // @ A B C   D E F G   H I J K   L M N O
	1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 0,   0, 0, 0, 1, // P Q R S   T U V W   X Y Z [   \ ] ^ _
	0, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1, // ` a b c   d e f g   h i j k   l m n o
	1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 0,   0, 0, 0, 0, // p q r s   t u v w   x y z {   | } ~
	0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 1, 0,   1, 0, 1, 0, // €   ‚ ƒ   „ … † ‡   ˆ ‰ Š ‹   Œ   Ž
	0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 1, 0,   1, 0, 1, 1, //   ‘ ’ “   ” • – —   ˜ ™ š ›   œ   ž Ÿ
	0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0, //   ¡ ¢ £   ¤ ¥ ¦ §   ¨ © ª «   ¬   ® ¯
	0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0, // ° ± ² ³   ´ µ ¶ ·   ¸ ¹ º »   ¼ ½ ¾ ¿
	1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1, // À Á Â Ã   Ä Å Æ Ç   È É Ê Ë   Ì Í Î Ï
	1, 1, 1, 1,   1, 1, 1, 0,   1, 1, 1, 1,   1, 1, 1, 1, // Ð Ñ Ò Ó   Ô Õ Ö ×   Ø Ù Ú Û   Ü Ý Þ ß
	1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1, // à á â ã   ä å æ ç   è é ê ë   ì í î ï
	1, 1, 1, 1,   1, 1, 1, 0,   1, 1, 1, 1,   1, 1, 1, 1, // ð ñ ò ó   ô õ ö ÷   ø ù ú û   ü ý þ ÿ
};

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
		while (windows1252_alpha_nums[*(unsigned char *)cur] || (*cur == '_') || (*cur == '-') || (*cur == '.') || (*cur == '%'))
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
parse_int (char ** p_cursor, int * out)
{
	char * cur = *p_cursor;
	struct string_slice ss;
	if (parse_string (&cur, &ss) && read_int (&ss, out)) {
		*p_cursor = cur;
		return 1;
	} else
		return 0;
}

int
parse_i31b (char ** p_cursor, int * out_i31b_val)
{
	char * cur = *p_cursor;
	struct string_slice ss;
	if (parse_string (&cur, &ss) && read_i31b (&ss, out_i31b_val)) {
		*p_cursor = cur;
		return 1;
	} else
		return 0;
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
	else if (0 == strncmp ("repl vis" , trimmed.str, trimmed.len)) *out = OJ_REPL_VIS;
	else if (0 == strncmp ("ext walup", trimmed.str, trimmed.len)) *out = OJ_EXT_WALUP;
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

// Parses a single row from civ_prog_objects.csv. The CSV file contains multiple columns for addresses but this method only reads one, specified by
// index in addr_column.
int
parse_civ_prog_object (char ** p_cursor, int addr_column, struct civ_prog_object * out)
{
	char * cursor = *p_cursor;
	struct string_slice columns[6];
	int count_columns = (sizeof columns) / (sizeof columns[0]);
	for (int n = 0; n < count_columns; n++) {
		if (parse_csv_value (&cursor, &columns[n].str, &columns[n].len)) {
			char c = *cursor; // Check character after value for errors & to skip if necessary
			if ((n < count_columns - 1) && (c == ',')) // Before the last column, the value must end in a comma. Skip it if it's present.
				cursor++;
			else if ((n == count_columns - 1) && (c == '\n')) // If the last column ends in a new line, skip it.
				cursor++;
			else if ((n == count_columns - 1) && (c == '\0')) // If it ends in a null terminator that's OK but don't skip it.
				;
			else // Anything else is an error
				return 0;
		} else
			return 0;
	}

	struct civ_prog_object tr;
	if (read_object_job (&columns[0], &tr.job) &&
	    read_int (&columns[addr_column], &tr.addr)) {
		tr.name = trim_and_extract_slice (&columns[4], 1);
		tr.type = trim_and_extract_slice (&columns[5], 1);
		*out = tr;
		*p_cursor = cursor;
		return 1;
	} else
		return 0;

}



// =======================================
// ||                                   ||
// ||     TEXT LOADING AND ENCODING     ||
// ||                                   ||
// =======================================

char *
file_to_string (char const * filepath)
{
	HANDLE file = CreateFileA (filepath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE)
		goto err_in_CreateFileA;
	DWORD size = GetFileSize (file, NULL);
	char * tr = malloc (size + 1);
	if (tr == NULL)
		goto err_in_malloc;
	DWORD size_read = 0;
	int success = ReadFile (file, tr, size, &size_read, NULL);
	if ((! success) || (size_read != size))
		goto err_in_ReadFile;
	tr[size] = '\0';
	CloseHandle (file);
	return tr;

err_in_ReadFile:
	free (tr);
err_in_malloc:
	CloseHandle (file);
err_in_CreateFileA:
	return NULL;
}

// Re-encodes a text buffer from UTF-8 to Windows-1252, a single-byte encoding used internally by Civ 3.
char *
utf8_to_windows1252 (char const * text)
{
	// Skip the UTF-8 byte order mark, if present
	if (0 == strncmp (text, "\xEF\xBB\xBF", 3))
		text += 3;

	int text_len = strlen (text);
	int wide_text_size = 2 * (text_len + 1); // Size of wide text buffer in bytes. Each char is 2 bytes, +1 char for null terminator

	void * wide_text = malloc (wide_text_size);
	int wide_text_len = MultiByteToWideChar (CP_UTF8, MB_ERR_INVALID_CHARS, text, -1, wide_text, text_len + 1);
	if (wide_text_len == 0) { // Error
		free (wide_text);
		return NULL;
	}

	void * tr = malloc (text_len + 1);
	int bytes_written = WideCharToMultiByte (1252, 0, wide_text, -1, tr, text_len + 1, NULL, NULL);
	if (bytes_written == 0) { // Error
		free (tr);
		free (wide_text);
		return NULL;
	}

	free (wide_text);
	return tr;
}
