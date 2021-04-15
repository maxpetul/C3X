
int
is_city_in_disorder (void * city, int param_1, int param_2)
{
	return *(int *)((char *)city + 0x30) & 1;
}

int
main ()
{
	return 0;
}
