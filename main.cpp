/*
MIT License

Copyright (c) 2024-2025 Luke Gustafson

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <unistd.h>
#include <cstdint>
#include <time.h>
#include <immintrin.h>

using Value = int16_t;

/////////////////////////////////////////////////////////////////////////////
//Configuration
/////////////////////////////////////////////////////////////////////////////

//#define DEBUGGING 1

#define VERSION "v04"

#define MAX_PLY 64

//#define TT_CLUSTERS 10000000

#define KILLERS 2

/////////////////////////////////////////////////////////////////////////////
//Includes (unity build)
/////////////////////////////////////////////////////////////////////////////

#ifdef DEBUGGING
#include <cstdio>
#endif

#include "rng.cpp"
#include "nnue_compressed.cpp"
#include "utility.cpp"

#define NDEBUG 1
#define CHESS_NO_EXCEPTIONS 1
#include "chess.hpp"

using namespace chess;

#include "transposition_table.cpp"

/////////////////////////////////////////////////////////////////////////////
//Search, eval, and main
/////////////////////////////////////////////////////////////////////////////

Board board = Board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
alignas(64) Movelist move_stack[MAX_PLY];
alignas(64) Move search_moves[MAX_PLY];
alignas(64) Move killers[MAX_PLY][KILLERS];
int32_t history_h[6][12][64];
Value static_eval_stack[MAX_PLY];
Move pv;
uint64_t nodes;
uint64_t qnodes;
uint64_t max_depth;
uint64_t time_allocated;
timespec think_time_start;
bool pondering;
bool stop;
Value last_value;

#if DEBUGGING > 1
uint64_t move_order[8][8];
#endif

Value calc_static_eval()
{
	return nnue::evaluate(nnue::accumulator[static_cast<int>(board.sideToMove())]);
}

Value piece_value(PieceType pt)
{
	static Value values[] = { 100, 290, 300, 500, 900, 0, 0 };
	return values[static_cast<int>(pt)];
}

void clear_history()
{
	for(int i = 0; i < 6; ++i)
		for(int j = 0; j < 12; ++j)
			for(int k = 0; k < 64; ++k)
				history_h[i][j][k] = 0;
}

int32_t& history_entry(Move m)
{
	int i = static_cast<int>(board.at<PieceType>(m.to()));
	i -= i > 5;
	int j = static_cast<int>(board.at<Piece>(m.from()));
	j -= j > 11;
	int k = m.to().index();
	
	return history_h[i][j][k];
}

int16_t history_score(Move m)
{
	return history_entry(m) >> 5;
}

void update_history(Movelist &ms, int n_moves, Move best_move, int depth)
{
	int bonus = std::clamp(depth * depth, 1, 1024);
	int malus = -std::max(bonus / 8, 1);
	for(int i = 0; i < n_moves; ++i)
	{
		int score = ms[i] == best_move ? bonus : malus;
		int32_t &entry = history_entry(ms[i]);
		entry -= std::abs(score) * entry >> 16;  //this will keep the entries bounded at +- 2^16
		entry += score;
	}
}

#ifdef DEBUGGING
void print_pv(int depth)
{
	if(depth <= 0)
		return;
	TTRef ttRef	= get_TTEntry(board.hash());
	if(ttRef.found && ttRef.entry.move != Move::NO_MOVE)
	{
		printf("%s ", move_str(ttRef.entry.move));
		board.makeMove(ttRef.entry.move);
		print_pv(depth - 1);
		board.unmakeMove(ttRef.entry.move);
	}
}
#endif

bool input_ready()
{
    fd_set rd;
    timeval tv = {0};
    
    FD_ZERO(&rd);
    FD_SET(STDIN_FILENO, &rd);
    return select(1, &rd, NULL, NULL, &tv) > 0;
}

uint64_t time_elapsed()
{
	timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return static_cast<uint64_t>(now.tv_sec - think_time_start.tv_sec) * 1000000000ull + now.tv_nsec - think_time_start.tv_nsec;
}

void sort_moves(Movelist& moves)
{
	//insertion sort as per Wikipedia
	for(int i = 1; i < moves.size(); ++i)
	{
		Move temp = moves[i];
		int j = i;
		for(; j > 0 && moves[j-1].score() < temp.score(); --j)
		{
			moves[j] = moves[j-1];
        }
		moves[j] = temp;
    }
}

bool check_kaggle_insufficient()
{
	/*
	//Code from Kaggle environments which we want to replicate
	
	def sufficient_material(pieces):
		"""Checks if the given pieces are sufficient for checkmate."""
		if pieces['q'] > 0 or pieces['r'] > 0 or pieces['p'] > 0:
			return True
		# we only have knights and bishops left on this team
		knight_bishop_count = pieces['n'] + pieces['b']
		if knight_bishop_count >= 3:
			return True
		if knight_bishop_count == 2 and pieces['b'] >= 1:
			return True

		return False


	def is_insufficient_material(board):
		white_pieces = defaultdict(int)
		black_pieces = defaultdict(int)

		for square in range(64):
			piece = board.get_piece(square)
			if piece and piece != " ":
				if piece.isupper():
					white_pieces[piece.lower()] += 1
				else:
					black_pieces[piece.lower()] += 1

		if not sufficient_material(
				white_pieces) and not sufficient_material(black_pieces):
			return True

		return False
	*/
	
	//Any QRP
	if(board.pieces(PieceType::PAWN) || board.pieces(PieceType::ROOK) || board.pieces(PieceType::QUEEN))
		return false;
	
	//One side has 3 minors or B + minor
	for(auto color : {Color::WHITE, Color::BLACK})
	{
		int knights = board.pieces(PieceType::KNIGHT, color).count();
		int bishops = board.pieces(PieceType::BISHOP, color).count();
		if(knights + bishops >= 3 || (bishops >= 1 && knights + bishops == 2))
			return false;
	}

	return true;
}

