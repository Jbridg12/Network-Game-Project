/*
	Genetic Algorithm based learning to train bots to play the platforming game I developed.

	Learns a sequence of actions based on button inputs
	> Playing Phase
	- A key (Left)			1
	- D key (Right)			2
	- Spacebar (Jump)		0
	- A + Space				6
	- D + Space				7

	> Placing Phase
	- W key (Up)			3
	- A key (Left)			1
	- S key (Down)			4
	- D key (Right)			2
	- Z key (Place block)	5


*/

// NOT USING KEY INPUTS JUST SIMULATE INPUT WITH INTEGER OPTIONS


internal void init_possible_inputs()
{
	poss_playing_inputs[0] = 1;
	poss_playing_inputs[1] = 2;
	poss_playing_inputs[2] = 0;
	poss_playing_inputs[3] = 6;
	poss_playing_inputs[4] = 7;

	poss_placing_inputs[0] = 3;
	poss_placing_inputs[1] = 1;
	poss_placing_inputs[2] = 4;
	poss_placing_inputs[3] = 2; 
	poss_placing_inputs[4] = 3;
	poss_placing_inputs[5] = 1;
	poss_placing_inputs[6] = 4;
	poss_placing_inputs[7] = 2;
	poss_placing_inputs[8] = 3;
	poss_placing_inputs[9] = 1;
	poss_placing_inputs[10] = 4;
	poss_placing_inputs[11] = 2;
	poss_placing_inputs[12] = 3;
	poss_placing_inputs[13] = 1;
	poss_placing_inputs[14] = 4;
	poss_placing_inputs[15] = 2;
	poss_placing_inputs[16] = 3;
	poss_placing_inputs[17] = 1;
	poss_placing_inputs[18] = 4;
	poss_placing_inputs[19] = 2;
	poss_placing_inputs[20] = 3;
	poss_placing_inputs[21] = 1;
	poss_placing_inputs[22] = 4;
	poss_placing_inputs[23] = 2;
	poss_placing_inputs[24] = 5;

}
// ----------------------------------------------------------------------------------------------------------



typedef EA::Genetic<Chromosome, Cost> Genetic_Algorithm;
typedef EA::GenerationType<Chromosome, Cost> Generation_Type;

internal u_int* generate_random_playing_input(const std::function<double(void)>& rnd01)
{
	return &poss_playing_inputs[(int)(PLAYING_INPUTS * rnd01())];
}

internal u_int* generate_random_placing_input(const std::function<double(void)>& rnd01)
{
	return &poss_placing_inputs[(int)(PLACING_INPUTS * rnd01())];
}

internal void init_genes(Chromosome& s, const std::function<double(void)>& rnd01)
{
	for (int j = 0; j < NUMBER_OF_ROUNDS; j++)
	{
		std::list<u_int*> playing_input;
		std::list<u_int*> placing_input;
		for (int i = 0; i < STARTING_INPUT_NUMBER; i++)
		{
			playing_input.push_back(generate_random_playing_input(rnd01));
			placing_input.push_back(generate_random_placing_input(rnd01));
		}
		s.playing_inputs.push_back(playing_input);
		s.placing_inputs.push_back(placing_input);
	}

	return;
}

internal bool calculate_cost(const Chromosome& individual, Cost &cost)
{
	// RUN THE GAME 
	// Need to implement a way to call the game with all of the inputs in the bot

	cost.score = GeneticApplication(individual);

	return true;
}

internal double calculate_total_fitness(const Genetic_Algorithm::thisChromosomeType& X)
{
	
	return X.middle_costs.score;
}

