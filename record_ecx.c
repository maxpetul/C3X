
// Memo is a flat array of ints. The contents are paired up into (value, count). Counts must be initialized to -1.

void
record_ecx (int ecx)
{
	int * memo = (int *)MEMO_ADDRESS;
	for (int n = 0; n < MEMO_LENGTH; n++)
		if (memo[2*n+1] < 0) { // If we've reached a blank entry
			memo[2*n] = ecx;
			memo[2*n+1] = 1;
			break;
		} else if (memo[2*n] == ecx) { // If we've reached a matching entry
			memo[2*n+1] += 1;
			break;
		}
}