//Increments by 1 the distance to mate
Value mate_counter(Value x)
{
	return x + (x < -31000) - (x > 31000);
}

bool gives_check(Move m)
{
	//todo: discovered checks
	Bitboard king = board.pieces(PieceType::KING, ~board.sideToMove());
	switch(board.at<PieceType>(m.from()).internal())
	{
	case PieceType::PAWN:
		return static_cast<bool>(attacks::pawn(board.sideToMove(), m.to()) & king);
	case PieceType::KNIGHT:
		return static_cast<bool>(attacks::knight(m.to()) & king);
	case PieceType::BISHOP:
		return static_cast<bool>(attacks::bishop(m.to(), board.occ()) & king);
	case PieceType::ROOK:
		return static_cast<bool>(attacks::rook(m.to(), board.occ()) & king);
	case PieceType::QUEEN:
		return static_cast<bool>(attacks::queen(m.to(), board.occ()) & king);
	default:
		return false;
	}
}

Value alpha_beta(int depth, int ply, Value lower_bound, Value upper_bound, int16_t skip_move)
{
	//update search stats
	++nodes;
	max_depth = std::max(max_depth, static_cast<uint64_t>(ply));

	//fetch TT
	TTRef ttRef = get_TTEntry(board.hash());
	//make a copy because it's possible it gets overwritten in the recursion
	TTEntry ttEntry = ttRef.entry;

	//check for draws
	if(ply > 0)
	{
		/*
		//This code would be correct with respect to the rules of chess
		if(board.isInsufficientMaterial())
		{
			return 0;
		}*/	

		//Check for draws
		//This code replicates Kaggle's adjudication, which isn't always the same as the official rules of chess
		if(board.halfMoveClock() >= 100 || check_kaggle_insufficient() || board.isRepetition())
			return 0;
				
		if(ply >= MAX_PLY)
		{
			if(ttRef.found)
			{
				Value search_eval = ttEntry.search_eval;
				if(search_eval != TT_NO_VALUE)
				{
					if((ttEntry.gen_and_bound & TTBound::LOWER) && search_eval >= upper_bound)
						return search_eval;
					if((ttEntry.gen_and_bound & TTBound::UPPER) && search_eval <= lower_bound)
						return search_eval;
				}
				
				return ttEntry.static_eval;
			}
			else
			{
				//don't create a TT entry
				return calc_static_eval();
			}
		}
		
		if((nodes & 4095) == 0)
		{
			//check for time or end of pondering
			if(time_elapsed() > time_allocated || (pondering && input_ready()))
			{
				stop = true;
				return 0;
			}
		}
	}

	Value best_value = -32001;
	Move best_move = Move::NO_MOVE;
	int best_move_num = -1;
	int moves_tried = -1;
	
	//TT cutoff
	if(ttRef.found && ttEntry.depth >= depth && skip_move == Move::NO_MOVE)
	{
		Value search_eval = ttEntry.search_eval;
		if(search_eval != TT_NO_VALUE && (ply > 0 || ttEntry.move != Move::NO_MOVE) && (ply > 0 || !pondering))
		{
			if(
				(ttEntry.gen_and_bound & TTBound::EXACT) == TTBound::EXACT
				|| ((ttEntry.gen_and_bound & TTBound::LOWER) && search_eval >= upper_bound)
				|| ((ttEntry.gen_and_bound & TTBound::UPPER) && search_eval <= lower_bound)
				)
			{
				if(ply == 0)
				{
					pv = ttEntry.move;
				}
				return search_eval;
			}
			else if(ttEntry.gen_and_bound & TTBound::LOWER)
			{
				best_value = ttEntry.search_eval;
				best_move = ttEntry.move;
			}
		}
	}
	
	Value static_eval = ttRef.found ? ttEntry.static_eval : calc_static_eval();
	static_eval_stack[ply] = static_eval;
	
	auto in_check = board.inCheck();
	
	auto ms = move_stack[ply];
	ms.clear();
	int move_gen = 0;
	
	//Initialze killer moves
	if(ply+1 < MAX_PLY)
		for(int j = 0; j < KILLERS; ++j)
			killers[ply+1][j] = Move::NO_MOVE;
	
	//QSearch
	if(depth <= 0)
	{
		qnodes++;
		depth = 0;
		
		if(!in_check)
		{
			best_value = std::max(best_value, static_eval);
		}
		
		//Stand pat
		//return if eval > upper_bound
		if(best_value >= upper_bound)
		{
			if(!ttRef.found)
			{
				ttRef.entry.write(board.hash(), Move::NO_MOVE, static_eval, best_value, 0, TTBound::LOWER);
			}
			
			return best_value;
		}
		
		if(best_value > lower_bound)
			lower_bound = best_value;
		
		move_gen = in_check ? 1 : 2;
	}
	else
	{
		move_gen = 3;
	}
	
	//Null move pruning
	Value eval = ttRef.found ? ttEntry.search_eval : TT_NO_VALUE;
	if(eval == TT_NO_VALUE)
		eval = static_eval;
	if(!in_check && ply > 1 && skip_move == Move::NO_MOVE && eval >= upper_bound && search_moves[ply - 1] != Move::NO_MOVE && board.hasNonPawnMaterial(board.sideToMove()))
	{
		board.makeNullMove();
		search_moves[ply] = Move::NO_MOVE;
		Value null_value = -alpha_beta(depth - depth / 3 - 4, ply + 1, -upper_bound, -upper_bound + 1, Move::NO_MOVE);
		board.unmakeNullMove();
		if(null_value >= upper_bound)
		{
			return null_value;
		}
	}
	
	for(int move_idx = -1; true; ++move_idx)
	{
		Move move;
		
		//Do move generation
		if(move_idx == 0)
		{
			if(move_gen == 1)
			{
				movegen::legalmoves(ms, board);
				if(ms.size() == 0)
					return -32000;
				
				//Prioritize captures
				for(auto move : ms)
				{
					Value captured_val = piece_value(board.at<PieceType>(move.to()));
					move.setScore(captured_val);
				}
			}
			else if(move_gen == 2)
			{
				movegen::legalmoves(ms, board, movegen::MoveGenType::CAPTURE);
				int last_capture = ms.size();
				
				for(int i = 0; i < last_capture; ++i)
				{
					Value captured_val = piece_value(board.at<PieceType>(ms[i].to()));
					Value my_val = piece_value(board.at<PieceType>(ms[i].from()));
					ms[i].setScore(captured_val - my_val / 16 + history_score(ms[i]));
				}
				
				//pawn promotions
				//TODO: for efficiency, see if we have pawns on 7th rank? Only do Queen promos?
				movegen::legalmoves(ms, board, movegen::MoveGenType::QUIET, PieceGenType::PAWN);
				for(int i = last_capture; i < ms.size(); ++i)
				{
					if(ms[i].typeOf() != Move::PROMOTION)
					{
						ms[i] = Move::NO_MOVE;
						ms[i].setScore(-32000);
					}
					else
					{
						ms[i].setScore(ms[i].promotionType() == PieceType::QUEEN ? piece_value(PieceType::QUEEN) : -10000);
					}
				}
			}
			else if(move_gen == 3)
			{
				movegen::legalmoves(ms, board);
		
				if(ms.size() == 0)
				{
					//checkmate or stalemate
					if(in_check)
						return -32000;
					else
						return 0;
				}
				
				//Find all squares attacked by < queen
				Bitboard pieces = board.pieces(PieceType::PAWN, ~board.sideToMove());
				Bitboard pawn_attacks = board.sideToMove() == Color::WHITE ? 
					attacks::pawnLeftAttacks<Color::BLACK>(pieces) | attacks::pawnRightAttacks<Color::BLACK>(pieces)
					: attacks::pawnLeftAttacks<Color::WHITE>(pieces) | attacks::pawnRightAttacks<Color::WHITE>(pieces);
				
				Bitboard minor_attacks = 0;
				pieces = board.pieces(PieceType::KNIGHT, ~board.sideToMove());
				while(pieces) 
				{
					const auto index = pieces.pop();
					minor_attacks |= attacks::knight(index);
				}
				
				pieces = board.pieces(PieceType::BISHOP, ~board.sideToMove());
				while(pieces) 
				{
					const auto index = pieces.pop();
					minor_attacks |= attacks::bishop(index, board.occ());
				}
				
				Bitboard rook_attacks = 0;
				pieces = board.pieces(PieceType::ROOK, ~board.sideToMove());
				while(pieces)
				{
					const auto index = pieces.pop();
					rook_attacks |= attacks::rook(index, board.occ());
				}				
				
				for(int i = 0; i < ms.size(); ++i)
				{
					if(ms[i].typeOf() == Move::PROMOTION)
					{
						ms[i].setScore(ms[i].promotionType() == PieceType::QUEEN ? 10000 : -10000);
					}
					else if(ms[i].typeOf() == Move::CASTLING)
					{
						ms[i].setScore(100);
					}
					else if(ms[i].typeOf() == Move::ENPASSANT)
					{
						ms[i].setScore(piece_value(PieceType::PAWN) * 16 + history_score(ms[i]));
					}
					else
					{
						Value score = piece_value(board.at<PieceType>(ms[i].to()));
						
						if(score == 0)
						{
							//For quiet moves, add for escapes, subtract for putting into danger
							Bitboard from = Bitboard::fromSquare(ms[i].from());
							Bitboard to = Bitboard::fromSquare(ms[i].to());
							Bitboard threats = pawn_attacks | minor_attacks;
							
							switch(board.at<PieceType>(ms[i].from()).internal())
							{
								case PieceType::KNIGHT:
								case PieceType::BISHOP:
									if(pawn_attacks & to)
										score -= piece_value(PieceType::KNIGHT) / 2;
									else if(pawn_attacks & from)
										score += piece_value(PieceType::KNIGHT) / 2;
									break;
								case PieceType::ROOK:
									if(threats & to)
										score -= piece_value(PieceType::ROOK) / 2;
									else if(threats & from)
										score += piece_value(PieceType::ROOK) / 2;
									break;
								case PieceType::QUEEN:
									threats |= rook_attacks;
									if(threats & to)
										score -= piece_value(PieceType::QUEEN) / 2;
									else if(threats & from)
										score += piece_value(PieceType::QUEEN) / 2;
								default: 
									break;
							}						
						}
						
						if(gives_check(ms[i]))
						{
							score += 150;
						}
						
						score *= 16;
						score += history_score(ms[i]);
						ms[i].setScore(score);
					}
				}
			}
			
			//boost killer moves
			if(ply > 0)
				for(auto & m : ms)
					if(m != Move::NO_MOVE)
						for(int i = 0; i < KILLERS; ++i)
							if(killers[ply][i] == m)
								m.setScore(std::max(m.score()+0, 200 - i));
						
			sort_moves(ms);
#if DEBUGGING > 1
			if(ply == 0)
				for(auto & m : ms)
					printf("%s %d\n", move_str(m), m.score());
#endif
		}
		
		//Are we done?
		if(move_idx >= ms.size())
			break;
		
		//Get next move from list
		if(move_idx >= 0)
		{
			move = ms[move_idx];
			if(ttRef.found && move == ttEntry.move)
				continue;
		}
		else
			move = ttRef.found ? ttEntry.move : Move::NO_MOVE;
				
		if(move == Move::NO_MOVE || move == skip_move)
			continue;
		
		moves_tried++;
		
		//Make sure we always have a move in case we get interrupted before we normally write the move
		if(ply == 0 && pv == Move::NO_MOVE)
			pv = move;
		
		//Reduce if not capture or improving move
		int depth_reduction = 4;

		//Improving move -- uncleare benefit
		// if(ply > 1 && static_eval_stack[ply] > static_eval_stack[ply-2])
			// depth_reduction = 2;

		//Extensions for captures and promotions -- unclear benefit
		// if(board.at<Piece>(move.to()) != Piece::NONE || move.typeOf() == Move::PROMOTION)
			// depth_reduction = 2;
		
		//Check extension
		if(in_check)
			depth_reduction = 1;

		//Singular extension / multi-cut -- unclear benefit
		/*
		if(ply > 0 && move_idx == -1 && depth >= 16 && ttEntry.search_eval != TT_NO_VALUE && skip_move == Move::NO_MOVE
			&& ttEntry.depth >= depth - 8 && (ttEntry.gen_and_bound & TTBound::LOWER) && ttEntry.search_eval < 4000)
		{
			Value singular_bound = ttEntry.search_eval - depth * 3;
			Value x = alpha_beta(depth / 2, ply + 1, singular_bound, ttEntry.search_eval, move.move());
			if(x <= singular_bound)
			{
#if DEBUGGING > 1				
				printf("SE success ply=%d ", ply);
				for(int i = 0; i < ply; ++i)
					printf("%s ", move_str(search_moves[i]));
				printf("%s\n", move_str(move));
#endif				
				depth_reduction = 0;
			}
			else if(x >= upper_bound)
			{
				//multi-cut
#if DEBUGGING > 1				
				printf("Multi-cut ply=%d ", ply);
				for(int i = 0; i < ply; ++i)
					printf("%s ", move_str(search_moves[i]));
				printf("%s\n", move_str(move));
#endif				
				return x;
			}
				
			//Reset the killer moves
			for(int j = 0; j < KILLERS; ++j)
				killers[ply+1][j] = Move::NO_MOVE;
		}
		*/

		//Late Move Reduction
		if((ply > 0 || best_move != Move::NO_MOVE) && depth > 11 && move.score() < 0)
		{
			board.makeMove(move);
			search_moves[ply] = move;
			Value x = -alpha_beta(depth - 8, ply + 1, -lower_bound, -lower_bound + 1, Move::NO_MOVE);
			board.unmakeMove(move);
			
			if(x < lower_bound)
			{
				if(stop)
					break;
				
				//Skip full search
				continue;
			}
		}
		
		// //Futility -- unclear benefit
		// if((ply > 0 || best_move != Move::NO_MOVE) && depth - depth_reduction <= 0 && move.score() < 0 
			// && eval < std::min(1000, lower_bound+0) - 300)
			// continue;
		
		//Evaluate
		board.makeMove(move);
		search_moves[ply] = move;
		Value x = mate_counter(-alpha_beta(depth - depth_reduction, ply + 1, -upper_bound, -lower_bound, Move::NO_MOVE));
		board.unmakeMove(move);
		
		//Interrupted?
		if(stop)
			break;
			
		//Update bounds
		if(x > lower_bound)
		{
			lower_bound = x;
		}
		if(x > best_value)
		{
			best_value = x;
			best_move = move;
			best_move_num = moves_tried;
			
			if(ply == 0)
			{
				pv = move;
#ifdef DEBUGGING
				printf("New PV %d %s ", x, move_str(pv));
				board.makeMove(move);
				print_pv(16);
				board.unmakeMove(move);
				printf("\n");
#endif
			}
		}
		
		//Beta cutoff
		if(x > upper_bound)
		{
			if(skip_move == Move::NO_MOVE)
				ttRef.entry.write(board.hash(), best_move.move(), static_eval, best_value, depth, TTBound::LOWER);
			
#if DEBUGGING > 1			
			if(best_move_num != -1)
				move_order[std::min(7,depth / 4)][std::min(7, best_move_num)]++;
#endif			
			
			//update history / killers
			update_history(ms, move_idx, best_move, depth);
			if(board.at(best_move.to()) == Piece::NONE)
			{
				for(int i = 0; i < KILLERS; ++i)
				{
					if(killers[ply][i] == best_move)
						break;
					if(killers[ply][i] == Move::NO_MOVE)
					{
						killers[ply][i] = best_move;
						break;
					}
				}
			}
			return x;
		}
	}
	
	//Write TT unless we were interrupted
	if(!stop)
	{
		if(skip_move == Move::NO_MOVE)
			ttRef.entry.write(board.hash(), best_move.move(), static_eval, best_value, depth, best_value >= lower_bound ? TTBound::EXACT : TTBound::UPPER);
		if(best_value >= lower_bound)
			update_history(ms, ms.size(), best_move, depth);
	}

#if DEBUGGING > 1	
	if(best_move_num != -1)
		move_order[std::min(7, depth/4)][std::min(7, best_move_num)]++;
#endif	
	
	return best_value;
}