internal Chromosome crossover_function(const Chromosome& x1, const Chromosome& x2, const std::function<double(void)>& rnd01)
{
	Chromosome new_chromosome;
	std::list <std::list<u_int*>> playing_inputs1 = x1.playing_inputs;
	std::list <std::list<u_int*>> placing_inputs1 = x1.placing_inputs;
	std::list <std::list<u_int*>> playing_inputs2 = x2.playing_inputs;
	std::list <std::list<u_int*>> placing_inputs2 = x2.placing_inputs;
	for (int j = 0; j < NUMBER_OF_ROUNDS; j++)
	{
		std::list<u_int*> playing_input;
		std::list<u_int*> placing_input;
		std::list<u_int*> sm_playing_inputs1 = playing_inputs1.front();
		std::list<u_int*> sm_placing_inputs1 = placing_inputs1.front();
		std::list<u_int*> sm_playing_inputs2 = playing_inputs2.front();
		std::list<u_int*> sm_placing_inputs2 = placing_inputs2.front();

		playing_inputs1.pop_front();
		placing_inputs1.pop_front();
		playing_inputs2.pop_front();
		placing_inputs2.pop_front();


		for (int i = 0; i < STARTING_INPUT_NUMBER; i++)
		{
			int rnd_num = (int)(4 * rnd01());
			switch (rnd_num)
			{
				case 0:
					playing_input.push_back(sm_playing_inputs1.front());
					placing_input.push_back(sm_placing_inputs1.front());
					break;
				case 1:
					playing_input.push_back(sm_playing_inputs1.front());
					placing_input.push_back(sm_placing_inputs2.front());
					break;
				case 2:
					playing_input.push_back(sm_playing_inputs2.front());
					placing_input.push_back(sm_placing_inputs1.front());
					break;
				case 3:
					playing_input.push_back(sm_playing_inputs2.front());
					placing_input.push_back(sm_placing_inputs2.front());
					break;
			}

			sm_playing_inputs1.pop_front();
			sm_placing_inputs1.pop_front();
			sm_playing_inputs2.pop_front();
			sm_placing_inputs2.pop_front();
		
		}

		new_chromosome.playing_inputs.push_back(playing_input);
		new_chromosome.placing_inputs.push_back(placing_input);
	}
	

	return new_chromosome;
}

internal Chromosome mutation_function(const Chromosome& x1, const std::function<double(void)>& rnd01, double shrink_scale)
{
	Chromosome new_chromosome;
	std::list <std::list<u_int*>> playing_inputs1 = x1.playing_inputs;
	std::list <std::list<u_int*>> placing_inputs1 = x1.placing_inputs;

	for (int j = 0; j < NUMBER_OF_ROUNDS; j++)
	{
		std::list<u_int*> playing_input;
		std::list<u_int*> placing_input;
		std::list<u_int*> sm_playing_inputs1 = playing_inputs1.front();
		std::list<u_int*> sm_placing_inputs1 = placing_inputs1.front();

		playing_inputs1.pop_front();
		placing_inputs1.pop_front();


		for (int i = 0; i < STARTING_INPUT_NUMBER; i++)
		{
			int rnd_num = (int)(2 * rnd01());
			if (rnd_num)
			{
				playing_input.push_back(generate_random_playing_input(rnd01));
				placing_input.push_back(generate_random_placing_input(rnd01));

			}
			else
			{
				playing_input.push_back(sm_playing_inputs1.front());
				placing_input.push_back(sm_placing_inputs1.front());
			}

			sm_playing_inputs1.pop_front();
			sm_placing_inputs1.pop_front();

		}

		new_chromosome.playing_inputs.push_back(playing_input);
		new_chromosome.placing_inputs.push_back(placing_input);
	}


	return new_chromosome;
}


internal void report_generation(int generation_number, const Generation_Type &last_generation, const Chromosome& best_genes)
{
	std::ofstream results(report_filename, std::ios::app);
	char report[256];
	sprintf_s(report, "Generation[%d]: %f", generation_number, last_generation.average_cost);
	results << string(report) << std::endl;
	results.close();

	//OutputDebugString(report);
}



internal int run_ga()
{
	init_possible_inputs();
	std::ifstream istream("AlgorithmCounter.txt");
	std::string line; 
	istream >> line;

	int app_num = atoi(line.c_str());
	istream.close();

	std::ofstream ostream("AlgorithmCounter.txt");
	ostream << std::to_string((int) !app_num);
	ostream.close();


	char report[256];
	sprintf_s(report, "Bot #%d.txt", app_num);

	report_filename = string(report);
	std::ofstream results(report_filename);
	results.close();

	EA::Chronometer timer;
	timer.tic();

	Genetic_Algorithm ga_obj;
	ga_obj.problem_mode = EA::GA_MODE::SOGA;	// State the Genetic Algorithm is aiming for a single objective.
	ga_obj.population = 20;
	ga_obj.generation_max = 1000;				// We want this to keep attempting for a long time.
	ga_obj.init_genes = init_genes;
	ga_obj.calculate_SO_total_fitness = calculate_total_fitness;
	ga_obj.eval_solution = calculate_cost;
	ga_obj.SO_report_generation = report_generation;
	ga_obj.mutate = mutation_function;
	ga_obj.crossover = crossover_function;
	ga_obj.crossover_fraction = 1;
	ga_obj.mutation_rate = 0.05;
	ga_obj.best_stall_max = 1000;
	ga_obj.average_stall_max = 1000;
	ga_obj.dynamic_threading = false;
	ga_obj.N_threads = 1;
	EA::StopReason reason = ga_obj.solve();


	char time[256];
	sprintf_s(time, "The problem is optimized in: %f seconds", timer.toc());
	OutputDebugString(time);

	return 0;

}