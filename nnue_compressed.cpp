#include <algorithm>

#define FIXED_POINT 8

#ifndef NNUE_FILE
#define NNUE_FILE "64_16_8_q64.cpp"
#endif


namespace nnue {
#include NNUE_FILE

//Arithmetic decoder for one bit at a time
//p = probability * 256, should be 1..256
bool decode(uint32_t p)
{
	static uint32_t h = 1, c = 0, i = 0;
	while(h < 16384)
	{
		h *= 256;
		c *= 256;
		c += compressed_data[i++];
	}
	uint32_t t = h * p >> 8;
	if(c < t)
	{
		h = t;
		return false;
	}
	else
	{
		c -= t;
		h -= t;
		return true;
	}
}

int16_t next_weight()
{
	int exp = 0;
	while(decode(thresholds[exp]))
		exp += 1;
	
	if(exp == 0)
		return 0;
	
	int x = 0;
	int sign = decode(128) ? -1 : 1;
	for(int i = 0; i < exp - 1; ++i)
		x += decode(128) << i;
	x += 1 << exp - 1;
	
	return (sign * x << FIXED_POINT) / QUANTIZE;
}

alignas(32) int32_t accumulator[2][INPUT_LAYER];
alignas(32) int16_t PST[768 * INPUT_LAYER];
alignas(32) int32_t l1_bias[INPUT_LAYER];
alignas(32) int32_t l1_output[INPUT_LAYER];
alignas(32) int32_t l2_weights[4][INPUT_LAYER * HIDDEN1];
alignas(32) int32_t l2_bias[4][HIDDEN1];
alignas(32) int32_t l2_output[HIDDEN1];
alignas(32) int32_t l3_weights[4][HIDDEN1 * HIDDEN2];
alignas(32) int32_t l3_bias[4][HIDDEN2];
alignas(32) int32_t l3_output[HIDDEN2];
alignas(32) int32_t l4_weights[4][HIDDEN2];
int32_t l4_bias[4];
int32_t game_phase;
int32_t queens;

#ifdef DEBUGGING
bool verbose = false;
#endif

void init()
{
	for(int piece = 0; piece < 6; ++piece)
		for(int color = 0; color < 2; ++color)
			for(int square = 0; square < 64; ++square)
				for(int j = 0; j < INPUT_LAYER; ++j)
				{
					int idx = ((color*6 + piece)*64 + square) * INPUT_LAYER + j;
					PST[idx] = piece == 0 && (square < 8 || square >= 56) ? 0 : next_weight();
					if(square != 0 && (square != 8 || piece != 0))
						PST[idx] += PST[idx - INPUT_LAYER];
				}
				
	for(int i = 0; i < INPUT_LAYER; ++i)
		l1_bias[i] = next_weight();
	
	for(int h = 0; h < 4; ++h)
	{
		for(int i = 0; i < INPUT_LAYER * HIDDEN1; ++i)
			l2_weights[h][i] = next_weight();
		
		for(int i = 0; i < HIDDEN1; ++i)
			l2_bias[h][i] = next_weight();
		
		for(int i = 0; i < HIDDEN1 * HIDDEN2; ++i)
			l3_weights[h][i] = next_weight();
		
		for(int i = 0; i < HIDDEN2; ++i)
			l3_bias[h][i] = next_weight();
		
		for(int i = 0; i < HIDDEN2; ++i)
			l4_weights[h][i] = next_weight();
		
		l4_bias[h] = next_weight();
	}
}

#ifdef DEBUGGING
void log(const char* s, int32_t *l, int n)
{
	printf("%s output: ", s);
	for(int i = 0; i < n; ++i)
		printf("%d ", l[i]);
	printf("\n");
}
#endif

__attribute__((optimize("tree-vectorize"))) Value evaluate(int32_t * accumulator)
{
	int bucket = 2 * (queens > 0) + (game_phase > 8);
	
	for(int i = 0; i < INPUT_LAYER; ++i)
		l1_output[i] = accumulator[i] * (accumulator[i] > 0);
	
#ifdef DEBUGGING	
	if(verbose) log("L1", l1_output, INPUT_LAYER);
#endif
	
	//l1 done
	
	for(int i = 0; i < HIDDEN1; ++i)
		l2_output[i] = l2_bias[bucket][i];
	
	for(int j = 0; j < HIDDEN1; ++j)
		for(int i = 0; i < INPUT_LAYER; ++i)
			l2_output[j] += l1_output[i] * l2_weights[bucket][j * INPUT_LAYER + i] >> FIXED_POINT;
	
	for(int i = 0; i < HIDDEN1; ++i)
#ifdef SOFTRELU_BUG
		//This was supposed to be a "soft" relu, but the trainer had a bug making it a regular relu
		l2_output[i] = l2_output[i] * (l2_output[i] > 0);
#else		
		l2_output[i] = l2_output[i] * (l2_output[i] > 0) + (l2_output[i] >> 4);
#endif	
		
	
#ifdef DEBUGGING	
	if(verbose) log("L2", l2_output, HIDDEN1);
#endif	
	
	//l2 done
	
	for(int i = 0; i < HIDDEN2; ++i)
		l3_output[i] = l3_bias[bucket][i];
	
	for(int j = 0; j < HIDDEN2; ++j)
		for(int i = 0; i < HIDDEN1; ++i)
			l3_output[j] += l2_output[i] * l3_weights[bucket][j * HIDDEN1 + i] >> FIXED_POINT;
		
	for(int i = 0; i < HIDDEN2; ++i)
		l3_output[i] = l3_output[i] * (l3_output[i] > 0) + (l3_output[i] >> 4);
	
#ifdef DEBUGGING	
	if(verbose) log("L3", l3_output, HIDDEN2);
#endif	
	
	//l3 done
	
	int32_t result = l4_bias[bucket];
	
	for(int i = 0; i < HIDDEN2; ++i)
		result += l3_output[i] * l4_weights[bucket][i] >> FIXED_POINT;
	
#ifdef DEBUGGING	
	if(verbose)	printf("Final output %d\n", result);
#endif
	
	return std::clamp(result >> FIXED_POINT, -20000, 20000);
}

void clear_accumulator()
{
	static bool initialized = false;
	
	if(!initialized)
	{
		init();
		initialized = true;
#ifdef DEBUGGING
		printf("NNUE check: %d %d %d\n", PST[16 * INPUT_LAYER], PST[16 * INPUT_LAYER + 1], l4_bias[3]);
#endif
	}
	
	for(int i = 0; i < INPUT_LAYER; ++i)
	{
		accumulator[0][i] = l1_bias[i];
		accumulator[1][i] = l1_bias[i];
	}
	game_phase = 0;
	queens = 0;
}

int pst_phase[] = {0,1,1,2,0,0,0,1,1,2,0,0};

__attribute__((optimize("tree-vectorize"))) void add_accumulator(int piece, int square)
{
	for(int i = 0; i < INPUT_LAYER; ++i)
	{
		accumulator[0][i] += PST[piece * 64 * INPUT_LAYER + square * INPUT_LAYER + i];
		accumulator[1][i] += PST[(piece > 5 ? piece - 6 : piece + 6) * 64 * INPUT_LAYER + (square ^ 56) * INPUT_LAYER + i];
	}
	
	queens += (piece == 4) + (piece == 10);
	game_phase += pst_phase[piece];
}

__attribute__((optimize("tree-vectorize"))) void remove_accumulator(int piece, int square)
{
	for(int i = 0; i < INPUT_LAYER; ++i)
	{
		accumulator[0][i] -= PST[piece * 64 * INPUT_LAYER + square * INPUT_LAYER + i];
		accumulator[1][i] -= PST[(piece > 5 ? piece - 6 : piece + 6) * 64 * INPUT_LAYER + (square ^ 56) * INPUT_LAYER + i];
	}
	
	queens -= (piece == 4) + (piece == 10);
	game_phase -= pst_phase[piece];
}

} //namespace nnue