/*
Build: g++ -std=c++20 -O2 trainer.cpp -o trainer
*/

#define PRINT_EVAL 0

#include "nnue_training_data_formats.h"

#include <cstdio>
#include <vector>
#include "rng.cpp"

using namespace chess;

//fit to win rate(eval) + small factor * eval
float obj_func(float x, float absolute_score_coeff)
{
	const float a = 115.39f / 0.4807692307692308f;
	const float b = 1.0f / 47.39f * 0.4807692307692308f;
	const float x_clamp = std::clamp(x, -5000.0f, 5000.0f);
	return 1.0f / (1.0f + expf((a-x_clamp)*b)) - 1.0f / (1.0f + expf((a+x_clamp)*b)) + absolute_score_coeff * x;
}

std::pair<float, float> obj_func_deriv(float x, float absolute_score_coeff)
{
	const float a = 115.39f / 0.4807692307692308f;
	const float b = 1.0f / 47.39f * 0.4807692307692308f;
	const float x_clamp = std::clamp(x, -5000.0f, 5000.0f);
	const float d1 = (1.0f + expf((a-x_clamp)*b));
	const float d2 = (1.0f + expf((a+x_clamp)*b));

	return std::pair(
		1.0f / d1 - 1.0f / d2  + absolute_score_coeff * x,
		b * expf((a-x_clamp)*b) / (d1 * d1) + b*expf((a+x_clamp)*b) / (d2 * d2) + absolute_score_coeff
	);
}

std::pair<float, float> loss_func_deriv(float x, float target, float absolute_score_coeff)
{
	auto [z, dz] = obj_func_deriv(x, absolute_score_coeff);
	auto t = obj_func(target, absolute_score_coeff);
	float diff = z - t;
	
	//Squared error
	//return std::pair(diff * diff, 2.0f * diff * dz);
	
	//2.5th power error
	float sq = std::sqrt(std::abs(diff));
	return std::pair(diff* diff * sq, 2.5f * diff * sq * dz);
}

////////////////////////////////////////////////////////////////////////////////
//AD
////////////////////////////////////////////////////////////////////////////////

class AD
{
public:
	std::vector<float> values;
	std::vector<float> derivative;
	
	AD(int size) :
		values(size, 0.0f),
		derivative(size, 0.0f)
	{
	}
	
	int size()
	{
		return values.size();
	}
	
	virtual void forward() = 0;
	virtual void update_parent_deriv() = 0;
	virtual bool pool_derivative()
	{
		return false;
	}
};

class ADConst : public AD
{
public:
	ADConst() :
		AD(0)
	{}
	
	ADConst(int size) :
		AD(size)
	{
	}

	void forward() override
	{
	}
	
	void update_parent_deriv() override
	{
	}
	
	bool pool_derivative()
	{
		return true;
	}
};

class ADAdd : public AD
{
	AD& lhs;
	AD&	rhs;
public:
	ADAdd(AD& _lhs, AD& _rhs) :
		AD(_lhs.size()),
		lhs(_lhs),
		rhs(_rhs)
	{
		if(lhs.size() != rhs.size())
			throw std::runtime_error("RHS size != LHS Size");
	}
	
	void forward() override
	{
		for(int i = 0; i < size(); ++i)
			values[i] = lhs.values[i] + rhs.values[i];
	}
	
	void update_parent_deriv() override
	{
		for(int i = 0; i < size(); ++i)
		{
			lhs.derivative[i] += derivative[i];
			rhs.derivative[i] += derivative[i];
		}
	}
};

class ADMult : public AD
{
	AD& lhs;
	AD&	rhs;
public:
	ADMult(AD& _lhs, AD& _rhs) :
		AD(_lhs.size()),
		lhs(_lhs),
		rhs(_rhs)
	{
		if(lhs.size() != rhs.size())
			throw std::runtime_error("RHS size != LHS Size");
	}
	
	void forward() override
	{
		for(int i = 0; i < size(); ++i)
			values[i] = lhs.values[i] * rhs.values[i];
	}
	
	void update_parent_deriv() override
	{
		for(int i = 0; i < size(); ++i)
		{
			lhs.derivative[i] += rhs.values[i] * derivative[i];
			rhs.derivative[i] += lhs.values[i] * derivative[i];
		}
	}
};


class ADSumReduce : public AD
{
	AD& parent;
public:
	ADSumReduce(AD& _parent) :
		AD(1),
		parent(_parent)
	{
	}
	
	void forward() override
	{
		values[0] = 0.0f;
		for(auto x : parent.values)
			values[0] += x;
	}
	
	void update_parent_deriv() override
	{
		for(auto &x : parent.derivative)
			x += derivative[0];
	}
};

class ADL1 : public AD
{
	AD& parent;
public:
	ADL1(AD& _parent) :
		AD(1),
		parent(_parent)
	{
	}
	
	void forward() override
	{
		values[0] = 0.0f;
		for(auto x : parent.values)
			values[0] += std::abs(x);
	}
	
	void update_parent_deriv() override
	{
		for(int i = 0; i < parent.size(); ++i)
			if(parent.values[i] != 0)
				parent.derivative[i] += parent.values[i] < 0.0f ? -derivative[0] : derivative[0];
	}
};

class ADSparseSum : public AD
{
	AD& parent;
public:
	std::vector<int> indices;
	
	ADSparseSum(AD& _parent) :
		AD(1),
		parent(_parent)
	{
	}
	
	void forward() override
	{
		values[0] = 0.0f;
		for(auto i : indices)
			values[0] += parent.values[i];
	}
	
	void update_parent_deriv() override
	{
		for(auto i : indices)
			parent.derivative[i] += derivative[0];
	}
};

//Warning: Mutable size!!
class ADSparseConcat : public AD
{
	AD& parent;
	int n;
public:
	std::vector<int> indices;
	
	ADSparseConcat(AD& _parent, int _n) :
		AD(_n),
		parent(_parent),
		n(_n)
	{
	}
	
	void forward() override
	{
		values.resize(0);
		derivative.resize(0);
		for(auto i : indices)
			for(int j = 0; j < n; ++j)
			{
				//printf("%d %d %d\n", i, j, i*n + j);
				values.push_back(parent.values[i * n + j]);
				derivative.push_back(0.0f);
			}
	}
	
	void update_parent_deriv() override
	{
		int idx = 0;
		for(auto i : indices)
			for(int j = 0; j < n; ++j)
				parent.derivative[i * n + j] += derivative[idx++];
	}
};

class ADSparseProduct : public AD
{
	AD& parent;
public:
	std::vector<int> indices;
	
	ADSparseProduct(AD& _parent, int n) :
		AD(n),
		parent(_parent)
	{
	}
	
	void forward() override
	{
		for(int j = 0; j < size(); ++j)
			values[j] = 0.0f;
		
		for(auto i : indices)
			for(int j = 0; j < size(); ++j)
				values[j] += parent.values[i * size() + j];
	}
	
	void update_parent_deriv() override
	{
		for(auto i : indices)
			for(int j = 0; j < size(); ++j)
				parent.derivative[i * size() + j] += derivative[j];
	}
};

class ADDenseProduct : public AD
{
	AD& vector;
	AD& matrix;
public:
	ADDenseProduct(AD& _vector, AD& _matrix, int n_out) :
		AD(n_out),
		vector(_vector),
		matrix(_matrix)
	{
		if(matrix.size() != n_out * vector.size())
			printf("Wrong size input for ADDenseProduct\n");
	}
	
	void forward() override
	{
		for(int j = 0; j < size(); ++j)
		{
			values[j] = 0.0f;
			for(int i = 0; i < vector.size(); ++i)
				values[j] += vector.values[i] * matrix.values[j * vector.size() + i];
		}
	}
	
	void update_parent_deriv() override
	{
		//if(derivative[0] != 0.0f || derivative[1] != 0.0f)
		//	printf("backward: %f %f\n", derivative[0], derivative[1]);
		for(int j = 0; j < size(); ++j)
		{
			for(int i = 0; i < vector.size(); ++i)
			{
				vector.derivative[i] += derivative[j] * matrix.values[j * vector.size() + i];
				matrix.derivative[j * vector.size() + i] += derivative[j] * vector.values[i];
			}
		}
	}		
};

class ADReLu : public AD
{
	AD& parent;
public:
	ADReLu(AD& _parent) :
		AD(_parent.size()),
		parent(_parent)
	{
	}

	void forward() override
	{
		for(int i = 0; i < size(); ++i)
			values[i] = std::max(0.0f, parent.values[i]);
	}
	
	void update_parent_deriv() override
	{
		for(int i = 0; i < size(); ++i)
			parent.derivative[i] += values[i] ? derivative[i] : 0.0f;
	}
};

class ADCReLu : public AD
{
	AD& parent;
public:
	ADCReLu(AD& _parent) :
		AD(_parent.size()),
		parent(_parent)
	{
	}

	void forward() override
	{
		for(int i = 0; i < size(); ++i)
			values[i] = std::clamp(parent.values[i], 0.0f, 1.0f);
	}
	
	void update_parent_deriv() override
	{
		for(int i = 0; i < size(); ++i)
			parent.derivative[i] += values[i] > 0.0f && values[i] < 1.0f ? derivative[i] : 0.0f;
	}
};


class ADSReLu : public AD
{
	AD& parent;
public:
	ADSReLu(AD& _parent) :
		AD(_parent.size()),
		parent(_parent)
	{
	}

	void forward() override
	{
		for(int i = 0; i < size(); ++i)
		{
			if(parent.values[i] < 0.0f)
				values[i] = 0.0f;
			else if(parent.values[i] > 1.0f)
				values[i] = parent.values[i] - 0.5f;
			else
				values[i] = parent.values[i] * parent.values[i] * 0.5f;
		}
	}
	
	void update_parent_deriv() override
	{
		for(int i = 0; i < size(); ++i)
		{
			if(parent.values[i] < 0.0f)
				continue;
			else if(parent.values[i] > 1.0f)
				parent.derivative[i] += derivative[i];
			else
				parent.derivative[i] += parent.values[i] * derivative[i];
		}
	}
};

class ADLossFunc : public AD
{
	AD& parent;
	float abs_coeff;
public:
	float target; //overwrite this each time
	
	ADLossFunc(AD& _parent, float _abs_coeff) :
		AD(1),
		parent(_parent),
		abs_coeff(_abs_coeff)
	{
		if(parent.size() != 1)
			throw std::runtime_error("Parent should have size 1 for ADLossFunc");
	}
	
	void forward() override
	{
		auto [loss, dloss] = loss_func_deriv(parent.values[0], target, abs_coeff);
		values[0] = loss;
	}
	
	void update_parent_deriv() override
	{
		auto [loss, dloss] = loss_func_deriv(parent.values[0], target, abs_coeff);
		parent.derivative[0] += dloss * derivative[0];
	}
};

class ADChooser : public AD
{
	AD **parents;
public:
	int index;
	ADChooser(AD **_parents) :
		AD(_parents[0]->size()),
		parents(_parents)
	{
	}

