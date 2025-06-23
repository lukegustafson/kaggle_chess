#define TT_NO_VALUE INT16_MIN
#define ENTRIES_PER_CLUSTER 5

#ifndef TT_CLUSTERS
#define TT_CLUSTERS 49152
#endif

//bound = 0 implies uninitialized
enum TTBound : uint8_t
{
	EMPTY = 0,
	LOWER = 1,
	UPPER = 2,
	EXACT = 3
};

using Collision_Resolver = uint32_t;

static uint8_t TT_gen = 0;
void advance_TT_gen()
{
	TT_gen += 4;
}

struct TTEntry
{
	Collision_Resolver hash;
	uint16_t move;
	Value static_eval;
	Value search_eval;
	uint8_t depth;
	uint8_t gen_and_bound;  //high 6 bits = generation. Low 2 bits = bound type
	
	void write(uint64_t hash_, uint16_t move_, Value static_eval_, Value search_eval_, int depth_, uint8_t bound_)
	{
		//Update move if we have one, or if this is a new entry
		if(hash_ != hash || move_ != Move::NO_MOVE)
			move = move_;

		//Update everything else if this is a new entry or is more exact
		if(hash_ != hash || bound_ == TTBound::EXACT || depth_ >= depth)
		{
			hash = static_cast<Collision_Resolver>(hash_);
			move = move_;
			static_eval = static_eval_;
			search_eval = search_eval_;
			depth = std::max(0, std::min(255, depth_));
			gen_and_bound = bound_ | TT_gen;
		}
	}
};
static_assert(sizeof(TTEntry) == 12, "TTEntry Size wrong");

struct TTCluster
{
	TTEntry entries[ENTRIES_PER_CLUSTER];
	char padding[4];
};
static_assert(sizeof(TTCluster) == 64, "TTCluster size wrong");

alignas(64) TTCluster TT[TT_CLUSTERS];

int TT_age(TTEntry& entry)
{
	return (64 + TT_gen - (entry.gen_and_bound >> 2)) & 63;
}

struct TTRef
{
	TTEntry& entry;
	bool found;
};

TTRef get_TTEntry(uint64_t hash)
{
	auto& cluster = TT[(static_cast<unsigned __int128>(TT_CLUSTERS) * static_cast<unsigned __int128>(hash)) >> 64];
	auto resolver = static_cast<Collision_Resolver>(hash);
	
	//Look for exact match
	for(int i = 0; i < ENTRIES_PER_CLUSTER; ++i)
	{
		TTEntry& entry = cluster.entries[i];
		if(entry.hash == resolver)
		{
			//Reading already means the entry was useful, so update the generation
			//todo: test whether this actually helps elo
			entry.gen_and_bound = TT_gen | (entry.gen_and_bound & 3);
			return TTRef{entry, true};
		}
	}
	
	//Find best replacement
	int best = 0;
	int best_score = cluster.entries[best].depth - 8 * TT_age(cluster.entries[best]);
	for(int i = 1; i < ENTRIES_PER_CLUSTER; ++i)
	{
		int score = cluster.entries[i].depth - 8 * TT_age(cluster.entries[i]);
		if(score < best_score)
		{
			best = i;
			best_score = score;
		}
	}
		
	return TTRef{cluster.entries[best], false};
}

void clear_TT()
{
	for(int i = 0; i < TT_CLUSTERS; ++i)
		for(int j = 0; j < ENTRIES_PER_CLUSTER; ++j)
			TT[i].entries[j].write(0, Move::NO_MOVE, TT_NO_VALUE, TT_NO_VALUE, 0, 0);
}