Move search()
{
	nodes = 0;
	
	//debugging stats
	qnodes = 0;
	
#if DEBUGGING > 1	
	for(int i = 0; i < 8; ++i)
		for(int j = 0; j < 8; j++)
			move_order[i][j] = 0;
#endif		
	
	stop = false;
	Move best_move = Move::NO_MOVE;
	advance_TT_gen();
	Value min_value = -32001;
	Value max_value = 32001;
	//Value bound_inc = 100;
	for(int depth = 4; depth < 256 && !stop;)
	{
		max_depth = 0;
		pv = Move::NO_MOVE;
#ifdef DEBUGGING
		printf("Search d = %d a = %d b = %d\n", depth, min_value, max_value);
#endif		
		Value v = alpha_beta(depth, 0, min_value, max_value, Move::NO_MOVE);
		if(v != -32001)
		{
			last_value = v;
		}
#ifdef DEBUGGING
		printf("Depth = %d %s. Eval = %d, nodes = %lu/q%lu, max_plies = %lu, bestmove = %s\n", depth, stop ? "stop" : "finished",
			last_value, nodes, qnodes, max_depth, move_str(pv));
#if DEBUGGING > 1			
		for(int j = 0; j < 8; ++j)
		{
			printf("depth %d*4: ", j);
			for(int i = 0; i < 8; ++i)
				printf("%d: %ld  ", i, move_order[j][i]);
			printf("\n");
		}
#endif		
#endif

		if(v >= min_value && v <= max_value)
			best_move = pv;

		if(v != 32001 && !stop)
		{
			if(v >= min_value && v <= max_value)
			{
				depth += 4;
				
				//Commented out code here is for aspiration window -- unclear benefit
				//bound_inc = 100 + std::abs(v) / 4;
				//min_value = std::max(-32000, v - bound_inc);
				//max_value = std::min(32000, v + bound_inc);
			}
			// else if(v < min_value)
			// {
				// min_value = std::max(-32000, v - bound_inc);
				// bound_inc = std::min(bound_inc * 2, 3200);
			// }
			// else
			// {
				// max_value = std::min(32000, v + bound_inc);
				// bound_inc = std::min(bound_inc * 2, 3200);
			// }
		}
	}
	
	if(best_move != Move::NO_MOVE)
		return best_move;
	else
		return pv;
}