	void forward() override
	{
		for(int j = 0; j < size(); ++j)
			values[j] = parents[index]->values[j];
	}
	
	void update_parent_deriv() override
	{
		for(int j = 0; j < size(); ++j)
			parents[index]->derivative[j] += derivative[j];
	}
};

void ad_calc_derivs(std::vector<AD*> & ad_stack)
{
	if(ad_stack.back()->derivative.size() != 1)
		throw std::runtime_error("Wrong size AD output");
	
	//Initialize derivatives & do forward pass
	for(int i = 0; i < ad_stack.size(); ++i)
	{
		ad_stack[i]->forward();
		if(!ad_stack[i]->pool_derivative())
			for(auto & x : ad_stack[i]->derivative)
				x = 0.0f;
	}
	ad_stack.back()->derivative[0] = 1.0f;
		
	//Backward pass
	for(int i = ad_stack.size() - 1; i >= 0; --i)
	{
		ad_stack[i]->update_parent_deriv();
	}
}

void ad_calc_values(std::vector<AD*> & ad_stack)
{
	//Do forward pass
	for(int i = 0; i < ad_stack.size(); ++i)
	{
		ad_stack[i]->forward();
	}
}

////////////////////////////////////////////////////////////////////////////////
//Evaluators
////////////////////////////////////////////////////////////////////////////////

std::vector<binpack::TrainingDataEntry> validation_data;

struct Evaluator_PieceValue
{
	alignas(32) float piece_values[12];
	alignas(32) float gradient[12];
	alignas(32) float input_layer[12];
	float abs_coeff = .0001f;
	
	void init()
	{
		for(int i = 0; i < 12; ++i)
			piece_values[i] = 0.0f;
	}
	
	void process_input(const binpack::TrainingDataEntry& entry)
	{
		const Piece * pieces = entry.pos.piecesRaw();
		for(int i = 0; i < 12; ++i)
			input_layer[i] = 0.0f;
		if(entry.pos.sideToMove() == Color::White)
		{
			for(int j = 0; j < 64; ++j)
				input_layer[static_cast<int>(pieces[j])] += 1.0f;
		}
		else
		{
			for(int j = 0; j < 64; ++j)
				input_layer[static_cast<int>(pieces[j ^ 56]) ^ 1] += 1.0f;
		}
	}
	
	void update_gradient(const binpack::TrainingDataEntry& entry)
	{
		process_input(entry);
				
		float eval = 0;
		for(int i = 0; i < 12; ++i)
			eval += input_layer[i] * piece_values[i];
		
		auto [loss, dloss] = loss_func_deriv(eval, entry.score, abs_coeff);
		for(int i = 0; i < 12; ++i)
			gradient[i] += dloss * input_layer[i];
	}
	
	void step(float step_size)
	{
		float grad_length = 0.0f;
		for(int i =  0; i < 12; ++i)
			grad_length += gradient[i] * gradient[i];
		
		float mult = step_size / sqrt(grad_length);
		
		for(int i = 0; i < 12; ++i)
		{
			piece_values[i] -= gradient[i] * mult;
		}
	}
	
	void decay_gradient()
	{
		for(int i = 0; i < 12; ++i)
			gradient[i] *= 0.0f;
	}
	
	void test()
	{
		printf("Piece values:");
		for(int i = 0; i < 12; ++i)
			printf(" %.2f", piece_values[i]);
		printf("\n");

		float loss = 0.0f, loss2 = 0.0f;
		float n = 0.0f;
		
		for(auto & entry : validation_data)
		{
			process_input(entry);
			float eval = 0.0f;
			for(int i = 0; i < 12; ++i)
				eval += input_layer[i] * piece_values[i];
			
			loss += (eval - entry.score) * (eval - entry.score);
			n += 1.0f;
			
			auto [loss2_i, dloss2_i] = loss_func_deriv(eval, entry.score, abs_coeff);
			loss2 += loss2_i;
		}
		printf("RMSE: %.6f WDL: %.6f n=%.0f\n", sqrt(loss / n), loss2 / n, n);		
	}
};

struct Evaluator_PST
{
	alignas(32) float piece_values[12*64];
	alignas(32) float gradient[12*64];
	alignas(32) float input_layer[12*64];
	float abs_coeff = .0001f;
	
	void init()
	{
		for(int i = 0; i < 12*64; ++i)
			piece_values[i] = 0.0f;
	}
	
	void process_input(const binpack::TrainingDataEntry& entry)
	{
		const Piece * pieces = entry.pos.piecesRaw();
		for(int i = 0; i < 12*64; ++i)
			input_layer[i] = 0.0f;
		if(entry.pos.sideToMove() == Color::White)
		{
			for(int j = 0; j < 64; ++j)
				if(pieces[j] != Piece::none())
					input_layer[static_cast<int>(pieces[j]) * 64 + j] += 1.0f;
		}
		else
		{
			for(int j = 0; j < 64; ++j)
				if(pieces[j] != Piece::none())
					input_layer[(static_cast<int>(pieces[j])^1) * 64 + (j ^ 56)] += 1.0f;
		}
	}
	
	void update_gradient(const binpack::TrainingDataEntry& entry)
	{
		process_input(entry);
				
		float eval = 0;
		for(int i = 0; i < 12*64; ++i)
			eval += input_layer[i] * piece_values[i];
		
		auto [loss, dloss] = loss_func_deriv(eval, entry.score, abs_coeff);
		for(int i = 0; i < 12*64; ++i)
			gradient[i] += dloss * input_layer[i];
	}
	
	void step(float step_size)
	{
		float grad_length = 0.0f;
		for(int i=  0; i < 12*64; ++i)
			grad_length += gradient[i] * gradient[i];
		
		float mult = step_size / sqrt(grad_length);
		
		for(int i = 0; i < 12*64; ++i)
		{
			piece_values[i] -= gradient[i] * mult;
		}
	}
	
	void decay_gradient()
	{
		for(int i = 0; i < 12*64; ++i)
			gradient[i] *= 0.0f;
	}
	
	void test()
	{
		printf("Piece values:");
		for(int i = 0; i < 12; ++i)
		{
			float avg = 0.0f;
			for(int j = 0; j < 64; ++j)
				avg += piece_values[i*64 + j];
			printf(" %.2f", avg / 64.0f);
		}
		printf("\n");

		float loss = 0.0f, loss2 = 0.0f;
		float n = 0.0f;
		
		for(auto & entry : validation_data)
		{
			process_input(entry);
			float eval = 0.0f;
			for(int i = 0; i < 12*64; ++i)
				eval += input_layer[i] * piece_values[i];
			
			loss += (eval - entry.score) * (eval - entry.score);
			n += 1.0f;
			
			auto [loss2_i, dloss2_i] = loss_func_deriv(eval, entry.score, abs_coeff);
			loss2 += loss2_i;
		}
		printf("RMSE: %.6f WDL: %.6f n=%.0f\n", sqrt(loss / n), loss2 / n, n);		
	}
};

struct Evaluator_PST_AD
{
	ADSparseSum input;
	ADConst PST;
	ADLossFunc loss;
	std::vector<AD*> ad_stack;
	constexpr static float abs_coeff = .0001f;
	
	Evaluator_PST_AD() :
		PST(768),
		input(PST),
		loss(input, abs_coeff)
	{
		ad_stack.push_back(&PST);
		ad_stack.push_back(&input);
		ad_stack.push_back(&loss);
	}
		
	void process_input(const binpack::TrainingDataEntry& entry)
	{
		const Piece * pieces = entry.pos.piecesRaw();
		input.indices.clear();
		if(entry.pos.sideToMove() == Color::White)
		{
			for(int j = 0; j < 64; ++j)
				if(pieces[j] != Piece::none())
					input.indices.push_back(static_cast<int>(pieces[j]) * 64 + j);
		}
		else
		{
			for(int j = 0; j < 64; ++j)
				if(pieces[j] != Piece::none())
					input.indices.push_back((static_cast<int>(pieces[j])^1) * 64 + (j ^ 56));
		}
	}
	
	void update_gradient(const binpack::TrainingDataEntry& entry)
	{
		process_input(entry);
		loss.target = entry.score;
		ad_calc_derivs(ad_stack);
	}
	
	void step(float step_size)
	{
		float grad_length = 0.0f;
		for(int i =  0; i < PST.size(); ++i)
		{
			grad_length += PST.derivative[i] * PST.derivative[i];
		}

		float mult = step_size / sqrt(grad_length);
		if(std::isinf(mult) || std::isnan(mult))
			throw std::runtime_error("Bad mult " + std::to_string(mult) + " " + std::to_string(step_size) + " " + std::to_string(grad_length));
		
		for(int i = 0; i < PST.size(); ++i)
		{
			PST.values[i] -= PST.derivative[i] * mult;
		}
	}
	
	void decay_gradient()
	{
		for(auto & x : PST.derivative)
			x = 0.0f;
	}
	
	void test()
	{
		printf("Piece values:");
		for(int i = 0; i < 12; ++i)
		{
			float avg = 0.0f;
			for(int j = 0; j < 64; ++j)
				avg += PST.values[i*64 + j];
			printf(" %.2f", avg / 64.0f);
		}
		printf("\n");

		float l = 0.0f;
		float n = 0.0f;
		
		for(auto & entry : validation_data)
		{
			process_input(entry);
			loss.target = entry.score;
			ad_calc_values(ad_stack);
			l += loss.values[0];
			n += 1.0f;
		}
		printf("Loss: %.6f n=%.0f\n", l / n, n);		
	}
};

struct Evaluator_NNUE1
{
	ADConst PST;
	ADConst PT;
	ADConst bias;
	ADConst l2_weights;
	ADConst l2_bias;
	ADSparseProduct input;
	ADSparseProduct input_PT;
	ADLossFunc* loss;
	std::vector<AD*> ad_stack;
	std::vector<ADConst*> all_data;
	constexpr static float abs_coeff = .0001f;
	
