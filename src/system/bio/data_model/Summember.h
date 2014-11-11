#pragma once
#include <vector>
#include <algorithm>
#include <cmath>

typedef unsigned int uint;

template <typename T>
class Summember
{
public:
	typedef T value_type;

	struct ramp
	{
		// dim is index to model.var_names but incremented by 1. Proper using is model.getVariable(dim -1)
		std::size_t dim;		//Warning: index of var_name from Model.h but indexing from 1 (not 0)
		value_type min;
		value_type max;
		value_type min_value;
		value_type max_value;
		bool negative;

        ramp() {}
		ramp(std::size_t dim, value_type min, value_type max, value_type minValue, value_type maxValue, bool negative = false)
                    : dim(dim), min(min), max(max), min_value(minValue), max_value(maxValue), negative(negative) {}
        ramp(const ramp& r) : dim(r.dim), min(r.min), max(r.max), min_value(r.min_value), max_value(r.max_value), negative(r.negative) {}


		value_type value(value_type value) const
		{
			value_type res = (value - min) / (max - min);
			if (res < 0)
				res = 0;
			else if (res > 1)
				res = 1;
			return (min_value + (res * (max_value - min_value)));
		}

		friend std::ostream& operator<<(std::ostream& out, const ramp& r) {
		    if(r.negative)
                out << "R(-)("; 
            else
                out << "R(+)(";
            out << r.dim - 1 << "," << r.min << "," << r.max << "," << r.min_value << "," << r.max_value << ")";
            return out;
		}
	};

private:

	struct step {
	    std::size_t dim;		//Warning: index of var_name from Model.h but indexing from 1 (not 0)
	    value_type theta;
	    value_type a;
	    value_type b;
	    bool positive;
	    
        step() {}
		step(std::size_t new_dim, value_type new_theta, value_type new_a, value_type new_b, bool new_positive = true)
                    : dim(new_dim), theta(new_theta), a(new_a), b(new_b), positive(new_positive) {
        	if( (positive && a > b) || (!positive && a < b) ) {
        		a = new_b;
        		b = new_a;
        	}
        }
        step(const step& s) : dim(s.dim), theta(s.theta), a(s.a), b(s.b), positive(s.positive) {}


		value_type value(value_type value) const
		{
			value_type result = value < theta ? a : b; 
			return result;
		}

		friend std::ostream& operator<<(std::ostream& out, const step& s) {
		    if(s.positive)
                out << "H(-)(";
            else
                out << "H(+)(";
            out << s.dim - 1 << "," << s.theta << "," << s.a << "," << s.b << ")";
            return out;
		}
	};

public:
	Summember()
		: constant(1), param(0)
	{}
	~Summember(){};

	// Access methods
	// GET
	value_type GetConstant() const
	{
		return constant;
	}

	std::size_t GetParam() const
	{
		return param;
	}

	std::vector<std::size_t> GetVars() const
	{
		return vars;
	}

	std::vector<ramp> GetRamps() const
	{
		return ramps;
	}

	std::vector<std::size_t> GetSigmoids() const
	{
		return sigmoids;
	}
	
	std::vector<step> GetSteps() const
	{
		return steps;
	}
	
	std::vector<std::size_t> GetHills() const
	{
		return hills;
	}

	// ADD
	void AddConstant(value_type c)
	{
		constant = c;
	}

	void AddParam(std::size_t p)
	{
		param = p;
	}

	void AddVar(std::size_t v)
	{
		vars.push_back(v);
	}

	void AddRamp(std::size_t dim, value_type min, value_type max, value_type min_value, value_type max_value, bool negative = 0)
	{
		ramp r;
		r.dim = dim;
		r.min = min;
		r.max = max;
		r.min_value = min_value;
		r.max_value = max_value;
		r.negative = negative;
		ramps.push_back(r);
	}

	void AddRamp(ramp & new_ramp)
	{
		ramps.push_back(new_ramp);
	}

	void AddSigmoid(std::size_t s)
	{
		sigmoids.push_back(s);
	}

	void AddStep(std::size_t dim, value_type theta, value_type a, value_type b, bool positive = 1) {
	    step s;
	    s.dim = dim;
	    s.theta = theta;
	    s.a = a;
	    s.b = b;
	    s.positive = positive;
	    steps.push_back(s);
	}

	void AddStep(step& new_step) {
	    steps.push_back(new_step);
	}
	
	void AddHill(size_t i) {
		hills.push_back(i);
	}

	std::size_t hasParam() const
	{
		return (param != 0 ? 1 : 0);

	}

	void negate() {
	    constant *= -1;
	}

	std::vector< Summember<T> > sigmoidAbstraction(std::vector<ramp> ramps, std::size_t replacedIndex);
	std::vector< Summember<T> > hillAbstraction(std::vector<ramp> ramps, std::size_t replacedIndex);


	const Summember<T>  operator*(/*const*/ Summember<T> &);

    template <class U>
	friend std::ostream& operator<<(std::ostream& out, const Summember<U>& sum);

private:
	value_type constant;
	//Warning: param contains index to Model.param_names and Model.param_ranges vectors, BUT incremented by 1 
	//(Proper using is Model.getParamName(param-1))
	std::size_t param;
	//Warning: vars contains indexes to Model.var_names vector, BUT incremented by 1 (Proper using is Model.getVariable(vars.at(i)-1))
	std::vector<std::size_t> vars;
	std::vector<ramp> ramps;
	//Warning: sigmoids contains indexes to Model.sigmoids vector, BUT incremented by 1 (Proper using is Model.getSigmoids().at(sigmoids.at(i)-1))
	std::vector<std::size_t> sigmoids;
	std::vector<step> steps;
	//Warning: hills contains indexes to Model.hills vector, BUT incremented by 1 (Proper using is Model.getHills().at(hills.at(i)-1))
	std::vector<std::size_t> hills;

};