int main(void)
{
	char buf[128];
	
	Zobrist::init();
	clear_TT();
	clear_history();

	print("M" VERSION "\n",5);
	print("MTT",3);
	print_num(TT_CLUSTERS);
	print("\n",1);
	while(true)
	{
		if(read(0, buf, 128) > 0)
		{
			clock_gettime(CLOCK_MONOTONIC, &think_time_start);
						
			if(buf[0] == 'q')
				break;
			if(buf[0] == 'f')
			{
				board = Board(buf+1);
				time_allocated = 500'000'000;
				clear_TT();
				clear_history();
			}
			else
			{
				Move move = uci::uciToMove(board, buf+1);
				board.makeMove(move);
				time_allocated = 20'000'000 * static_cast<uint64_t>(buf[0] - 32);
			}
			
			pondering = false;
			nodes = 0;
			Move best_move = search();
						
			timespec end_think;
			clock_gettime(CLOCK_MONOTONIC, &end_think);
			uint64_t x = static_cast<uint64_t>(end_think.tv_sec - think_time_start.tv_sec) * 1000000000ull + end_think.tv_nsec - think_time_start.tv_nsec;
			x /= 1000000;
			print("Mtime=",6);
			print_num(x);
			print("\nMnodes=",8);
			print_num(nodes);
			print("\nMvalue=",8);
			if(last_value < 0)
			{
				print("-",1);
				print_num(-last_value);
			}
			else
				print_num(last_value);
			print("\n",1);
			
			uci::moveToUci(best_move);
						
			board.makeMove(best_move);
			board.compact();

#ifdef DEBUGGING
			char display[9];
			display[8] = '\n';
			for(int r = 0; r < 8; ++r)
			{
				for(int c = 0; c < 8; ++c)
					display[c] = board.at(Square(File(c), Rank(7-r))).toChar();
				print(display, 9);
			}

			clock_gettime(CLOCK_MONOTONIC, &think_time_start);
			time_allocated = 100'000'000;
#else
			time_allocated = 10'000'000'000;
#endif
			pondering = true;
			nodes = 0;
			best_move = search();
			print("Mponder_nodes=",14);
			print_num(nodes);
			print("\n",1);
			
#ifdef DEBUGGING
			clock_gettime(CLOCK_MONOTONIC, &end_think);
			x = static_cast<uint64_t>(end_think.tv_sec - think_time_start.tv_sec) * 1000000000ull + end_think.tv_nsec - think_time_start.tv_nsec;
			x /= 1000000;
			print_num(x);
			print("\n",1);
#endif
		}
	}
	
	return 0;
}