	Evaluator_NNUE1(int hidden_size) :
		PT(12 * hidden_size),
		PST(768 * hidden_size),
		bias(hidden_size),
		input(PST, hidden_size),
		input_PT(PT, hidden_size),
		l2_weights(hidden_size),
		l2_bias(1)
	{
		ad_stack.push_back(&PT);
		ad_stack.push_back(&PST);
		ad_stack.push_back(&bias);
		ad_stack.push_back(&input);
		ad_stack.push_back(&input_PT);
		ad_stack.push_back(&l2_weights);
		ad_stack.push_back(&l2_bias);
		
		//factorize code
		//ADAdd *input_total = new ADAdd(input_PT, input);
		//ad_stack.push_back(input_total);
		
		//ADAdd *l1 = new ADAdd(*input_total, bias);
		ADAdd *l1 = new ADAdd(input, bias);
		ad_stack.push_back(l1);
		
		ADReLu *relu1 = new ADReLu(*l1);
		ad_stack.push_back(relu1);
		
		ADMult *l2 = new ADMult(*relu1, l2_weights);
		ad_stack.push_back(l2);
		
		ADSumReduce *output = new ADSumReduce(*l2);
		ad_stack.push_back(output);
		
		ADAdd *final_output = new ADAdd(*output, l2_bias);
		ad_stack.push_back(final_output);
		
		loss = new ADLossFunc(*final_output, abs_coeff);
		// auto temp = new ADSumReduce(input);
		// ad_stack.push_back(temp);
		// loss = new ADLossFunc(*temp, abs_coeff);
		ad_stack.push_back(loss);
		
		all_data.push_back(&PT);
		all_data.push_back(&PST);
		all_data.push_back(&bias);
		all_data.push_back(&l2_weights);
		all_data.push_back(&l2_bias);
		
		for(auto data : all_data)
			for(auto & x : data->values)
				x = (random64() & 65535) / 6553.5f - 5.0f;
	}
		
	void process_input(const binpack::TrainingDataEntry& entry)
	{
		const Piece * pieces = entry.pos.piecesRaw();
		input_PT.indices.clear();
		input.indices.clear();
		if(entry.pos.sideToMove() == Color::White)
		{
			for(int j = 0; j < 64; ++j)
				if(pieces[j] != Piece::none())
				{
					input_PT.indices.push_back(static_cast<int>(pieces[j]));
					input.indices.push_back(static_cast<int>(pieces[j]) * 64 + j);
				}
		}
		else
		{
			for(int j = 0; j < 64; ++j)
				if(pieces[j] != Piece::none())
				{
					input_PT.indices.push_back(static_cast<int>(pieces[j])^1);
					input.indices.push_back((static_cast<int>(pieces[j])^1) * 64 + (j ^ 56));
				}
		}
	}
	
	void update_gradient(const binpack::TrainingDataEntry& entry)
	{
		process_input(entry);
		loss->target = entry.score;
		ad_calc_derivs(ad_stack);
	}
	
	void step(float step_size)
	{
		float grad_length = 0.0f;
		
		for(auto data : all_data)
		{
			for(int i =  0; i < data->size(); ++i)
			{
				grad_length += data->derivative[i] * data->derivative[i];
			}
		}

		float mult = step_size / sqrt(grad_length);
		if(std::isinf(mult) || std::isnan(mult))
			throw std::runtime_error("Bad mult " + std::to_string(mult) + " " + std::to_string(step_size) + " " + std::to_string(grad_length));
		
		for(auto data : all_data)
		{
			for(int i = 0; i < data->size(); ++i)
			{
				data->values[i] -= data->derivative[i] * mult;
			}
		}
	}
	
	void decay_gradient()
	{
		for(auto data : all_data)
			for(auto & x : data->derivative)
				x = 0.0f;
	}
	
	void test()
	{
		float l = 0.0f;
		float n = 0.0f;
		
		for(auto & entry : validation_data)
		{
			process_input(entry);
			loss->target = entry.score;
			ad_calc_values(ad_stack);
			l += loss->values[0];
			n += 1.0f;
		}
		printf("Loss: %.6f n=%.0f\n", l / n, n);
	}
	
	void write(std::string & file_name)
	{
		FILE* f = fopen(file_name.c_str(), "w");
		if(!f)
		{
			printf("ERROR: Could not save to log file\n");
			return;
		}
		for(auto data: all_data)
			for(auto x : data->values)
				fprintf(f, "%.10f\n", x);
			
		fclose(f);
	}
};

//ReLu + residual
struct Evaluator_NNUE1r
{
	ADConst PST;
	ADConst l1_bias;
	ADConst l1_residual;
	ADConst l2_weights;
	ADConst l2_bias;
	ADSparseProduct input;
	ADLossFunc* loss;
	std::vector<AD*> ad_stack;
	std::vector<ADConst*> all_data;
	constexpr static float abs_coeff = .0001f;
	
	Evaluator_NNUE1r(int hidden_size) :
		PST(768 * hidden_size),
		l1_bias(hidden_size),
		l1_residual(hidden_size),		
		input(PST, hidden_size),
		l2_weights(hidden_size),
		l2_bias(1)
	{
		ad_stack.push_back(&PST);
		ad_stack.push_back(&l1_bias);
		ad_stack.push_back(&l1_residual);
		ad_stack.push_back(&input);
		ad_stack.push_back(&l2_weights);
		ad_stack.push_back(&l2_bias);
		
		ADAdd *l1 = new ADAdd(input, l1_bias);
		ad_stack.push_back(l1);
		
		ADReLu *relu1 = new ADReLu(*l1);
		ad_stack.push_back(relu1);
		
		ADMult *resid1 = new ADMult(input, l1_residual);
		ad_stack.push_back(resid1);
		
		ADAdd *l1_out = new ADAdd(*resid1, *relu1);	
		ad_stack.push_back(l1_out);
		
		ADMult *l2 = new ADMult(*l1_out, l2_weights);
		ad_stack.push_back(l2);
		
		ADSumReduce *output = new ADSumReduce(*l2);
		ad_stack.push_back(output);
		
		ADAdd *final_output = new ADAdd(*output, l2_bias);
		ad_stack.push_back(final_output);
		
		loss = new ADLossFunc(*final_output, abs_coeff);
		// auto temp = new ADSumReduce(input);
		// ad_stack.push_back(temp);
		// loss = new ADLossFunc(*temp, abs_coeff);
		ad_stack.push_back(loss);
		
		all_data.push_back(&l1_residual);
		all_data.push_back(&PST);
		all_data.push_back(&l1_bias);
		all_data.push_back(&l2_weights);
		all_data.push_back(&l2_bias);
		
		int params = 0;
		for(auto data : all_data)
		{
			printf("Layer with %d params\n", data->size());
			for(auto & x : data->values)
			{
				params++;
				x = (random64() & 65535) / 6553.5f - 5.0f;
			}
		}
		
		for(auto & x : l1_residual.values)
			x = 0.0f;
			
		printf("Network has %d parameters\n", params);
	}
		
	void process_input(const binpack::TrainingDataEntry& entry)
	{
		const Piece * pieces = entry.pos.piecesRaw();
		input.indices.clear();
		if(entry.pos.sideToMove() == Color::White)
		{
			for(int j = 0; j < 64; ++j)
				if(pieces[j] != Piece::none())
				{
					input.indices.push_back(static_cast<int>(pieces[j]) * 64 + j);
				}
		}
		else
		{
			for(int j = 0; j < 64; ++j)
				if(pieces[j] != Piece::none())
				{
					input.indices.push_back((static_cast<int>(pieces[j])^1) * 64 + (j ^ 56));
				}
		}
	}
	
	void update_gradient(const binpack::TrainingDataEntry& entry)
	{
		process_input(entry);
		loss->target = entry.score;
		ad_calc_derivs(ad_stack);
	}
	
	void step(float step_size)
	{
		float grad_length = 0.0f;
		
		for(auto data : all_data)
		{
			for(int i =  0; i < data->size(); ++i)
			{
				grad_length += data->derivative[i] * data->derivative[i];
			}
		}

		float mult = step_size / sqrt(grad_length);
		if(std::isinf(mult) || std::isnan(mult))
			throw std::runtime_error("Bad mult " + std::to_string(mult) + " " + std::to_string(step_size) + " " + std::to_string(grad_length));
		
		int a = 0;
		for(auto data : all_data)
		{
			for(int i = 0; i < data->size(); ++i)
			{
				if(a == 0)
				{
					data->values[i] -= data->derivative[i] * mult * .1f;
					//data->values[i] = std::clamp(data->values[i], -0.0001f, 0.0001f);
				}
				else
					data->values[i] -= data->derivative[i] * mult;
			}
			++a;
		}
	}
	
	void decay_gradient()
	{
		for(auto data : all_data)
			for(auto & x : data->derivative)
				x = 0.0f;
	}
	
	void test()
	{
		float l = 0.0f;
		float n = 0.0f;
		
		for(auto & entry : validation_data)
		{
			process_input(entry);
			loss->target = entry.score;
			ad_calc_values(ad_stack);
			l += loss->values[0];
			n += 1.0f;
		}
		printf("Loss: %.6f n=%.0f\n", l / n, n);
	}
	
	void write(std::string & file_name)
	{
		FILE* f = fopen(file_name.c_str(), "w");
		if(!f)
		{
			printf("ERROR: Could not save to log file\n");
			return;
		}
		for(auto data: all_data)
			for(auto x : data->values)
				fprintf(f, "%.10f\n", x);
			
		fclose(f);
	}
};

struct Evaluator_NNUE1_Sparse
{
	ADConst PST;
	ADConst l1_bias;
	ADConst l2_weights;
	ADConst l2_bias;
	ADConst regulator;
	ADSparseProduct input;
	ADSparseConcat input_concat;
	ADLossFunc* loss;
	ADAdd* loss_plus_l1;
	std::vector<AD*> ad_stack;
	std::vector<ADConst*> all_data;
	constexpr static float abs_coeff = .0001f;
	
	Evaluator_NNUE1_Sparse(int hidden_size, float _regulator) :
		PST(768 * hidden_size),
		l1_bias(hidden_size),
		input(PST, hidden_size),
		input_concat(PST, hidden_size),
		l2_weights(hidden_size),
		l2_bias(1),
		regulator(1)
	{
		ad_stack.push_back(&PST);
		ad_stack.push_back(&l1_bias);
		ad_stack.push_back(&input);
		ad_stack.push_back(&input_concat);
		ad_stack.push_back(&l2_weights);
		ad_stack.push_back(&l2_bias);
		ad_stack.push_back(&regulator);
		regulator.values[0] = _regulator;
		
		ADAdd *l1 = new ADAdd(input, l1_bias);
		ad_stack.push_back(l1);
		
		ADReLu *relu1 = new ADReLu(*l1);
		ad_stack.push_back(relu1);
		
		ADMult *l2 = new ADMult(*relu1, l2_weights);
		ad_stack.push_back(l2);
		
		ADSumReduce *output = new ADSumReduce(*l2);
		ad_stack.push_back(output);
		
		ADAdd *final_output = new ADAdd(*output, l2_bias);
		ad_stack.push_back(final_output);
		
		loss = new ADLossFunc(*final_output, abs_coeff);
		ad_stack.push_back(loss);
		
		ADL1 *l1_loss_pre = new ADL1(input_concat);
		ad_stack.push_back(l1_loss_pre);
		
		ADMult *l1_loss = new ADMult(*l1_loss_pre, regulator);
		ad_stack.push_back(l1_loss);
		
		loss_plus_l1 = new ADAdd(*loss, *l1_loss);
		ad_stack.push_back(loss_plus_l1);
		
		all_data.push_back(&PST);
		all_data.push_back(&l1_bias);
		all_data.push_back(&l2_weights);
		all_data.push_back(&l2_bias);
		
		int params = 0;
		for(auto data : all_data)
		{
			printf("Layer with %d params\n", data->size());
			for(auto & x : data->values)
			{
				params++;
				x = (random64() & 65535) / 6553.5f - 5.0f;
			}
		}
		
		printf("Network has %d parameters\n", params);
	}
		
