#pragma once
#define STARTING_INPUT_NUMBER 300
#define INPUT_TIMER 10
#define NUMBER_OF_ROUNDS 3
#define PLAYING_INPUTS 5
#define PLACING_INPUTS 25

#include <fstream>
#include <string>

struct Chromosome
{
	std::list <std::list<u_int*>> playing_inputs;
	std::list <std::list<u_int*>> placing_inputs;
};

struct Cost
{
	float score;
};

global_variable u_int poss_placing_inputs[PLACING_INPUTS];
global_variable u_int poss_playing_inputs[PLAYING_INPUTS];
global_variable std::string report_filename;