/*
Build: 
g++ -std=c++17 -O2 playoff.cpp -o playoff

Run: 
playoff [engine1] [engine2] [log_file]

*/

#include<unistd.h>
#include<sys/wait.h>
#include<sys/prctl.h>
#include<signal.h>
#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<cstdint>
#include<string>
#include<cmath>

#include "rng.cpp"
#include "openings.cpp"
#include "chess_unmodified.hpp"

#define print_log(...) printf(__VA_ARGS__);fprintf(log_file,__VA_ARGS__);fflush(log_file)

using namespace chess;

std::pair<double, double> calc_elo(int * score)
{
	double n_games = score[0] + score[1] + score[2] + score[3] + score[4] + 1;
	double p[5];
	for(int i = 0; i < 5; i++)
		p[i] = static_cast<double>(score[i] + (i==2?1:0)) / n_games;
	
	double e_score = 0.25 * p[1] + 0.5 * p[2] + 0.75 * p[3] + 1.0 * p[4];
	double v_score = p[0] * e_score * e_score + p[1] * (0.25 - e_score) * (0.25 - e_score)
		+ p[2] * (0.5 - e_score) * (0.5 - e_score) + p[3] * (0.75 - e_score) * (0.75 - e_score)
		+ p[4] * (1.0 - e_score) * (1.0 - e_score);
	double std = sqrt(v_score / n_games);
	
	double elo = log10(1.0 / e_score - 1.0) * 400.0;
	
	double score2 = e_score > 0.5 ? e_score - std : e_score + std;
	double elo2 = log10(1.0 / score2 - 1.0) * 400.0;
	
	return std::make_pair(elo, std::abs(elo-elo2));
}

int main(int argc, char** argv)
{
	unsigned int randseed = time(NULL);
	for(int i = 0; i < randseed % 65536; ++i)
		random64();
	
	if(argc < 4)
	{
		printf("Expected 3 arguments\n");
		return 0;
	}
	printf("Matching up %s and %s\n", argv[1], argv[2]);

	printf("Log file = %s\n", argv[3]);
	
	FILE* log_file = fopen(argv[3], "w");
	
	if(!log_file)
	{
		printf("Failed to open log file\n");
		return 0;
	}
	
	pid_t pid = 0;
	int inpipefd[2][2];
	int outpipefd[2][2];
	char buf[2048];
	char msg[256];
	int status;

	//Spawn child processes and set up pipes for stdin/stdout
	for(int i = 0; i < 2; ++i)
	{
		pipe(inpipefd[i]);
		pipe(outpipefd[i]);
		pid = fork();
		if (pid == 0)
		{
			// Child
			dup2(outpipefd[i][0], STDIN_FILENO);
			dup2(inpipefd[i][1], STDOUT_FILENO);

			//ask kernel to deliver SIGTERM in case the parent dies
			prctl(PR_SET_PDEATHSIG, SIGTERM);

			//close unused pipe ends
			close(outpipefd[i][1]);
			close(inpipefd[i][0]);

			execlp(argv[i+1], "", (char*) NULL);
			exit(1);
		}
		
		//close unused pipe ends
		close(outpipefd[i][0]);
		close(inpipefd[i][1]);
	}
	
	int score[5] = {0,0,0,0,0};
	for(int game_num = 0; game_num < 200; ++game_num)
	{
		//choose a game
		int opening_idx = random64() % std::size(OPENINGS);
		print_log("Game #%d Start pos (%d)=%s\n", game_num, opening_idx, OPENINGS[opening_idx]);
		int pair_score[2] = {0,0};
		
		for(int start_player = 0; start_player < 2; ++start_player)
		{
			Board board{OPENINGS[opening_idx]};
			
			std::string last_move;
			int move = 0;
			for(; board.isGameOver().second == GameResult::NONE; ++move)
			{
				int player = move % 2 ^ start_player;
				
				if(move < 2)
				{
					std::string fen = board.getFen();
					fen = "f" + fen + "\n";
					write(outpipefd[player][1], fen.c_str(), fen.size());
					sched_yield();
				}
				else
				{
					last_move = "!" + last_move + "\n"; //" " = 0ms, "!" = 20 ms, "\"" = 40 ms
					write(outpipefd[player][1], last_move.c_str(), last_move.size());
					sched_yield();
				}
				
				last_move = "";
				std::string line;
				while(last_move.size() == 0 || line.size() > 0)
				{
					int buf_idx = 0;
					int chars_remaining = read(inpipefd[player][0], buf, 2048);
					buf[chars_remaining] = 0;
					while(chars_remaining > 0)
					{
						//parse input: log lines starting with M. Otherwise, record the move into last_move
						line += buf[buf_idx];
						chars_remaining--;
						buf_idx++;
						if(line.back() == '\n')
						{
							if(line[0] == 'M')
							{
								fprintf(log_file, "Player %d move %d log: %s", player, move, line.c_str() + 1);
								fflush(log_file);
							}
							else
							{
								last_move = line;
								last_move.pop_back();
								fprintf(log_file, "Player %d move %d: %s\n", player, move, last_move.c_str());
								fflush(log_file);
								if(last_move.size() == 0)
								{
									fprintf(log_file, "Received empty move!\n");
									fflush(log_file);
									goto end;
								}
							}
							line = "";
						}
					}
				}

				//apply move
				Move m = uci::uciToMove(board, last_move);
				if(m == Move::NO_MOVE)
				{
					print_log("Bad move! %s\n", last_move.c_str());
					goto end;
				}
				board.makeMove(m);
			}
			
			//tabulate and print results
			auto [reason, result] = board.isGameOver();
			Color side = board.sideToMove();
			int side_int = static_cast<int>(side);
					
			if(result == GameResult::WIN)
			{
				print_log("%d (%s) wins", start_player ^ side_int, static_cast<std::string>(side).c_str());
				pair_score[start_player ^ side_int] += 2;
			}
			else if(result == GameResult::DRAW)
			{
				print_log("Draw");
				pair_score[0] += 1;
				pair_score[1] += 1;
			}
			else if(result == GameResult::LOSE)
			{
				print_log("%d (%s) wins", 1 ^ start_player ^ side_int, static_cast<std::string>(~side).c_str());
				pair_score[1 ^ start_player ^ side_int] += 2;
			}
						
			switch(reason)
			{
			case GameResultReason::CHECKMATE:
				print_log(" by mate");
				break;
			case GameResultReason::STALEMATE:
				print_log(" by stalemate");
				break;
			case GameResultReason::INSUFFICIENT_MATERIAL:
				print_log(" by insufficient");
				break;
			case GameResultReason::FIFTY_MOVE_RULE:
				print_log(" by 50 move rule");
				break;
			case GameResultReason::THREEFOLD_REPETITION:
				print_log(" by repetition");
				break;
			default:
				print_log(" by UNKNOWN");
				goto end;
			}
			
			print_log(" in %d plies\n", move);
		}
		
		//print running score
		score[pair_score[1] - pair_score[0] + 4 >> 1]++;
		auto [elo, elo_std] = calc_elo(score);		
		print_log("Games: %d Pair score: %d %d Score: %d %d %d %d %d Elo: %.2f +- %.2f\n", game_num, pair_score[0], pair_score[1], score[0], score[1], score[2], score[3], score[4], elo, elo_std);
		fflush(log_file);
	}

end:
	fclose(log_file);
	kill(pid, SIGKILL); //send SIGKILL signal to the child process
	waitpid(pid, &status, 0);
	
	return 0;
}