	void process_input(const binpack::TrainingDataEntry& entry)
	{
		const Piece * pieces = entry.pos.piecesRaw();
		input_concat.indices.clear();
		input.indices.clear();
		if(entry.pos.sideToMove() == Color::White)
		{
			for(int j = 0; j < 64; ++j)
				if(pieces[j] != Piece::none())
				{
					input_concat.indices.push_back(static_cast<int>(pieces[j]) * 64 + j);
					input.indices.push_back(static_cast<int>(pieces[j]) * 64 + j);
				}
		}
		else
		{
			for(int j = 0; j < 64; ++j)
				if(pieces[j] != Piece::none())
				{
					input_concat.indices.push_back((static_cast<int>(pieces[j])^1) * 64 + (j ^ 56));
					input.indices.push_back((static_cast<int>(pieces[j])^1) * 64 + (j ^ 56));
				}
		}
	}
	
	void update_gradient(const binpack::TrainingDataEntry& entry)
	{
		process_input(entry);
		loss->target = entry.score;
		ad_calc_derivs(ad_stack);
	}
	
	void step(float step_size)
	{
		float grad_length = 0.0f;
		
		for(auto data : all_data)
		{
			for(int i =  0; i < data->size(); ++i)
			{
				grad_length += data->derivative[i] * data->derivative[i];
			}
		}

		float mult = step_size / sqrt(grad_length);
		if(std::isinf(mult) || std::isnan(mult))
			throw std::runtime_error("Bad mult " + std::to_string(mult) + " " + std::to_string(step_size) + " " + std::to_string(grad_length));
		
		for(auto data : all_data)
		{
			for(int i = 0; i < data->size(); ++i)
			{
				//data->values[i] -= data->derivative[i] * mult;
				float before = data->values[i];
				float after = before - data->derivative[i] * mult;
				if(before < 0.0f)
					after = std::min(0.0f, after);
				if(before > 0.0f)
					after = std::max(0.0f, after);
				if(before == 0.0f)
					after = std::clamp(after, -1e-9f, 1e-9f);
				data->values[i] = after;
			}
		}
	}
	
	void decay_gradient()
	{
		for(auto data : all_data)
			for(auto & x : data->derivative)
				x = 0.0f;
	}
	
	void test()
	{
		float l = 0.0f;
		float l1 = 0.0f;
		float n = 0.0f;
		
		for(auto & entry : validation_data)
		{
			process_input(entry);
			loss->target = entry.score;
			ad_calc_values(ad_stack);
			l += loss->values[0];
			l1 += loss_plus_l1->values[0];
			n += 1.0f;
		}
		
		int zeros = 0, nonzeros = 0;
		for(int i = 0; i < PST.size(); ++i)
		{
			if(std::abs(PST.values[i]) < 1e-8f)
				zeros++;
			else
				nonzeros++;
		}
		printf("Loss: %.6f Loss+L1: %.6f Sparse: %d/%d\n", l / n, l1 / n, zeros, nonzeros);
	}
	
	void write(std::string & file_name)
	{
		FILE* f = fopen(file_name.c_str(), "w");
		if(!f)
		{
			printf("ERROR: Could not save to log file\n");
			return;
		}
		for(auto data: all_data)
			for(auto x : data->values)
				fprintf(f, "%.10f\n", x);
			
		fclose(f);
	}
};

struct Evaluator_NNUE1_Sparse_Phase2
{
	ADConst PST;
	ADConst l1_bias;
	ADConst l2_weights;
	ADConst l2_bias;
	ADSparseProduct input;
	ADLossFunc* loss;
	std::vector<AD*> ad_stack;
	std::vector<ADConst*> all_data;
	constexpr static float abs_coeff = .0001f;
	
	Evaluator_NNUE1_Sparse_Phase2(int hidden_size) :
		PST(768 * hidden_size),
		l1_bias(hidden_size),
		input(PST, hidden_size),
		l2_weights(hidden_size),
		l2_bias(1)
	{
		ad_stack.push_back(&PST);
		ad_stack.push_back(&l1_bias);
		ad_stack.push_back(&input);
		ad_stack.push_back(&l2_weights);
		ad_stack.push_back(&l2_bias);
		
		ADAdd *l1 = new ADAdd(input, l1_bias);
		ad_stack.push_back(l1);
		
		ADReLu *relu1 = new ADReLu(*l1);
		ad_stack.push_back(relu1);
		
		ADMult *l2 = new ADMult(*relu1, l2_weights);
		ad_stack.push_back(l2);
		
		ADSumReduce *output = new ADSumReduce(*l2);
		ad_stack.push_back(output);
		
		ADAdd *final_output = new ADAdd(*output, l2_bias);
		ad_stack.push_back(final_output);
		
		loss = new ADLossFunc(*final_output, abs_coeff);
		ad_stack.push_back(loss);
		
		all_data.push_back(&PST);
		all_data.push_back(&l1_bias);
		all_data.push_back(&l2_weights);
		all_data.push_back(&l2_bias);
	}
	
	void read(std::string file_name)
	{
		FILE* f = fopen(file_name.c_str(), "r");
		if(!f)
		{
			printf("ERROR: Could not load file\n");
			return;
		}
		for(auto data: all_data)
			for(auto & x : data->values)
			{
				auto r = fscanf(f, "%f", &x);
				if(std::abs(x) < 1e-8f)
					x = 0.0f;
				if(r == EOF)
				{
					printf("ERROR: EOF\n");
					break;
				}
			}
			
		fclose(f);
		printf("Successfully read data\n");
	}
		
	void process_input(const binpack::TrainingDataEntry& entry)
	{
		const Piece * pieces = entry.pos.piecesRaw();
		input.indices.clear();
		if(entry.pos.sideToMove() == Color::White)
		{
			for(int j = 0; j < 64; ++j)
				if(pieces[j] != Piece::none())
				{
					input.indices.push_back(static_cast<int>(pieces[j]) * 64 + j);
				}
		}
		else
		{
			for(int j = 0; j < 64; ++j)
				if(pieces[j] != Piece::none())
				{
					input.indices.push_back((static_cast<int>(pieces[j])^1) * 64 + (j ^ 56));
				}
		}
	}
	
	void update_gradient(const binpack::TrainingDataEntry& entry)
	{
		process_input(entry);
		loss->target = entry.score;
		ad_calc_derivs(ad_stack);
	}
	
	void step(float step_size)
	{
		float grad_length = 0.0f;
		
		for(auto data : all_data)
		{
			for(int i =  0; i < data->size(); ++i)
			{
				grad_length += data->derivative[i] * data->derivative[i];
			}
		}

		float mult = step_size / sqrt(grad_length);
		if(std::isinf(mult) || std::isnan(mult))
			throw std::runtime_error("Bad mult " + std::to_string(mult) + " " + std::to_string(step_size) + " " + std::to_string(grad_length));
		
		for(auto data : all_data)
		{
			for(int i = 0; i < data->size(); ++i)
			{
				if(data -> values[i] != 0.0f)
					data->values[i] -= data->derivative[i] * mult;
			}
		}
	}
	
	void decay_gradient()
	{
		for(auto data : all_data)
			for(auto & x : data->derivative)
				x = 0.0f;
	}
	
	void test()
	{
		float l = 0.0f;
		float n = 0.0f;
		
		for(auto & entry : validation_data)
		{
			process_input(entry);
			loss->target = entry.score;
			ad_calc_values(ad_stack);
			l += loss->values[0];
			n += 1.0f;
		}
		
		int zeros = 0, nonzeros = 0;
		for(int i = 0; i < PST.size(); ++i)
		{
			if(std::abs(PST.values[i]) < 1e-8f)
				zeros++;
			else
				nonzeros++;
		}
		printf("Loss: %.6f Sparse: %d/%d\n", l / n, zeros, nonzeros);
	}
	
	void write(std::string & file_name)
	{
		FILE* f = fopen(file_name.c_str(), "w");
		if(!f)
		{
			printf("ERROR: Could not save to log file\n");
			return;
		}
		for(auto data: all_data)
			for(auto x : data->values)
				fprintf(f, "%.10f\n", x);
			
		fclose(f);
	}
};

struct Evaluator_NNUE2
{
	ADConst PST;
	ADConst l1_bias;
	ADConst l2_weights;
	ADConst l2_bias;
	ADConst l3_weights;
	ADConst l3_bias;
	ADSparseProduct input;
	ADLossFunc* loss;
	std::vector<AD*> ad_stack;
	std::vector<ADConst*> all_data;
	int layer_mask = 0;
	constexpr static float abs_coeff = .0001f;
	
	Evaluator_NNUE2(int hidden_size1, int hidden_size2) :
		PST(768 * hidden_size1),
		input(PST, hidden_size1),
		l1_bias(hidden_size1),
		l2_weights(hidden_size1 * hidden_size2),
		l2_bias(hidden_size2),
		l3_weights(hidden_size2),
		l3_bias(1)
	{
		ad_stack.push_back(&PST);
		ad_stack.push_back(&input);
		ad_stack.push_back(&l1_bias);
		ad_stack.push_back(&l2_weights);
		ad_stack.push_back(&l2_bias);
		ad_stack.push_back(&l3_weights);
		ad_stack.push_back(&l3_bias);
		
		ADAdd *l1 = new ADAdd(input, l1_bias);
		ad_stack.push_back(l1);
		
		ADReLu *relu1 = new ADReLu(*l1);
		ad_stack.push_back(relu1);
		
		ADDenseProduct *dp = new ADDenseProduct(*relu1, l2_weights, hidden_size2);
		ad_stack.push_back(dp);
		
		ADAdd *l2 = new ADAdd(*dp, l2_bias);
		ad_stack.push_back(l2);
		
		ADReLu *relu2 = new ADReLu(*l2);
		ad_stack.push_back(relu2);
	
		ADConst *soft = new ADConst(hidden_size2);
		ad_stack.push_back(soft);
		for(auto & x : soft->values)
			x = 0.1f;
		
		ADMult *res = new ADMult(*l2, *soft);
		ad_stack.push_back(soft);
		
		ADAdd *softrelu2 = new ADAdd(*res, *relu2);
		ad_stack.push_back(softrelu2);
		
		ADMult *l3 = new ADMult(*softrelu2, l3_weights);
		ad_stack.push_back(l3);
		
		ADSumReduce *output = new ADSumReduce(*l3);
		ad_stack.push_back(output);
		
		ADAdd *final_output = new ADAdd(*output, l3_bias);
		ad_stack.push_back(final_output);
		
		loss = new ADLossFunc(*final_output, abs_coeff);
		ad_stack.push_back(loss);
		
		all_data.push_back(&PST);
		all_data.push_back(&l1_bias);
		all_data.push_back(&l2_weights);
		all_data.push_back(&l2_bias);
		all_data.push_back(&l3_weights);
		all_data.push_back(&l3_bias);
		
		int params = 0;
		int i = 0;
		for(auto data : all_data)
		{
			printf("Layer with %d params\n", data->size());
			
			float scale = 0.0f;
			switch(i)
			{
				case 0: scale = 1.0f; break;
				case 1: scale = 1.0f; break;
				case 2: scale = sqrt(2.0f / hidden_size1); break;
				case 3: scale = 1.0f; break;
				case 4: scale = 100.0f * sqrt(2.0f / hidden_size2); break;
				case 5: scale = 1.0f; break;
			}
			
			for(auto & x : data->values)
			{
				params++;
				x = (random64() & 65535) / 65536.0f - 0.5f;
				x *= scale * 2;
				// if(i == 2)
					// x += 4.0f;
			}
			++i;
		}
		
		if(hidden_size2 >= 2)
		{
			l3_weights.values[0] = 1.0f;
			l3_weights.values[1] = -1.0f;
		}
		
		printf("Network has %d parameters\n", params);
	}
	
