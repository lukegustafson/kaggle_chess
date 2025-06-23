uint64_t rotl(uint64_t x, int r)
{
	return (x << r) | (x >> 64-r);
}

///xoroshiro128++ with David Stafford's finalizer
uint64_t random64()
{
	static uint64_t x = 0x4112CF68649A260E, y = 0xD813F2FAB7F5C5CA;
	
	uint64_t result = rotl(x + y, 17) + x;

	y ^= x;
	x = rotl(x, 49) ^ y ^ (y << 21);
	y = rotl(y, 28);

	result ^= result >> 31;
	result *= 0x7fb5d329728ea185;
	result ^= result >> 27;
	result *= 0x81dadef4bc2dd44d;
	result ^= result >> 33;
	return result;
}