template <typename T>
std::vector< Summember<T> > Summember<T>::hillAbstraction(std::vector<ramp> ramps, std::size_t replacedIndex) {

    std::vector< Summember<T> > result;
//    std::cout << "removing hill function with index " << replacedIndex-1 << " and size before is " << hills.size() <<"\n";
    for(std::vector<std::size_t>::iterator it = hills.begin(); it != hills.end(); it++) {
        if(*it == replacedIndex) {
            hills.erase(it);
            break;
        }
    }
//    std::cout << "size after is " << hills.size() << "\n";

	for (size_t r = 0; r < ramps.size(); r++) {
        Summember<T> sum;
        sum.AddRamp(ramps.at(r));
        result.push_back(sum * (*this));
    }

    return result;
}


template <typename T>
std::vector< Summember<T> > Summember<T>::sigmoidAbstraction(std::vector<ramp> ramps, std::size_t replacedIndex) {

    std::vector< Summember<T> > result;
//    std::cout << "removing sigmoid with index " << replacedIndex-1 << " and size before is " << sigmoids.size() <<"\n";
    for(std::vector<std::size_t>::iterator it = sigmoids.begin(); it != sigmoids.end(); it++) {
        if(*it == replacedIndex) {
            sigmoids.erase(it);
            break;
        }
    }
//    std::cout << "size after is " << sigmoids.size() << "\n";

	for (size_t r = 0; r < ramps.size(); r++) {
        Summember<T> sum;
        sum.AddRamp(ramps.at(r));
        result.push_back(sum * (*this));
    }

    return result;
}


template <class U>
std::ostream& operator<<(std::ostream& out, const Summember<U>& sum) {
    out << sum.GetConstant();
    if(sum.hasParam()) {
        out << "*Param(" << sum.GetParam() - 1 << ")";
    }
    for(uint i = 0; i < sum.GetVars().size(); i++) {
        out << "*Var(" << sum.GetVars().at(i) - 1 << ")";
    }
    for(uint i = 0; i < sum.GetRamps().size(); i++) {
        out << "*" << sum.GetRamps().at(i);
    }
    for(uint i = 0; i < sum.GetSigmoids().size(); i++) {
        out << "*Sigm(" << sum.GetSigmoids().at(i) - 1 << ")";
    }
    for(uint i = 0; i < sum.GetSteps().size(); i++) {
        out << "*" << sum.GetSteps().at(i);
    }
    for(uint i = 0; i < sum.GetHills().size(); i++) {
        out << "*Hill(" << sum.GetHills().at(i) - 1 << ")";
    }

    return out;
}

template <typename T>
const Summember<T> Summember<T>::operator*(/*const*/ Summember<T> & s2)
{
	Summember<T> & s1 = *this;
	Summember<T>  s_final;

	typedef T value_type;

	value_type c1 = s1.GetConstant();
	value_type c2 = s2.GetConstant();

	s_final.AddConstant(c1*c2);

	if (s1.hasParam())
	{
		s_final.AddParam(s1.GetParam());
	}

	if (s2.hasParam())
	{
		s_final.AddParam(s2.GetParam());
	}


	std::vector<std::size_t> vars1 = s1.GetVars();
	for (typename std::vector<size_t>::iterator it = vars1.begin(); it != vars1.end(); it++) {
		s_final.AddVar(*it);
	}

	std::vector<std::size_t> vars2 = s2.GetVars();
	for (typename std::vector<size_t>::iterator it = vars2.begin(); it != vars2.end(); it++) {
		s_final.AddVar(*it);
	}

	std::vector<ramp> ramps1 = s1.GetRamps();
	for (typename std::vector<ramp>::iterator it = ramps1.begin(); it != ramps1.end(); it++) {
		s_final.AddRamp(*it);
	}

	std::vector<ramp> ramps2 = s2.GetRamps();
	for (typename std::vector<ramp>::iterator it = ramps2.begin(); it != ramps2.end(); it++) {
		s_final.AddRamp(*it);
	}

	std::vector<std::size_t> sigmoids1 = s1.GetSigmoids();
	for (typename std::vector<std::size_t>::iterator it = sigmoids1.begin(); it != sigmoids1.end(); it++) {
		s_final.AddSigmoid(*it);
	}

	std::vector<std::size_t> sigmoids2 = s2.GetSigmoids();
	for (typename std::vector<std::size_t>::iterator it = sigmoids2.begin(); it != sigmoids2.end(); it++) {
		s_final.AddSigmoid(*it);
	}
	
	std::vector<step> steps1 = s1.GetSteps();
	for (typename std::vector<step>::iterator it = steps1.begin(); it != steps1.end(); it++) {
		s_final.AddStep(*it);
	}

	std::vector<step> steps2 = s2.GetSteps();
	for (typename std::vector<step>::iterator it = steps2.begin(); it != steps2.end(); it++) {
		s_final.AddStep(*it);
	}
	
	std::vector<std::size_t> hills1 = s1.GetHills();
	for (typename std::vector<std::size_t>::iterator it = hills1.begin(); it != hills1.end(); it++) {
		s_final.AddHill(*it);
	}

	std::vector<std::size_t> hills2 = s2.GetHills();
	for (typename std::vector<std::size_t>::iterator it = hills2.begin(); it != hills2.end(); it++) {
		s_final.AddHill(*it);
	}

	return s_final;

}