	void process_input(const binpack::TrainingDataEntry& entry)
	{
		const Piece * pieces = entry.pos.piecesRaw();
		input.indices.clear();
		if(entry.pos.sideToMove() == Color::White)
		{
			for(int j = 0; j < 64; ++j)
				if(pieces[j] != Piece::none())
				{
					input.indices.push_back(static_cast<int>(pieces[j]) * 64 + j);
				}
		}
		else
		{
			for(int j = 0; j < 64; ++j)
				if(pieces[j] != Piece::none())
				{
					input.indices.push_back((static_cast<int>(pieces[j])^1) * 64 + (j ^ 56));
				}
		}
	}
	
	void update_gradient(const binpack::TrainingDataEntry& entry)
	{
		process_input(entry);
		loss->target = entry.score;
		ad_calc_derivs(ad_stack);
	}
	
	void step(float step_size)
	{
		layer_mask ^= 1;
		
		float grad_length = 0.0f;
		
		int idx = -1;
		for(auto data : all_data)
		{
			// ++idx;
			// if(layer_mask == 0 && idx == 2)
				// continue;
			// if(layer_mask == 1 && idx == 4)
				// continue;
			
			for(int i =  0; i < data->size(); ++i)
			{
				grad_length += data->derivative[i] * data->derivative[i];
			}
		}

		float mult = step_size / sqrt(grad_length) * .25f;
		if(std::isinf(mult) || std::isnan(mult))
			throw std::runtime_error("Bad mult " + std::to_string(mult) + " " + std::to_string(step_size) + " " + std::to_string(grad_length));
		
		idx = -1;
		for(auto data : all_data)
		{
			// ++idx;
			// if(layer_mask == 0 && idx == 2)
				// continue;
			// if(layer_mask == 1 && idx == 4)
				// continue;
			for(int i = 0; i < data->size(); ++i)
			{
				data->values[i] -= data->derivative[i] * mult;
			}
		}
	}
	
	void decay_gradient()
	{
		for(auto data : all_data)
			for(auto & x : data->derivative)
				x = 0.0f;
	}
	
	void test()
	{
		float l = 0.0f;
		float n = 0.0f;
		
		for(auto & entry : validation_data)
		{
			process_input(entry);
			loss->target = entry.score;
			ad_calc_values(ad_stack);
			l += loss->values[0];
			n += 1.0f;
		}
		
		printf("Loss: %.6f\n", l / n);
	}
	
	void write(std::string & file_name)
	{
		FILE* f = fopen(file_name.c_str(), "w");
		if(!f)
		{
			printf("ERROR: Could not save to log file\n");
			return;
		}
		for(auto data: all_data)
			for(auto x : data->values)
				fprintf(f, "%.10f\n", x);
			
		fclose(f);
	}
};

struct Evaluator_NNUE3
{
	ADConst PST;
	ADConst l1_bias;
	ADConst l2_weights;
	ADConst l2_bias;
	ADConst l3_weights;
	ADConst l3_bias;
	ADConst l4_weights;
	ADConst l4_bias;
	ADSparseProduct input;
	ADLossFunc* loss;
	ADAdd* eval;
	ADReLu* relu1;
	ADReLu* relu2;
	ADAdd* l2;
	ADAdd* softrelu2;
	ADAdd* softrelu3;
	ADDenseProduct *dp;
	std::vector<AD*> ad_stack;
	std::vector<ADConst*> all_data;
	int layer_mask = 0;
	constexpr static float abs_coeff = 0;
	
	Evaluator_NNUE3(int hidden_size1, int hidden_size2, int hidden_size3) :
		PST(768 * hidden_size1),
		input(PST, hidden_size1),
		l1_bias(hidden_size1),
		l2_weights(hidden_size1 * hidden_size2),
		l2_bias(hidden_size2),
		l3_weights(hidden_size2 * hidden_size3),
		l3_bias(hidden_size3),
		l4_weights(hidden_size3),
		l4_bias(1)
	{
		ad_stack.push_back(&PST);
		ad_stack.push_back(&input);
		ad_stack.push_back(&l1_bias);
		ad_stack.push_back(&l2_weights);
		ad_stack.push_back(&l2_bias);
		ad_stack.push_back(&l3_weights);
		ad_stack.push_back(&l3_bias);
		ad_stack.push_back(&l4_weights);
		ad_stack.push_back(&l4_bias);
		
		ADAdd *l1 = new ADAdd(input, l1_bias);
		ad_stack.push_back(l1);
		
		relu1 = new ADReLu(*l1);
		ad_stack.push_back(relu1);
		
		dp = new ADDenseProduct(*relu1, l2_weights, hidden_size2);
		ad_stack.push_back(dp);
		
		l2 = new ADAdd(*dp, l2_bias);
		ad_stack.push_back(l2);
		
		relu2 = new ADReLu(*l2);
		ad_stack.push_back(relu2);
	
		ADConst *soft = new ADConst(hidden_size2);
		ad_stack.push_back(soft);
		for(auto & x : soft->values)
			x = 0.0625f;
		
		ADMult *res = new ADMult(*l2, *soft);
		ad_stack.push_back(res);
		
		softrelu2 = new ADAdd(*res, *relu2);
		ad_stack.push_back(softrelu2);
		
		ADDenseProduct *dp2 = new ADDenseProduct(*softrelu2, l3_weights, hidden_size3);
		ad_stack.push_back(dp2);
		
		ADAdd *l3 = new ADAdd(*dp2, l3_bias);
		ad_stack.push_back(l3);
		
		ADReLu *relu3 = new ADReLu(*l3);
		ad_stack.push_back(relu3);
		
		ADConst *soft2 = new ADConst(hidden_size3);
		ad_stack.push_back(soft2);
		for(auto & x : soft2->values)
			x = 0.0625f;
		
		ADMult *res2 = new ADMult(*l3, *soft2);
		ad_stack.push_back(res2);
		
		softrelu3 = new ADAdd(*res2, *relu3);
		ad_stack.push_back(softrelu3);
		
		ADMult *l4 = new ADMult(*softrelu3, l4_weights);
		ad_stack.push_back(l4);
		
		ADSumReduce *output = new ADSumReduce(*l4);
		ad_stack.push_back(output);
		
		eval = new ADAdd(*output, l4_bias);
		ad_stack.push_back(eval);
		
		loss = new ADLossFunc(*eval, abs_coeff);
		ad_stack.push_back(loss);
		
		all_data.push_back(&PST);
		all_data.push_back(&l1_bias);
		all_data.push_back(&l2_weights);
		all_data.push_back(&l2_bias);
		all_data.push_back(&l3_weights);
		all_data.push_back(&l3_bias);
		all_data.push_back(&l4_weights);
		all_data.push_back(&l4_bias);
		
		int params = 0;
		int i = 0;
		for(auto data : all_data)
		{
			printf("Layer with %d params\n", data->size());
			
			float scale = 0.0f;
			switch(i)
			{
				case 0: scale = .2f; break;
				case 1: scale = 1.0f; break;
				case 2: scale = sqrt(2.0f / hidden_size1); break;
				case 3: scale = 1.0f; break;
				case 4: scale = sqrt(2.0f / hidden_size2); break;
				case 5: scale = 1.0f; break;
				case 6: scale = 100.0f * sqrt(2.0f / hidden_size3); break;
				case 7: scale = 1.0f; break;
			}
			
			for(auto & x : data->values)
			{
				params++;
				x = (random64() & 65535) / 65536.0f - 0.5f;
				x *= scale * 2;
				// if(i == 2)
					// x += 4.0f;
			}
			++i;
		}
		
		if(hidden_size3 >= 2)
		{
			l4_weights.values[0] = 100.0f * sqrt(2.0f / hidden_size3);
			l4_weights.values[1] = -100.0f * sqrt(2.0f / hidden_size3);
		}
		
		printf("Network has %d parameters\n", params);
	}
	
	void read(std::string file_name)
	{
		FILE* f = fopen(file_name.c_str(), "r");
		if(!f)
		{
			printf("ERROR: Could not load file\n");
			return;
		}
		for(auto data: all_data)
			for(auto & x : data->values)
			{
				auto r = fscanf(f, "%f", &x);
				if(std::abs(x) < 1e-8f)
					x = 0.0f;
				if(r == EOF)
				{
					printf("ERROR: EOF\n");
					break;
				}
			}
			
		fclose(f);
		printf("Successfully read data\n");
	}
	
	void quantize(float p)
	{
		for(auto data: all_data)
			for(auto & x : data->values)
			{
				float q = p;
				if(std::abs(x) > 1.0f)
					q *= .5f;
				if(std::abs(x) > 2.0f)
					q *= .5f;
				if(std::abs(x) > 4.0f)
					q *= .5f;
				if(std::abs(x) > 8.0f)
					q *= .5f;
				if(std::abs(x) > 16.0f)
					q *= .5f;
				if(std::abs(x) > 32.0f)
					q *= .5f;
				x = round(x * q) / q;
			}
	}
		
	void process_input(const binpack::TrainingDataEntry& entry)
	{
		const Piece * pieces = entry.pos.piecesRaw();
		input.indices.clear();
		if(entry.pos.sideToMove() == Color::White)
		{
			for(int j = 0; j < 64; ++j)
				if(pieces[j] != Piece::none())
				{
					input.indices.push_back(static_cast<int>(pieces[j]) * 64 + j);
				}
		}
		else
		{
			for(int j = 0; j < 64; ++j)
				if(pieces[j] != Piece::none())
				{
					input.indices.push_back((static_cast<int>(pieces[j])^1) * 64 + (j ^ 56));
				}
		}
	}
	
	void update_gradient(const binpack::TrainingDataEntry& entry)
	{
		process_input(entry);
		loss->target = entry.score;
		ad_calc_derivs(ad_stack);
		
		if(PRINT_EVAL)
		{
			for(auto x : relu1->values)
				printf("%.1f ", x * 256.0f);
			printf("\nDP ");
			for(auto x : dp->values)
				printf("%.1f ", x * 256.0f);
			printf("\nl2 ");
			for(auto x : l2->values)
				printf("%.1f ", x * 256.0f);
			printf("\nrelu2 ");
			for(auto x : relu2->values)
				printf("%.1f ", x * 256.0f);
			printf("\nSRelu2 ");
			for(auto x : softrelu2->values)
				printf("%.1f ", x * 256.0f);
			printf("\nSRelu3");
			for(auto x : softrelu3->values)
				printf("%.1f ", x * 256.0f);
			printf("\neval = %.2f\n", eval->values[0]);
		}
	}
	
	void step(float step_size)
	{
		layer_mask ^= 1;
		
		float grad_length = 0.0f;
		
		int idx = -1;
		for(auto data : all_data)
		{
			++idx;
			// if(layer_mask == 0 && idx == 2)
				// continue;
			// if(layer_mask == 1 && idx == 4)
				// continue;
			
			for(int i =  0; i < data->size(); ++i)
			{
				grad_length += data->derivative[i] * data->derivative[i];
			}
		}

		float mult = step_size / sqrt(grad_length) * .25f;
		if(std::isinf(mult) || std::isnan(mult))
			throw std::runtime_error("Bad mult " + std::to_string(mult) + " " + std::to_string(step_size) + " " + std::to_string(grad_length));
		
		idx = -1;
		for(auto data : all_data)
		{
			++idx;
			// if(layer_mask == 0 && idx == 2)
				// continue;
			// if(layer_mask == 1 && idx == 4)
				// continue;
			for(int i = 0; i < data->size(); ++i)
			{
				data->values[i] -= data->derivative[i] * mult;
			}
		}
	}
	
	void decay_gradient()
	{
		for(auto data : all_data)
			for(auto & x : data->derivative)
				x = 0.0f;
	}
	
	void test()
	{
		float l = 0.0f;
		float n = 0.0f;
		
		for(auto & entry : validation_data)
		{
			process_input(entry);
			loss->target = entry.score;
			ad_calc_values(ad_stack);
			l += loss->values[0];
			n += 1.0f;
		}
		
		printf("Loss: %.6f\n", l / n);
	}
	
	void write(std::string & file_name)
	{
		FILE* f = fopen(file_name.c_str(), "w");
		if(!f)
		{
			printf("ERROR: Could not save to log file\n");
			return;
		}
		for(auto data: all_data)
			for(auto x : data->values)
				fprintf(f, "%.10f\n", x);
			
		fclose(f);
	}
};


struct Evaluator_NNUE3_SRELU
{
	ADConst PST;
	ADConst l1_bias;
	ADConst l2_weights;
	ADConst l2_bias;
	ADConst l3_weights;
	ADConst l3_bias;
	ADConst l4_weights;
	ADConst l4_bias;
	ADSparseProduct input;
	ADLossFunc* loss;
	std::vector<AD*> ad_stack;
	std::vector<ADConst*> all_data;
	int layer_mask = 0;
	constexpr static float abs_coeff = .00001f;
	
	Evaluator_NNUE3_SRELU(int hidden_size1, int hidden_size2, int hidden_size3) :
		PST(768 * hidden_size1),
		input(PST, hidden_size1),
		l1_bias(hidden_size1),
		l2_weights(hidden_size1 * hidden_size2),
		l2_bias(hidden_size2),
		l3_weights(hidden_size2 * hidden_size3),
		l3_bias(hidden_size3),
		l4_weights(hidden_size3),
		l4_bias(1)
	{
		ad_stack.push_back(&PST);
		ad_stack.push_back(&input);
		ad_stack.push_back(&l1_bias);
		ad_stack.push_back(&l2_weights);
		ad_stack.push_back(&l2_bias);
		ad_stack.push_back(&l3_weights);
		ad_stack.push_back(&l3_bias);
		ad_stack.push_back(&l4_weights);
		ad_stack.push_back(&l4_bias);
		
		ADAdd *l1 = new ADAdd(input, l1_bias);
		ad_stack.push_back(l1);
		
		ADSReLu *relu1 = new ADSReLu(*l1);
		ad_stack.push_back(relu1);
		
		ADDenseProduct *dp = new ADDenseProduct(*relu1, l2_weights, hidden_size2);
		ad_stack.push_back(dp);
		
		ADAdd *l2 = new ADAdd(*dp, l2_bias);
		ad_stack.push_back(l2);
		
		ADSReLu *relu2 = new ADSReLu(*l2);
		ad_stack.push_back(relu2);
	
		ADConst *soft = new ADConst(hidden_size2);
		ad_stack.push_back(soft);
		for(auto & x : soft->values)
			x = 0.0625f;
		
		ADMult *res = new ADMult(*l2, *soft);
		ad_stack.push_back(soft);
		
		ADAdd *softrelu2 = new ADAdd(*res, *relu2);
		ad_stack.push_back(softrelu2);
		
		ADDenseProduct *dp2 = new ADDenseProduct(*softrelu2, l3_weights, hidden_size3);
		ad_stack.push_back(dp2);
		
		ADAdd *l3 = new ADAdd(*dp2, l3_bias);
		ad_stack.push_back(l3);
		
		ADSReLu *relu3 = new ADSReLu(*l3);
		ad_stack.push_back(relu3);
		
		ADConst *soft2 = new ADConst(hidden_size3);
		ad_stack.push_back(soft2);
		for(auto & x : soft2->values)
			x = 0.0625f;
		
		ADMult *res2 = new ADMult(*l3, *soft2);
		ad_stack.push_back(res2);
		
		ADAdd *softrelu3 = new ADAdd(*res2, *relu3);
		ad_stack.push_back(softrelu3);
		
		ADMult *l4 = new ADMult(*softrelu3, l4_weights);
		ad_stack.push_back(l4);
		
		ADSumReduce *output = new ADSumReduce(*l4);
		ad_stack.push_back(output);
		
		ADAdd *final_output = new ADAdd(*output, l4_bias);
		ad_stack.push_back(final_output);
		
		loss = new ADLossFunc(*final_output, abs_coeff);
		ad_stack.push_back(loss);
		
		all_data.push_back(&PST);
		all_data.push_back(&l1_bias);
		all_data.push_back(&l2_weights);
		all_data.push_back(&l2_bias);
		all_data.push_back(&l3_weights);
		all_data.push_back(&l3_bias);
		all_data.push_back(&l4_weights);
		all_data.push_back(&l4_bias);
		
		int params = 0;
		int i = 0;
		for(auto data : all_data)
		{
			printf("Layer with %d params\n", data->size());
			
			float scale = 0.0f;
			switch(i)
			{
				case 0: scale = .2f; break;
				case 1: scale = 1.0f; break;
				case 2: scale = sqrt(2.0f / hidden_size1); break;
				case 3: scale = 1.0f; break;
				case 4: scale = sqrt(2.0f / hidden_size2); break;
				case 5: scale = 1.0f; break;
				case 6: scale = 100.0f * sqrt(2.0f / hidden_size3); break;
				case 7: scale = 1.0f; break;
			}
			
			for(auto & x : data->values)
			{
				params++;
				x = (random64() & 65535) / 65536.0f - 0.5f;
				x *= scale * 2;
				// if(i == 2)
					// x += 4.0f;
			}
			++i;
		}
		
		if(hidden_size3 >= 2)
		{
			l4_weights.values[0] = 100.0f * sqrt(2.0f / hidden_size3);
			l4_weights.values[1] = -100.0f * sqrt(2.0f / hidden_size3);
		}
		
		printf("Network has %d parameters\n", params);
	}
	
	void read(std::string file_name)
	{
		FILE* f = fopen(file_name.c_str(), "r");
		if(!f)
		{
			printf("ERROR: Could not load file\n");
			return;
		}
		for(auto data: all_data)
			for(auto & x : data->values)
			{
				auto r = fscanf(f, "%f", &x);
				if(std::abs(x) < 1e-8f)
					x = 0.0f;
				if(r == EOF)
				{
					printf("ERROR: EOF\n");
					break;
				}
			}
			
		fclose(f);
		printf("Successfully read data\n");
	}
	
	void quantize(float p)
	{
		for(auto data: all_data)
			for(auto & x : data->values)
			{
				float q = p;
				if(std::abs(x) > 1.0f)
					q *= .5f;
				if(std::abs(x) > 2.0f)
					q *= .5f;
				if(std::abs(x) > 4.0f)
					q *= .5f;
				if(std::abs(x) > 8.0f)
					q *= .5f;
				if(std::abs(x) > 16.0f)
					q *= .5f;
				if(std::abs(x) > 32.0f)
					q *= .5f;
				x = round(x * q) / q;
			}
	}
		
	void process_input(const binpack::TrainingDataEntry& entry)
	{
		const Piece * pieces = entry.pos.piecesRaw();
		input.indices.clear();
		if(entry.pos.sideToMove() == Color::White)
		{
			for(int j = 0; j < 64; ++j)
				if(pieces[j] != Piece::none())
				{
					input.indices.push_back(static_cast<int>(pieces[j]) * 64 + j);
				}
		}
		else
		{
			for(int j = 0; j < 64; ++j)
				if(pieces[j] != Piece::none())
				{
					input.indices.push_back((static_cast<int>(pieces[j])^1) * 64 + (j ^ 56));
				}
		}
	}
	
	void update_gradient(const binpack::TrainingDataEntry& entry)
	{
		process_input(entry);
		loss->target = entry.score;
		ad_calc_derivs(ad_stack);
	}
	
	void step(float step_size)
	{
		layer_mask ^= 1;
		
		float grad_length = 0.0f;
		
		int idx = -1;
		for(auto data : all_data)
		{
			++idx;
			// if(layer_mask == 0 && idx == 2)
				// continue;
			// if(layer_mask == 1 && idx == 4)
				// continue;
			
			for(int i =  0; i < data->size(); ++i)
			{
				grad_length += data->derivative[i] * data->derivative[i];
			}
		}

		float mult = step_size / sqrt(grad_length) * .25f;
		if(std::isinf(mult) || std::isnan(mult))
			throw std::runtime_error("Bad mult " + std::to_string(mult) + " " + std::to_string(step_size) + " " + std::to_string(grad_length));
		
		idx = -1;
		for(auto data : all_data)
		{
			++idx;
			// if(layer_mask == 0 && idx == 2)
				// continue;
			// if(layer_mask == 1 && idx == 4)
				// continue;
			for(int i = 0; i < data->size(); ++i)
			{
				data->values[i] -= data->derivative[i] * mult;
			}
		}
	}
	
	void decay_gradient()
	{
		for(auto data : all_data)
			for(auto & x : data->derivative)
				x = 0.0f;
	}
	
	void test()
	{
		float l = 0.0f;
		float n = 0.0f;
		
		for(auto & entry : validation_data)
		{
			process_input(entry);
			loss->target = entry.score;
			ad_calc_values(ad_stack);
			l += loss->values[0];
			n += 1.0f;
		}
		
		printf("Loss: %.6f\n", l / n);
	}
	
	void write(std::string & file_name)
	{
		FILE* f = fopen(file_name.c_str(), "w");
		if(!f)
		{
			printf("ERROR: Could not save to log file\n");
			return;
		}
		for(auto data: all_data)
			for(auto x : data->values)
				fprintf(f, "%.10f\n", x);
			
		fclose(f);
	}
};

struct Evaluator_NNUE2_SRELU
{
	ADConst PST;
	ADConst l1_bias;
	ADConst l2_weights;
	ADConst l2_bias;
	ADConst l3_weights;
	ADConst l3_bias;
	ADSparseProduct input;
	ADLossFunc* loss;
	std::vector<AD*> ad_stack;
	std::vector<ADConst*> all_data;
	int layer_mask = 0;
	constexpr static float abs_coeff = .0001f;
	
	Evaluator_NNUE2_SRELU(int hidden_size1, int hidden_size2) :
		PST(768 * hidden_size1),
		input(PST, hidden_size1),
		l1_bias(hidden_size1),
		l2_weights(hidden_size1 * hidden_size2),
		l2_bias(hidden_size2),
		l3_weights(hidden_size2),
		l3_bias(1)
	{
		ad_stack.push_back(&PST);
		ad_stack.push_back(&input);
		ad_stack.push_back(&l1_bias);
		ad_stack.push_back(&l2_weights);
		ad_stack.push_back(&l2_bias);
		ad_stack.push_back(&l3_weights);
		ad_stack.push_back(&l3_bias);
		
		ADAdd *l1 = new ADAdd(input, l1_bias);
		ad_stack.push_back(l1);
		
		ADSReLu *relu1 = new ADSReLu(*l1);
		ad_stack.push_back(relu1);
		
		ADDenseProduct *dp = new ADDenseProduct(*relu1, l2_weights, hidden_size2);
		ad_stack.push_back(dp);
		
		ADAdd *l2 = new ADAdd(*dp, l2_bias);
		ad_stack.push_back(l2);
		
		ADSReLu *relu2 = new ADSReLu(*l2);
		ad_stack.push_back(relu2);
	
		ADConst *soft = new ADConst(hidden_size2);
		ad_stack.push_back(soft);
		for(auto & x : soft->values)
			x = 0.1f;
		
		ADMult *res = new ADMult(*l2, *soft);
		ad_stack.push_back(soft);
		
		ADAdd *softrelu2 = new ADAdd(*res, *relu2);
		ad_stack.push_back(softrelu2);
		
		ADMult *l3 = new ADMult(*softrelu2, l3_weights);
		ad_stack.push_back(l3);
		
		ADSumReduce *output = new ADSumReduce(*l3);
		ad_stack.push_back(output);
		
		ADAdd *final_output = new ADAdd(*output, l3_bias);
		ad_stack.push_back(final_output);
		
		loss = new ADLossFunc(*final_output, abs_coeff);
		ad_stack.push_back(loss);
		
		all_data.push_back(&PST);
		all_data.push_back(&l1_bias);
		all_data.push_back(&l2_weights);
		all_data.push_back(&l2_bias);
		all_data.push_back(&l3_weights);
		all_data.push_back(&l3_bias);
		
		int params = 0;
		int i = 0;
		for(auto data : all_data)
		{
			printf("Layer with %d params\n", data->size());
			
			float scale = 0.0f;
			switch(i)
			{
				case 0: scale = 1.0f; break;
				case 1: scale = 1.0f; break;
				case 2: scale = sqrt(2.0f / hidden_size1); break;
				case 3: scale = 1.0f; break;
				case 4: scale = 100.0f * sqrt(2.0f / hidden_size2); break;
				case 5: scale = 1.0f; break;
			}
			
			for(auto & x : data->values)
			{
				params++;
				x = (random64() & 65535) / 65536.0f - 0.5f;
				x *= scale * 2;
				// if(i == 2)
					// x += 4.0f;
			}
			++i;
		}
		
		if(hidden_size2 >= 2)
		{
			l3_weights.values[0] = 100.0f * sqrt(2.0f / hidden_size2);
			l3_weights.values[1] = -100.0f * sqrt(2.0f / hidden_size2);
		}
		
		printf("Network has %d parameters\n", params);
	}
	
	void process_input(const binpack::TrainingDataEntry& entry)
	{
		const Piece * pieces = entry.pos.piecesRaw();
		input.indices.clear();
		if(entry.pos.sideToMove() == Color::White)
		{
			for(int j = 0; j < 64; ++j)
				if(pieces[j] != Piece::none())
				{
					input.indices.push_back(static_cast<int>(pieces[j]) * 64 + j);
				}
		}
		else
		{
			for(int j = 0; j < 64; ++j)
				if(pieces[j] != Piece::none())
				{
					input.indices.push_back((static_cast<int>(pieces[j])^1) * 64 + (j ^ 56));
				}
		}
	}
	
	void update_gradient(const binpack::TrainingDataEntry& entry)
	{
		process_input(entry);
		loss->target = entry.score;
		ad_calc_derivs(ad_stack);
	}
	
	void step(float step_size)
	{
		layer_mask ^= 1;
		
		float grad_length = 0.0f;
		
		int idx = -1;
		for(auto data : all_data)
		{
			// ++idx;
			// if(layer_mask == 0 && idx == 2)
				// continue;
			// if(layer_mask == 1 && idx == 4)
				// continue;
			
			for(int i =  0; i < data->size(); ++i)
			{
				grad_length += data->derivative[i] * data->derivative[i];
			}
		}

		float mult = step_size / sqrt(grad_length) * .25f;
		if(std::isinf(mult) || std::isnan(mult))
			throw std::runtime_error("Bad mult " + std::to_string(mult) + " " + std::to_string(step_size) + " " + std::to_string(grad_length));
		
		idx = -1;
		for(auto data : all_data)
		{
			// ++idx;
			// if(layer_mask == 0 && idx == 2)
				// continue;
			// if(layer_mask == 1 && idx == 4)
				// continue;
			for(int i = 0; i < data->size(); ++i)
			{
				data->values[i] -= data->derivative[i] * mult;
			}
		}
	}
	
	void decay_gradient()
	{
		for(auto data : all_data)
			for(auto & x : data->derivative)
				x = 0.0f;
	}
	
	void test()
	{
		float l = 0.0f;
		float n = 0.0f;
		
		for(auto & entry : validation_data)
		{
			process_input(entry);
			loss->target = entry.score;
			ad_calc_values(ad_stack);
			l += loss->values[0];
			n += 1.0f;
		}
		
		printf("Loss: %.6f\n", l / n);
	}
	
	void write(std::string & file_name)
	{
		FILE* f = fopen(file_name.c_str(), "w");
		if(!f)
		{
			printf("ERROR: Could not save to log file\n");
			return;
		}
		for(auto data: all_data)
			for(auto x : data->values)
				fprintf(f, "%.10f\n", x);
			
		fclose(f);
	}
};

struct Evaluator_NNUE3_Multi
{
	ADConst PST;
	ADConst l1_bias;
	ADConst l2_weights[4];
	ADConst l2_bias[4];
	ADConst l3_weights[4];
	ADConst l3_bias[4];
	ADConst l4_weights[4];
	ADConst l4_bias[4];
	ADSparseProduct input;
	ADLossFunc* loss[4];
	//ADChooser* chosen_eval;
	AD * eval[4];
	std::vector<AD*> ad_stack[4];
	std::vector<ADConst*> all_data;
	int layer_mask = 0;
	constexpr static float abs_coeff = 0;
	
	Evaluator_NNUE3_Multi(int hidden_size1, int hidden_size2, int hidden_size3) :
		PST(768 * hidden_size1),
		input(PST, hidden_size1),
		l1_bias(hidden_size1)		
	{
		all_data.push_back(&PST);
		all_data.push_back(&l1_bias);
		
		for(int i = 0; i < 4; i++)
		{
			ad_stack[i].push_back(&PST);
			ad_stack[i].push_back(&input);
			ad_stack[i].push_back(&l1_bias);
			
			l2_weights[i] = ADConst(hidden_size1 * hidden_size2);
			l2_bias[i] = ADConst(hidden_size2);
			l3_weights[i] = ADConst(hidden_size2 * hidden_size3);
			l3_bias[i] = ADConst(hidden_size3);
			l4_weights[i] = ADConst(hidden_size3);
			l4_bias[i] = ADConst(1);
			
			ad_stack[i].push_back(&l2_weights[i]);
			ad_stack[i].push_back(&l2_bias[i]);
			ad_stack[i].push_back(&l3_weights[i]);
			ad_stack[i].push_back(&l3_bias[i]);
			ad_stack[i].push_back(&l4_weights[i]);
			ad_stack[i].push_back(&l4_bias[i]);
			
			all_data.push_back(&l2_weights[i]);
			all_data.push_back(&l2_bias[i]);
			all_data.push_back(&l3_weights[i]);
			all_data.push_back(&l3_bias[i]);
			all_data.push_back(&l4_weights[i]);
			all_data.push_back(&l4_bias[i]);
			
			ADAdd *l1 = new ADAdd(input, l1_bias);
			ad_stack[i].push_back(l1);
		
			ADReLu * relu1 = new ADReLu(*l1);
			ad_stack[i].push_back(relu1);
			
			ADDenseProduct * dp = new ADDenseProduct(*relu1, l2_weights[i], hidden_size2);
			ad_stack[i].push_back(dp);
			
			ADAdd * l2 = new ADAdd(*dp, l2_bias[i]);
			ad_stack[i].push_back(l2);
			
			ADReLu * relu2 = new ADReLu(*l2);
			ad_stack[i].push_back(relu2);
		
			ADConst *soft = new ADConst(hidden_size2);
			ad_stack[i].push_back(soft);
			for(auto & x : soft->values)
				x = 0.0625f;
			
			ADMult *res = new ADMult(*l2, *soft);
			ad_stack[i].push_back(res);
			
			ADAdd *softrelu2 = new ADAdd(*res, *relu2);
			ad_stack[i].push_back(softrelu2);
			
			ADDenseProduct *dp2 = new ADDenseProduct(*softrelu2, l3_weights[i], hidden_size3);
			ad_stack[i].push_back(dp2);
			
			ADAdd *l3 = new ADAdd(*dp2, l3_bias[i]);
			ad_stack[i].push_back(l3);
			
			ADReLu *relu3 = new ADReLu(*l3);
			ad_stack[i].push_back(relu3);
			
			ADConst *soft2 = new ADConst(hidden_size3);
			ad_stack[i].push_back(soft2);
			for(auto & x : soft2->values)
				x = 0.0625f;
			
			ADMult *res2 = new ADMult(*l3, *soft2);
			ad_stack[i].push_back(res2);
			
			ADAdd * softrelu3 = new ADAdd(*res2, *relu3);
			ad_stack[i].push_back(softrelu3);
			
			ADMult *l4 = new ADMult(*softrelu3, l4_weights[i]);
			ad_stack[i].push_back(l4);
			
			ADSumReduce * output = new ADSumReduce(*l4);
			ad_stack[i].push_back(output);
			
			eval[i] = new ADAdd(*output, l4_bias[i]);
			ad_stack[i].push_back(eval[i]);
			
			loss[i] = new ADLossFunc(*eval[i], abs_coeff);
			ad_stack[i].push_back(loss[i]);
		}
		
		// chosen_eval = new ADChooser(eval);
		// ad_stack.push_back(chosen_eval);
		
		// loss = new ADLossFunc(*chosen_eval, abs_coeff);
		// ad_stack.push_back(loss);
		
		int params = 0;
		int i = 0;
		for(auto data : all_data)
		{
			printf("Layer with %d params\n", data->size());
			
			float scale = 0.0f;
			if(i == 0)
				scale = .2f;
			else if(i == 1)
				scale = 1.0f;
			else if(i%6 == 2)
				scale = sqrt(2.0f / hidden_size1);
			else if(i%6 == 3)
				scale = 1.0f;
			else if(i%6 == 4)
				scale = sqrt(2.0f / hidden_size2);
			else if(i%6 == 5)
				scale = 1.0f;
			else if(i%6 == 0)
				scale = 100.0f * sqrt(2.0f / hidden_size3);
			else if(i%6 == 1)
				scale = 1.0f;
			
			for(auto & x : data->values)
			{
				params++;
				x = (random64() & 65535) / 65536.0f - 0.5f;
				x *= scale * 2;
				// if(i == 2)
					// x += 4.0f;
			}
			++i;
		}
		
		printf("Network has %d parameters\n", params);
	}
	
	void read(std::string file_name)
	{
		FILE* f = fopen(file_name.c_str(), "r");
		if(!f)
		{
			printf("ERROR: Could not load file\n");
			return;
		}
		for(auto data: all_data)
			for(auto & x : data->values)
			{
				auto r = fscanf(f, "%f", &x);
				if(std::abs(x) < 1e-8f)
					x = 0.0f;
				if(r == EOF)
				{
					printf("ERROR: EOF\n");
					break;
				}
			}
			
		fclose(f);
		printf("Successfully read data\n");
	}
	
	void quantize(float p)
	{
		for(auto data: all_data)
			for(auto & x : data->values)
			{
				float q = p;
				if(std::abs(x) > 1.0f)
					q *= .5f;
				if(std::abs(x) > 2.0f)
					q *= .5f;
				if(std::abs(x) > 4.0f)
					q *= .5f;
				if(std::abs(x) > 8.0f)
					q *= .5f;
				if(std::abs(x) > 16.0f)
					q *= .5f;
				if(std::abs(x) > 32.0f)
					q *= .5f;
				x = round(x * q) / q;
			}
	}
		
	void process_input(const binpack::TrainingDataEntry& entry)
	{
		const Piece * pieces = entry.pos.piecesRaw();
		input.indices.clear();
		if(entry.pos.sideToMove() == Color::White)
		{
			for(int j = 0; j < 64; ++j)
				if(pieces[j] != Piece::none())
				{
					input.indices.push_back(static_cast<int>(pieces[j]) * 64 + j);
				}
		}
		else
		{
			for(int j = 0; j < 64; ++j)
				if(pieces[j] != Piece::none())
				{
					input.indices.push_back((static_cast<int>(pieces[j])^1) * 64 + (j ^ 56));
				}
		}
	}
	
	int get_phase(const binpack::TrainingDataEntry& entry)
	{
		// static int pst_phase[] = {0,1,1,2,0,0};
		// int phase = 0;
		// bool queen = false;
		// const Piece * pieces = entry.pos.piecesRaw();
		// for(int j = 0; j < 64; ++j)
			// if(pieces[j].type() != PieceType::None)
			// {
				// phase += pst_phase[static_cast<int>(pieces[j].type())];
				// queen |= pieces[j].type() == PieceType::Queen;
			// }
		// phase = queen * 2 + (phase > 8);
		// return phase;
		
		static int pst_phase[] = {0,1,1,2,4,0};
		int phase = 0;
		const Piece * pieces = entry.pos.piecesRaw();
		for(int j = 0; j < 64; ++j)
			if(pieces[j].type() != PieceType::None)
			{
				phase += pst_phase[static_cast<int>(pieces[j].type())];
			}
		phase = (phase > 6) + (phase > 12) + (phase > 18);
		return phase;
	}
	
	void update_gradient(const binpack::TrainingDataEntry& entry)
	{
		process_input(entry);
		int phase = get_phase(entry);
		loss[phase]->target = entry.score;
		ad_calc_derivs(ad_stack[phase]);
	}
	
	void step(float step_size)
	{
		layer_mask ^= 1;
		
		float grad_length = 0.0f;
		
		int idx = -1;
		for(auto data : all_data)
		{
			++idx;
			// if(layer_mask == 0 && idx == 2)
				// continue;
			// if(layer_mask == 1 && idx == 4)
				// continue;
			
			for(int i =  0; i < data->size(); ++i)
			{
				grad_length += data->derivative[i] * data->derivative[i];
			}
		}

		float mult = step_size / sqrt(grad_length) * .25f;
		if(std::isinf(mult) || std::isnan(mult))
			throw std::runtime_error("Bad mult " + std::to_string(mult) + " " + std::to_string(step_size) + " " + std::to_string(grad_length));
		
		idx = -1;
		for(auto data : all_data)
		{
			++idx;
			// if(layer_mask == 0 && idx == 2)
				// continue;
			// if(layer_mask == 1 && idx == 4)
				// continue;
			for(int i = 0; i < data->size(); ++i)
			{
				data->values[i] -= data->derivative[i] * mult;
			}
		}
	}
	
	void decay_gradient()
	{
		for(auto data : all_data)
			for(auto & x : data->derivative)
				x = 0.0f;
	}
	
	void test()
	{
		float l = 0.0f;
		float n = 0.0f;
		
		for(auto & entry : validation_data)
		{
			process_input(entry);
			int phase = get_phase(entry);
			loss[phase]->target = entry.score;
			ad_calc_values(ad_stack[phase]);
			l += loss[phase]->values[0];
			n += 1.0f;
		}
		
		printf("Loss: %.6f\n", l / n);
	}
	
	void write(std::string & file_name)
	{
		FILE* f = fopen(file_name.c_str(), "w");
		if(!f)
		{
			printf("ERROR: Could not save to log file\n");
			return;
		}
		for(auto data: all_data)
			for(auto x : data->values)
				fprintf(f, "%.10f\n", x);
			
		fclose(f);
	}
};

//////////////////////////////////////////////////////////////////////
//main
//////////////////////////////////////////////////////////////////////

std::tuple<double, double, double> win_rate_model(int ply, int score) 
{

   // The model captures only up to 240 plies, so limit input (and rescale)
   double m = std::min(240, int(ply)) / 64.0;

   // Coefficients of a 3rd order polynomial fit based on fishtest data
   // for two parameters needed to transform eval to the argument of a
   // logistic function.
   double as[] = {-3.68389304,  30.07065921, -60.52878723, 149.53378557};
   double bs[] = {-2.0181857,   15.85685038, -29.83452023,  47.59078827};
   double a = (((as[0] * m + as[1]) * m + as[2]) * m) + as[3];
   double b = (((bs[0] * m + bs[1]) * m + bs[2]) * m) + bs[3];

   // tweak wdl model, deviating from fishtest results,
   // but yielding improved training results
   b *= 1.5;

   // Transform eval to centipawns with limited range
   double x = std::clamp(double(100 * score) / 208, -2000.0, 2000.0);
   double w = 1.0 / (1 + std::exp((a - x) / b));
   double l = 1.0 / (1 + std::exp((a + x) / b));
   double d = 1.0 - w - l;

   // Return win, loss, draw rate in per mille (rounded to nearest)
   return std::make_tuple(w, l, d);
}

int main()
{
	std::string weight_file = "weight_NNUE3o5_64_16_8_s0.txt";	
	
	int seed = 0;
	printf("Output file: %s\n", weight_file.c_str());
	printf("Seed = %d\n", seed);
	
	for(int i = 0; i < 100000*seed; ++i)
		random64();
	
	Evaluator_NNUE3_Multi evaluator(64, 16, 8);
	
	//For starting training from existing weight files
	//evaluator.read("weight_NNUE3_80_24_16_s0.txt");
	//evaluator.read("NNUE_quantized.txt");
	//evaluator.quantize(20);
	
	auto reader = binpack::CompressedTrainingDataEntryReader("./training/test80-2024-06-jun-2tb7p.min-v2.v6.binpack");
	
	int64_t count = 0;
	int64_t i = 0;
	
	//Gradient descent params
	int batch_size = 1000;
	int batch_size_inc = 100;
	float step_size = 2.0;
	float step_size_shrink = 0.015f;
	float step_size_min = 0.025f;
		
	validation_data.reserve(400000);
	
	for(; reader.hasNext(); ++i)
	{
		auto entry = reader.next();
		
		//skip in check
		if(entry.pos.isCheck())
			continue;
		
		//skip extreme or missing scores
		if(std::abs(entry.score) > 20000)
			continue;

		//skip when best is a capture
		if(entry.pos.pieceAt(entry.move.to) != Piece::none() || entry.move.type == MoveType::EnPassant)
			continue;
		
		//Let's go!
		count++;
		
		//Gradually create validation set
		if(count % 100 == 1 && validation_data.size() < 400000)
		{
			validation_data.push_back(entry);
			continue;
		}
	
		if(step_size > 0)  //hacks for testing stuff
		{
			if(PRINT_EVAL)
			{
				std::string s;
				binpack::emitPlainEntry(s, entry);
				printf("%s\n", s.c_str());
			}
			//features to gradient / eval
			evaluator.update_gradient(entry);
			
			if(PRINT_EVAL && count > 100)
				return 0;
			
			//step
			if(count % batch_size == 0)
			{
				evaluator.step(step_size);
				evaluator.decay_gradient();
			}
		}
	
		//display progress and run validation once in a while
		if(count % 5'000'000 == 0)
		{
			printf(" ** Count = %ld / Position# = %ld\n", count, i);
			evaluator.test();
			step_size -= (step_size - step_size_min) * step_size_shrink;
			batch_size += batch_size_inc;
			printf("New step size %f, batch size %d\n", step_size, batch_size);
	
			if(count % 25'000'000 == 0)
			{
				evaluator.write(weight_file);
				printf("weights saved\n");
			}
		}
	}
	
	printf("Reached end of data at i = %ld count = %ld\n", i, count);
	evaluator.test();
		
	return 0;
}