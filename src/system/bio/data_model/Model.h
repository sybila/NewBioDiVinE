#pragma once
#include <stdlib.h>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <cmath>
#include <time.h>
#include <climits>
#include <algorithm>

#include "Summember.h"


using namespace std;

using std::string;

template <typename T>
class Model
{
	typedef T value_type;

private:

	struct sigmoid
	{
		std::size_t dim;
		value_type k;
		value_type theta;
		value_type a;
		value_type b;
		std::size_t n;
		bool positive;
		bool isInverse;

		friend std::ostream& operator<<(std::ostream& out, const sigmoid& s) {
			if (!s.positive)
				out << "S(-)[";
			else
				out << "S(+)[";
			out << s.n << "](" << s.dim << "," << s.k << "," << s.theta << "," << s.a << "," << s.b << ")";
			return out;
			
		}
		
		std::vector<value_type> enumerateYPoints(std::vector<value_type> x)
		{
			std::vector<value_type> y;
			value_type tempA = a;
			value_type tempB = b;
			
			for (size_t i = 0; i < x.size(); i++) {
				y.push_back(tempA + (tempB - tempA)*((1 + tanh(k*(x.at(i) - theta)))*0.5));
			}
			
			return y;
		}

	};
	
	
	struct hill {
	    std::size_t dim;		//Warning: index of var_name from Model.h but indexing from 1 (not 0)
	    value_type theta;
	    value_type n;
	    value_type a;
	    value_type b;
	    bool positive;
	    
        hill() {}
		hill(std::size_t new_dim, value_type new_theta, value_type new_n, value_type new_a, value_type new_b, bool new_positive = true)
                    : dim(new_dim), theta(new_theta), n(new_n), a(new_a), b(new_b), positive(new_positive) {
        	if( (positive && a > b) || (!positive && a < b) ) {        		
        		a = new_b;
        		b = new_a;
        	}
        }
        hill(const hill& s) : dim(s.dim), theta(s.theta), n(s.n), a(s.a), b(s.b), positive(s.positive) {}


		value_type value(value_type value) const
		{
			value_type result = a + (b - a)*(std::pow(1.0,n) / (std::pow(1.0,n) + std::pow(theta/value,n))); 
			return result;
		}
		
		std::vector<value_type> enumerateYPoints(std::vector<value_type> x) {
			std::vector<value_type> y;
			for(size_t i = 0; i < x.size(); i++) {
				y.push_back(value(x.at(i)));
			}
			return y;
		}

		friend std::ostream& operator<<(std::ostream& out, const hill& s) {
		    if(s.positive)
                out << "Hill(-)(";
            else
                out << "Hill(+)(";
            out << s.dim << "," << s.theta << "," << s.n << "," << s.a << "," << s.b << ")";
            return out;
		}
	};

public:
	void AddVariable(string var);
	void AddParam(string param);
	void AddParamRange(value_type a, value_type b);
	void AddConstantName(string constant);
	void AddConstantValue(value_type constant);

	void AddSigmoid(std::size_t segments, std::size_t dim, value_type k, value_type theta,
		value_type a, value_type b, bool positive = 1, bool isInverse = 0);
	void AddSigmoid(sigmoid & new_sigmoid);
	
	void AddHill(std::size_t dim, value_type theta, value_type n, value_type a, value_type b, bool positive = 1);
	void AddHill(hill & copy);

	void AddEquation(std::vector<Summember<T> > summs);
	void AddEquationName(std::string var);

	void AddThresholdName(std::string var);
	void AddThresholdValue(std::string value);

	void AddVarPointsName(std::string var);
	void AddVarPointsFstValue(std::string value);
	void AddVarPointsSndValue(std::string value);

	void AddInitsName(std::string var);
	void AddInitsFstValue(std::string value);
	void AddInitsSndValue(std::string value);

	void AddBaLine(std::string);

	std::size_t GetSigmoidsSize();

    const std::vector<std::string> getVariables() const;
	const std::string getVariable(int index) const;
	const size_t getVariableIndex(std::string var) const;
	const size_t getDims() const { return var_names.size(); }

    const std::vector<std::string> getParamNames() const;
	const std::string getParamName(int index) const;
	const std::vector<std::pair<value_type, value_type> > getParamRanges() const;
	const std::pair<value_type, value_type> getParamRange(int index) const;

	const std::vector<std::pair<std::string, value_type> > getConstants() const;
	const std::pair<std::string, value_type> getConstant(int index) const;

	const std::vector<std::pair<std::size_t, std::vector<Summember<T> > > > getEquations() const;
	const std::vector<Summember<T> > getEquationForVariable(size_t varIndex) const;
	const Summember<T> getSumForVarByIndex(size_t varIndex, size_t i) const;

	const std::vector<std::pair<std::size_t, std::vector<value_type> > > getThresholds() const;
	const std::vector<value_type> getThresholdsForVariable(size_t varIndex) const;
	const value_type getThresholdForVarByIndex(size_t varIndex, size_t i) const;

	const std::vector<std::pair<value_type, value_type> > getVarPointsValues() const;
	const std::vector<std::string> getVarPointsNames() const;

	const std::vector<std::pair<value_type, value_type> > getInitsValues() const;
	const std::vector<std::string> getInitsNames() const;
	const std::pair<value_type, value_type> getInitsValuesForVariable(size_t varIndex) const;

	const std::vector<std::string> getBaLines() const;

	const std::vector<sigmoid> getSigmoids() const;
	const std::vector<hill> getHills() const;

	void RunAbstraction(bool useFastApproximation = true);

private:
	std::vector<std::size_t> FindSigmoids(std::size_t dim);
	std::vector<std::size_t> FindHills(std::size_t dim);

	std::vector<sigmoid> sigmoids;
	std::vector<hill> hills;
	std::vector<std::string> var_names;  //variables
	std::vector<std::string> param_names;  //params
	std::vector<std::pair<value_type, value_type> > param_ranges;  //params
	std::vector<std::pair<std::string, value_type> > constants;  //constants
	//std::vector<std::pair<std::size_t, Entite<T> > > equations;
	std::vector<std::pair<std::size_t, std::vector<Summember<T> > > > equations;
	std::vector<std::pair<std::size_t, std::vector<value_type> > > thresholds;
	std::vector<std::size_t> var_points;
	std::vector<std::pair<value_type, value_type> > var_points_values;
	std::vector<std::size_t> inits;
	std::vector<std::pair<value_type, value_type> > inits_values;
	std::vector<std::string> ba_lines;


    std::vector<double> computeThresholds(std::vector<std::size_t> s, std::vector<std::size_t> hfs, int numOfSegments, int numOfX = 0, bool fast = true);
    std::vector<std::vector<double> > generateSpace(std::vector<std::size_t> s, std::vector<std::size_t> hfs, std::vector<double> &x, int numOfX);
    std::vector<double> generateXPoints (double ai, double bi, int num_segments);
    std::vector<double> segmentErr (std::vector <double> x, std::vector <double> y);
    std::vector<double> optimalGlobalLinearApproximation(std::vector<double> x, std::vector<std::vector<double> > y, int n_segments);
    vector <double> optimalFastGlobalLinearApproximation2 (vector <double> x, vector < vector <double> > y, int n_segments);
    std::vector<std::vector<typename Summember<value_type>::ramp> > generateNewRamps(std::vector<double> x, std::vector< std::vector<double> > y, std::size_t dim);
};


template <typename T>
void Model<T>::AddBaLine(std::string ba) {
    ba_lines.push_back(ba);
}

template <typename T>
void Model<T>::AddInitsName(std::string var) {
    size_t indexOfVar = -1;
    for (size_t i = 0; i < var_names.size(); i++) {
        if (var_names.at(i).compare(var) == 0 ) {
            indexOfVar = i;
            break;
        }
    }
    if(indexOfVar >= 0 && indexOfVar < var_names.size())
        inits.push_back(indexOfVar);
}

template <typename T>
void Model<T>::AddInitsFstValue(std::string value) {
    std::pair<T, T> values;
    values.first = (T)std::stod(value);
    inits_values.push_back(values);
}

template <typename T>
void Model<T>::AddInitsSndValue(std::string value) {
    inits_values.back().second = (T)std::stod(value);
}

template <typename T>
void Model<T>::AddVarPointsName(std::string var) {
    size_t indexOfVar = -1;
    for (size_t i = 0; i < var_names.size(); i++) {
        if (var_names.at(i).compare(var) == 0 ) {
            indexOfVar = i;
            break;
        }
    }
    if(indexOfVar >= 0 && indexOfVar < var_names.size())
        var_points.push_back(indexOfVar);
}

template <typename T>
void Model<T>::AddVarPointsFstValue(std::string value) {
    std::pair<T, T> values;
    values.first = (T)std::stod(value);
    var_points_values.push_back(values);
}

template <typename T>
void Model<T>::AddVarPointsSndValue(std::string value) {
    var_points_values.back().second = (T)std::stod(value);
}

template <typename T>
void Model<T>::AddThresholdName(std::string var) {

	// Finding for index of given variable
    int indexOfVar = -1;
    for (size_t i = 0; i < var_names.size(); i++) {
        if (var_names.at(i).compare(var) == 0 ) {
            indexOfVar = i;
            break;
        }
    }
    
    // If given variable could not be found
    if(indexOfVar == -1) {
    	std::cerr << "ERROR: Wrong given variable " << var << "\n";
    	return;
	}

	// Finding previous thresholds for given variable
    size_t i = 0;
    for( ; i < thresholds.size(); i++) {
        if(thresholds.at(i).first == indexOfVar) {
            if(i < thresholds.size() - 1) {
                swap(thresholds.at(i), thresholds.at(thresholds.size() - 1));
            }
            i = 0;
            break;
        }
    }
    
    // No previous thresholds were found - variable need new record of thresholds
    if(i == thresholds.size()) {
        std::pair<std::size_t, std::vector<T> > p;
        std::vector<T> th;
        p.first = indexOfVar;
        p.second = th;
        thresholds.push_back(p);
    }
}

template <typename T>
void Model<T>::AddThresholdValue(std::string value) {

	value_type val = (T)std::stod(value);
	
    if(!thresholds.empty()) {
    	if(std::find(thresholds.back().second.begin(),thresholds.back().second.end(),val) == thresholds.back().second.end()) {
		    thresholds.back().second.push_back(val);
		    std::sort(thresholds.back().second.begin(),thresholds.back().second.end());
	    }
    }
}

template <typename T>
void Model<T>::AddEquationName(std::string var) {
    std::pair<std::size_t, std::vector<Summember<T> > > eq;

    for (int i = 0; i != var_names.size(); i++) {
        if (var_names.at(i).compare(var) == 0 ) {
            //std::cout << "I have found var for eq:" << var_names.at(i) << ";\n";
            eq.first = i;
        }
    }
    //TODO: chcelo by to kontrolu ci sa nejaka premenna vobec nasla

    equations.push_back(eq);
}

template <typename T>
void Model<T>::AddEquation(std::vector<Summember<T> > summs) {
    if(!equations.empty()) {
        //std::cout << "I'm saving summs\n";
        equations.back().second = summs;
    }
}


template <typename T>
void Model<T>::AddVariable(string var)
{
	var_names.push_back(var);
}

template <typename T>
void Model<T>::AddParam(string param)
{
	param_names.push_back(param);
}

template <typename T>
void Model<T>::AddParamRange(value_type a, value_type b)
{
	std::pair<value_type, value_type> p;
	p.first = a;
	p.second = b;

	param_ranges.push_back(p);
}

template <typename T>
void Model<T>::AddConstantName(string constant)
{
	std::pair<std::string, value_type> p;
	p.first = constant;
	constants.push_back(p);
}

template <typename T>
void Model<T>::AddConstantValue(value_type constant)
{
	if (constants.empty())
	{
		//ERROR /*TODO*/
	}

	std::pair<std::string, value_type> & p = constants.back();
	p.second = constant;
}

template <typename T>
void Model<T>::AddHill(std::size_t dim, value_type theta, value_type n, value_type a, value_type b, bool positive) {
	hill h;
	h.dim = dim;
	h.theta = theta;
	h.n = n;
	h.a = a;
	h.b = b;
	hills.push_back(h);
}

template <typename T>
void Model<T>::AddHill(hill & copy) {
	hills.push_back(copy);
}

template <typename T>
void Model<T>::AddSigmoid(std::size_t segments, std::size_t dim, value_type k, value_type theta,
	value_type a, value_type b, bool positive, bool isInverse)
{
	sigmoid s;
	s.n = segments;
	s.dim = dim;
	s.k = k;
	s.theta = theta;
	s.positive = positive;
	s.isInverse = isInverse;
	s.a = a;
	s.b = b;
	sigmoids.push_back(s);
}

template <typename T>
void Model<T>::AddSigmoid(sigmoid & new_sigmoid)
{
	sigmoids.push_back(new_sigmoid);
}

template <typename T>
std::size_t Model<T>::GetSigmoidsSize()
{
	return sigmoids.size();
}


template <typename T>
const std::vector<std::string> Model<T>::getVariables() const {
	return var_names;
}

template <typename T>
const std::string Model<T>::getVariable(int index) const {
	return var_names.at(index);
}

template <typename T>
const size_t Model<T>::getVariableIndex(std::string var) const {
	for(size_t i = 0; i < var_names.size(); i++) {
		if(var_names.at(i).compare(var) == 0)
			return i;
	}
	return UINT_MAX;
}

template <typename T>
const std::vector<std::string> Model<T>::getParamNames() const {
    return param_names;
}

template <typename T>
const std::string Model<T>::getParamName(int index) const {
    return param_names.at(index);
}

template <typename T>
const std::vector<std::pair<T, T> > Model<T>::getParamRanges() const {
    return param_ranges;
}

template <typename T>
const std::pair<T, T> Model<T>::getParamRange(int index) const {
    return param_ranges.at(index);
}

template <typename T>
const std::vector<std::pair<std::string, T> > Model<T>::getConstants() const {
    return constants;
}

template <typename T>
const std::pair<std::string, T> Model<T>::getConstant(int index) const {
    return constants.at(index);
}

template <typename T>
const std::vector<std::pair<std::size_t, typename std::vector<Summember<T> > > > Model<T>::getEquations() const {
    return equations;
}

template <typename T>
const std::vector<Summember<T> > Model<T>::getEquationForVariable(size_t varIndex) const {
	for(size_t i = 0; i < equations.size(); i++) {
		if(varIndex == equations.at(i).first) {
			return equations.at(i).second;
		}
	}
	//TODO: case of unexisting var or wrong index
	return std::vector<Summember<T> >();
}

template <typename T>
const Summember<T> Model<T>::getSumForVarByIndex(size_t varIndex, size_t i) const {
	return getEquationForVariable(varIndex).at(i);
}

template <typename T>
const std::vector<std::pair<std::size_t, std::vector<T> > > Model<T>::getThresholds() const {
    return thresholds;
}

template <typename T>
const std::vector<T> Model<T>::getThresholdsForVariable(size_t varIndex) const {
	for(size_t i = 0; i < thresholds.size(); i++) {
		if(varIndex == thresholds.at(i).first)
			return thresholds.at(i).second;
	}
}

template <typename T>
const T Model<T>::getThresholdForVarByIndex(size_t varIndex, size_t i) const {
	return getThresholdsForVariable(varIndex).at(i);
}

template <typename T>
const std::vector<std::pair<T, T> > Model<T>::getVarPointsValues() const {
    return var_points_values;
}

template <typename T>
const std::vector<std::string> Model<T>::getVarPointsNames() const {
    std::vector<std::string> vpn;
    for(int i = 0; i < var_points.size(); i++) {
        vpn.push_back(var_names.at(var_points.at(i)));
    }
    return vpn;
}

template <typename T>
const std::vector<std::pair<T, T> > Model<T>::getInitsValues() const {
    return inits_values;
}

template <typename T>
const std::pair<T, T> Model<T>::getInitsValuesForVariable(size_t varIndex) const {
    for(int i = 0; i < inits.size(); i++) {
		if(varIndex == inits.at(i)) {
			return inits_values.at(i);
		}
	}
	std::cerr << "ERROR: Wrong index of variable\n";
	return std::pair<T,T>();
}

template <typename T>
const std::vector<std::string> Model<T>::getInitsNames() const {
    std::vector<std::string> vpn;
    for(int i = 0; i < inits.size(); i++) {
        vpn.push_back(var_names.at(inits.at(i)));
    }
    return vpn;
}

template <typename T>
const std::vector<std::string> Model<T>::getBaLines() const {
    return ba_lines;
}

template <typename T>
const std::vector<typename Model<T>::sigmoid> Model<T>::getSigmoids() const {
    return sigmoids;
}

template <typename T>
const std::vector<typename Model<T>::hill> Model<T>::getHills() const {
	return hills;
}


template <typename T>
std::vector<std::size_t> Model<T>::FindHills(std::size_t dim) {
	std::vector<std::size_t> result;
	
	for (uint i = 0; i < hills.size(); i++) {
		if(hills.at(i).dim == dim + 1)
			result.push_back(i);
	}
	return result;
}

template <typename T>
std::vector<std::size_t> Model<T>::FindSigmoids(std::size_t dim) {
	std::vector<std::size_t> result;

	for (uint i = 0; i < sigmoids.size(); i++)
	{
		if (sigmoids.at(i).dim == dim + 1)
			result.push_back(i);
	}
	return result;
}

template <typename T>
void Model<T>::RunAbstraction(bool useFastApproximation)
{
    // dostane alebo si zoberie priamo ako class method vector so vsetkymi sigmoidami a rozdeli ich podla Vars do skupin
    // postupne vsetky skupiny sigmoidov posle funkcii comuteThresholds, ktore zaroven ulozi medzi ostatne thresholdy
    // nasledne podla tychto novych thresholdov vytvori rampy pre vsetky sigmoidy zo skupiny a ulozi medzi rampy
    // nakoniec mozno zmaze sigmoidy ak uz nebudu treba

	std::vector<std::vector<typename Summember<T>::ramp> > new_sigmoids_ramps;
	std::vector<std::vector<typename Summember<T>::ramp> > new_hills_ramps;	
	unsigned int curveNum = 0;

    for(size_t i = 0; i < var_names.size(); i++) {

        std::vector<std::size_t> groupOfSigmoids = FindSigmoids(i);
        std::cout << "For var of index " << i << " has been found " << groupOfSigmoids.size() << " sigmoids\n";
        
        std::vector<std::size_t> groupOfHills = FindHills(i);
        std::cout << "For var of index " << i << " has been found " << groupOfHills.size() << " hill functions\n";        

        if(groupOfSigmoids.empty() && groupOfHills.empty())
            continue;       // no need for abstraction for this variable

        int numOfSegments = 5;
        int numOfXPoints = 0;

        for(size_t j = 0; j < var_points.size(); j++) {
            if(i == var_points.at(j)) {
				// check if default number of requested segments is enough against number from input file
                if(numOfSegments < var_points_values.at(j).second)
                    numOfSegments = var_points_values.at(j).second;

				// check if default number of requested x-points is enough against number from input file
                if(numOfXPoints < var_points_values.at(j).first)
                    numOfXPoints = var_points_values.at(j).first;
            }
        }

        std::vector<double> thresholdsX = computeThresholds(groupOfSigmoids,groupOfHills,numOfSegments,numOfXPoints,useFastApproximation);

    // New generated thresholds from computeThresholds() are storing to the previous ones here
		std::cout << "New threses for var " << getVariable(i) << ": ";	//just for testing
        AddThresholdName(getVariable(i));
        for(size_t t = 0; t < thresholdsX.size(); t++) {
            std::stringstream ss;
            ss << thresholdsX.at(t);
            AddThresholdValue(ss.str());
            std::cout << ss.str() << ", ";		//Just for testing
        }
        std::cout << "\n";		//just for testing

        
        // Testing-----------------------------------------------------
        std::cout << "All threses for var " << getVariable(i) << ": ";
        for(size_t t = 0; t < getThresholdsForVariable(i).size(); t++) {
        	std::cout << getThresholdForVarByIndex(i,t) << ", ";
        }
        std::cout << "\n";
		// End of testing----------------------------------------------


        std::vector<std::vector<double> > thresholdsY;

        for(size_t j = 0; j < groupOfSigmoids.size(); j++) {

            thresholdsY.push_back(sigmoids.at(groupOfSigmoids.at(j)).enumerateYPoints(thresholdsX));

            // Testing functionality
			/*
            std::stringstream ss;
            ss << curveNum++;
            std::string fileName = "nezmysel";
            if(useFastApproximation)
            	fileName = "test_curve" + ss.str() + ".fast.dat";
            else
            	fileName = "test_curve" + ss.str() + ".slow.dat";
			
            std::ofstream out(fileName,std::ofstream::out | std::ofstream::trunc);
            if(out.is_open()) {
				for (size_t sp = 0; sp < thresholdsX.size(); sp++) {
                    out << thresholdsX.at(sp) << "\t" << thresholdsY.back().at(sp) << "\n";
                }
            }
            out.close();
			*/
            // End of testing

        }
        new_sigmoids_ramps = generateNewRamps(thresholdsX, thresholdsY, i+1);
        
        thresholdsY.clear();
		for (size_t j = 0; j < groupOfHills.size(); j++) {
            thresholdsY.push_back(hills.at(groupOfHills.at(j)).enumerateYPoints(thresholdsX));

            // Testing functionality
			/*
            std::stringstream ss;
            ss << curveNum++;
            std::string fileName = "nezmysel";
            if(useFastApproximation)
            	fileName = "test_curve" + ss.str() + ".fast.dat";
            else
            	fileName = "test_curve" + ss.str() + ".slow.dat";
            std::ofstream out(fileName,std::ofstream::out | std::ofstream::trunc);
            if(out.is_open()) {
				for (size_t sp = 0; sp < thresholdsX.size(); sp++) {
                    out << thresholdsX.at(sp) << "\t" << thresholdsY.back().at(sp) << "\n";
                }
            }
            out.close();
			*/
            // End of testing

        }
        new_hills_ramps = generateNewRamps(thresholdsX, thresholdsY, i+1);

        // For testing----------------------------------------------------
        if(groupOfSigmoids.size() > 0) {
		    std::cout << "----------new sigmoids ramps----------\n";
			for (size_t vr = 0; vr < new_sigmoids_ramps.size(); vr++) {
		        std::cout << "----new sigmoid----\n";
				for (size_t r = 0; r < new_sigmoids_ramps.at(vr).size(); r++) {
		            std::cout << new_sigmoids_ramps.at(vr).at(r) << std::endl;
		        }
		    }
		    std::cout << "------end of new sigmoids ramps-------\n";
   		}
        
        // For testing-------------------------------------------------------
        if(groupOfHills.size() > 0) {
		    std::cout << "----------new hills ramps----------\n";
			for (size_t vr = 0; vr < new_hills_ramps.size(); vr++) {
		        std::cout << "----new hill----\n";
				for (size_t r = 0; r < new_hills_ramps.at(vr).size(); r++) {
		            std::cout << new_hills_ramps.at(vr).at(r) << std::endl;
		        }
		    }
		    std::cout << "------end of new hills ramps-------\n";
	    }

        // for all equations
		for (size_t j = 0; j < equations.size(); j++) {
            std::cout << "in EQ " << j << "\n";
            int summsCounter = 0;
            // for all summembers in one equation
            for(typename std::vector<Summember<T> >::iterator sit = equations.at(j).second.begin(); sit != equations.at(j).second.end(); sit++) {
            //for(int s = 0; s < equations.at(j).second.size(); s++) {
                std::cout << "in Summember:" << summsCounter++ << "\n";
                Summember<T> summ = *sit;
                //Summember<T> summ = equations.at(j).second.at(s);

                // for all sigmoids in one summember
				for (size_t g = 0; g < summ.GetSigmoids().size(); g++) {
                    std::cout << "in sigmoid " << g << "\n";
                    std::size_t index = (summ.GetSigmoids().at(g)) - 1;

                    // for all sigmoids found for one variable
					for (size_t v = 0; v < groupOfSigmoids.size(); v++) {
                        if(index == groupOfSigmoids.at(v)) {
                            std::cout << "TERAZ TREBA NAHRADIT SIGMOID " << v << " RAMPAMI\n";
                            std::vector<Summember<T> > newSummembers = summ.sigmoidAbstraction(new_sigmoids_ramps.at(v), index+1);
                            std::cout << "pocet new summs:" << newSummembers.size() << "\n";

                            std::cout << "pocty summs:" << equations.at(j).second.size() << ",";
                            typename std::vector<Summember<T> >::iterator newSit;
                            newSit = equations.at(j).second.erase(sit);
                            std::cout << equations.at(j).second.size() << ",";

                            typename std::vector<Summember<T> >::iterator replacingSit;
                            for(replacingSit = newSummembers.begin(); replacingSit != newSummembers.end(); replacingSit++) {
                                sit = equations.at(j).second.insert(newSit, *replacingSit);
                                newSit = sit;
                            }
                            //sit = equations.at(j).second.insert(newSit, newSummembers.begin(), newSummembers.end());
                            std::cout << equations.at(j).second.size() << "\n";
                            summ = *sit;
                            g = -1;
                            summsCounter = 0;
                            std::cout << "USPESNE NAHRADENE\n";

                            break;
                      // TODO: ak sa rovnaju treba rampami v new_sigmoids_ramps.at(v) nahradit najdeny vyskyt sigmoidu sigmoids.at(index)
                            // v j-tej rovnici a v s-tom summembre tak ze sa tento summember odstrani a miesto neho vzniknu uplne
                            // nove summembre (jeden pre kazdu rampu z new_sigmoids_ramps.at(v)) so vsetkymi ostatnymi clenmi zachovanymi
                            // Po najdeni by teoreticky mohol nasledovat masivny break az k prvemu for cyklu ale neviem isto,
                            // ci kazdy sigmoid musi byt unikatny
                            // TREBA DAVAT POZOR PRI VKLADANI NOVYCH SUMMEMBROV - najlepsie bude ulozit do docasnej struktury
                            // a po spravnom cykle ich ulozit
                        }
                    }
                }
                
                // for all hill functions in one summember
				for (size_t g = 0; g < summ.GetHills().size(); g++) {
                    std::cout << "in hill function " << g << "\n";
                    std::size_t index = (summ.GetHills().at(g)) - 1;

                    // for all hill functions found for one variable
					for (size_t v = 0; v < groupOfHills.size(); v++) {
                        if(index == groupOfHills.at(v)) {
                            std::cout << "TERAZ TREBA NAHRADIT HILLOVU FUNKCIU " << v << " RAMPAMI\n";
                            std::vector<Summember<T> > newSummembers = summ.hillAbstraction(new_hills_ramps.at(v), index+1);
                            std::cout << "pocet new summs:" << newSummembers.size() << "\n";

                            std::cout << "pocty summs:" << equations.at(j).second.size() << ",";
                            typename std::vector<Summember<T> >::iterator newSit;
                            newSit = equations.at(j).second.erase(sit);
                            std::cout << equations.at(j).second.size() << ",";

                            typename std::vector<Summember<T> >::iterator replacingSit;
                            for(replacingSit = newSummembers.begin(); replacingSit != newSummembers.end(); replacingSit++) {
                                sit = equations.at(j).second.insert(newSit, *replacingSit);
                                newSit = sit;
                            }
                            //sit = equations.at(j).second.insert(newSit, newSummembers.begin(), newSummembers.end());
                            std::cout << equations.at(j).second.size() << "\n";
                            summ = *sit;
                            g = -1;
                            summsCounter = 0;
                            std::cout << "USPESNE NAHRADENE\n";

                            break;
                        }
                    }
                }
            }
        }
    }
}

template <typename T>
std::vector<std::vector<typename Summember<T>::ramp> > Model<T>::generateNewRamps(std::vector<double> x, std::vector< std::vector<double> > y, std::size_t dim) {
    std::vector<std::vector<typename Summember<T>::ramp> > result;

    for(size_t i = 0; i < y.size(); i++) {

        std::vector<typename Summember<T>::ramp> ramps;

        for(size_t t = 0; t < x.size() - 1; t++) {
            double x1 = x.at(t);
            double x2 = x.at(t+1);
            double y1 = y.at(i).at(t);
            double y2 = y.at(i).at(t+1);

            if(y1 <= y2)
                ramps.push_back(typename Summember<T>::ramp::ramp(dim, x1, x2, y1, y2));
            else
                ramps.push_back(typename Summember<T>::ramp::ramp(dim, x1, x2, y1, y2, true));
        }

        result.push_back(ramps);
    }

    return result;
}

template <typename T>
std::vector<double> Model<T>::computeThresholds(std::vector<std::size_t> s, std::vector<std::size_t> hfs, int numOfSegments, int numOfX, bool fast) {

    //TODO: treba sa dohodnut ohladom obsahu hranatych zatvoriek u jednotlivych sigmoidov
	for (size_t i = 0; i < s.size(); i++) {
        if(numOfSegments < sigmoids.at(s.at(i)).n) {
            numOfSegments = sigmoids.at(s.at(i)).n;
        }
    }

    std::cout << "NUMBER OF SEGMENTS = " << numOfSegments << std::endl;

    std::vector<double> xPoints;
    std::vector<std::vector<double> > curves = generateSpace(s,hfs,xPoints,numOfX);
    std::vector<double> segmentsPoints;

    std::cout << "xPoints.size() = " << xPoints.size() << std::endl;
    std::cout << "curves.size() = " << curves.size() << std::endl;
	for (size_t i = 0; i < curves.size(); i++) {
    	std::cout << "curve " << i << " size: " << curves.at(i).size() << std::endl;
    }

    clock_t start, finish;          //TODO: neskor zmazat
    double durationInSec;           //TODO: neskor zmazat


	if(fast) {
	
		start = clock();                //TODO: neskor zmazat
		std::cout << "before fast approximation...\n";
		segmentsPoints = optimalFastGlobalLinearApproximation2 (xPoints, curves, numOfSegments);
		std::cout << "after fast approximation...\n";

		finish = clock();               //TODO: neskor zmazat
		durationInSec = (double)(finish - start) / CLOCKS_PER_SEC;          //TODO: neskor zmazat
		std::cout << "duration = " << durationInSec << " sec. Found = ";          //TODO: neskor zmazat
		for (size_t i = 0; i < segmentsPoints.size(); i++)
		    std::cout << segmentsPoints.at(i) << ",";
		std::cout << std::endl;
		
	} else {

		start = clock();                //TODO: neskor zmazat

		std::cout << "before slow approximation...\n";
		segmentsPoints = optimalGlobalLinearApproximation (xPoints, curves, numOfSegments);
		std::cout << "after slow approximation...\n";

		finish = clock();               //TODO: neskor zmazat
		durationInSec = (double)(finish - start) / CLOCKS_PER_SEC;      //TODO: neskor zmazat
		std::cout << "duration = " << durationInSec << " sec. Found = ";     //TODO: neskor zmazat
		for (size_t i = 0; i < segmentsPoints.size(); i++)                  //TODO: neskor zmazat
		    std::cout << segmentsPoints.at(i) << ",";                        //TODO: neskor zmazat
		std::cout << std::endl;                                                   //TODO: neskor zmazat
		
	}

    return segmentsPoints;
}


template <typename T>
std::vector <double>  Model<T>::generateXPoints (double ai, double bi, int num_segments)
{
	   double a = ai;
	   double b = bi;

	   std::vector <double > x;

	   double dx = (b-a)/(num_segments-1);

	   for (int i = 0; i < num_segments; i++)
	   {
		   double cx = a + dx*i;
		   x.push_back(cx);
	   }

	   return x;
}


template <typename T>
std::vector<std::vector<double> > Model<T>::generateSpace(std::vector<std::size_t> s, std::vector<std::size_t> hfs, std::vector<double> &x, int numOfX) {

    std::vector<std::vector<double> > y;

    if(s.size() > 0 || hfs.size() > 0) {

        double intervalDeviationParam = 1.5;

        double min = 0.0;
        double max = 0.0;
        if(s.size() > 0) {
        	min =  sigmoids.at(s.at(0)).theta - (2.0 / sigmoids.at(s.at(0)).k) * intervalDeviationParam;
        	max = sigmoids.at(s.at(0)).theta + (2.0 / sigmoids.at(s.at(0)).k) * intervalDeviationParam;
        	
		    if(s.size() > 1) {

		        for(size_t i = 1; i < s.size(); i++) {

		            double tempMin = sigmoids.at(s.at(i)).theta - (2.0 / sigmoids.at(s.at(i)).k) * intervalDeviationParam;
		            double tempMax = sigmoids.at(s.at(i)).theta + (2.0 / sigmoids.at(s.at(i)).k) * intervalDeviationParam;

		            if(tempMin < min) {
		                min = tempMin;
		            }
		            if(tempMax > max) {
		                max = tempMax;
		            }
		        }
		    }
        }
        
        if(hfs.size() > 0) {
        
        	min = 0.0;
        	for (size_t i = 0; i < hfs.size(); i++) {
        	
        		value_type tempMax = 2.0*hills.at(hfs.at(i)).theta + (5.0/hills.at(hfs.at(i)).n)*hills.at(hfs.at(i)).theta;
        		if(max < tempMax)
        			max = tempMax;
        	}
        }

        if(min < 0.0)
            min = 0.0;

        if(numOfX != 0) {
            x = generateXPoints(min, max, numOfX);
        } else {
            x = generateXPoints(min, max, (max - min)*100);
        }

        for(size_t i = 0; i < s.size(); i++) {
            y.push_back(sigmoids.at(s.at(i)).enumerateYPoints(x));
        }
        
        for(size_t i = 0; i < hfs.size(); i++) {
        	y.push_back(hills.at(hfs.at(i)).enumerateYPoints(x));
        }
    }

    return y;
}


template <typename T>
std::vector <double> Model<T>::segmentErr (std::vector <double> x, std::vector <double> y)
{
       // Find out size of x
       int nx = x.size();
       int ny = y.size();

       std::vector<double> result (3, 0.0);

       if (nx != ny) {
           std::cout << "Error in segmentErr: nx is not consistent with ny\n";
           return result;
       }

       // Compute line segment coefficients
       double a = (y[nx-1] - y[0]) / (x[nx-1] - x[0]);
       double b = (y[0] * x[nx-1] - y[nx-1] * x[0]) / (x[nx-1] - x[0]);

       // Compute error for above line segment
       double e = 0;

	   for (int k = 0; k < nx; k++) {
		    e += pow((y[k] - a * x[k] - b),2);
	   }
	   e /= (pow(a,2) + 1);

       result[0] = e;
       result[1] = a;
       result[2] = b;

       return result;
}


template <typename T>
std::vector<double> Model<T>::optimalGlobalLinearApproximation(std::vector<double> x, std::vector<std::vector<double> > y,
                                                               int n_segments)
{
       int n_points = x.size();
       std::cout << "x.size() = " << n_points << std::endl;
       int n_curves = y.size();
       std::cout << "y.size() = " << n_curves << std::endl;

       std::vector<std::vector<double> > mCost(n_points, std::vector<double>(n_segments, INFINITY));
       std::vector<std::vector<double> > hCst(n_points, std::vector<double>(n_points, INFINITY));

       mCost[1][0] = 0.0;

       std::vector<std::vector<int> > father(n_points, std::vector<int>(n_segments, 0));



       for (int n = 1; n < n_points; n++) {
       
            double temp = -1 * INFINITY;
            for (int ic = 0; ic < n_curves; ic++) {
            
                 std::vector<double> v1 (x.begin(), x.begin() + n+1);
                 std::vector<double> v2 (y[ic].begin(), y[ic].begin() + n+1);
                 std::vector<double> seg_err = segmentErr(v1, v2);

                 temp = std::max(seg_err[0], temp);
            }

            mCost[n][0] = temp;
            father[n][0] = 0;
            //std::cout << " n=" << n << " mCost[" << n << "][0]=" << temp << " father[" << n << "][0]=" << father[n][0] << "\n";
       }

       double minErr, currErr;
       int minIndex;

       for (int m = 1; m < n_segments; m++) {
            std::cout << "segment m=" << m << "\n";
            
            for (int n = 2; n < n_points; n++) {

                minErr = mCost[n-1][m-1];
                minIndex = n - 1;

                for (int i = m; i <= n-2; i++) {
                
                    if (hCst[i][n]==INFINITY) {
                        double temp = -1 * INFINITY;

                        for (int ic = 0; ic < n_curves; ic++) {
                             std::vector<double> v1 (x.begin() + i, x.begin() + n+1);
                             std::vector<double> v2 (y[ic].begin() + i, y[ic].begin() + n+1);
                             std::vector<double> seg_err = segmentErr(v1, v2);

                             temp = std::max(seg_err[0], temp);
                        }

                        hCst[i][n] = temp;
                    }

                    currErr = mCost[i][m-1] + hCst[i][n];

                    if (currErr < minErr) {
                        minErr = currErr;
                        minIndex = i;
                    }
                }
               mCost[n][m]  = minErr;
               father[n][m] = minIndex;
         //  std::cout << " n=" << n << " mCost[" << n << "][" << m << "]=" << mCost[n][m] << " father[" << n << "][" << m << "]=" << father[n][m] << "\n";
        }
    }

    std::vector<int> ib (n_segments+1, 0);
    std::vector<double> xb (n_segments+1, 0.0);

    ib[n_segments] = n_points-1;
    xb[n_segments] = x[ib[n_segments]];
    
    for (int i = n_segments-1; i >= 0; i--) {
         ib[i] = father[ib[i+1]][i];
         xb[i] = x[ib[i]];
    }

    for (int i = 0; i < n_segments + 1; i++) {
        std::cout << "x[" << i << "] =" << xb[i] << "\n";
    }

    return xb;
}

template <typename T>
vector <double> Model<T>::optimalFastGlobalLinearApproximation2 (vector <double> x, vector < vector <double> > y, int n_segments)
{
	
	int n_points = x.size();
	int n_curves = y.size();
	
	vector <vector <double> > mCost (n_points, vector <double>(n_segments, INFINITY));
	vector <vector <double> > hCst  (n_points, vector <double>(n_points, INFINITY));
	
	mCost[1][0] = 0.0;
	
	vector <vector <int> > father (n_points, vector <int> (n_segments, 0));
	
	
	for (int n=1; n < n_points; n++) {
		double temp = -1 * INFINITY;
		
		//cout << "temp =" << temp << "\n";
		
		for (int ic=0; ic < n_curves; ic++) {
			vector <double> v1 (x.begin(), x.begin() + n+1);
			vector <double> v2 (y[ic].begin(), y[ic].begin() + n+1);    
			vector <double> seg_err = segmentErr(v1, v2);
			
			temp = max(seg_err[0], temp);
		} 
		
		mCost [n][0] = temp;
		father[n][0] = 0;
		//cout << " n=" << n << " mCost[" << n << "][0]=" << temp << " father[" << n << "][0]=" << father[n][0] << "\n";
	}
	
	
	vector < vector <double> > sy2 (n_curves, vector <double> (n_points, 0.0));
	vector < vector <double> > sy  (n_curves, vector <double> (n_points, 0.0));
	vector < vector <double> > sxy (n_curves, vector <double> (n_points, 0.0));
	
	vector <double> sx2 (n_points, 0);
	vector <double> sx (n_points, 0);
	
	for (int ic=0; ic < n_curves; ic++) {
		sy2[ic][0] = y[ic][0] * y[ic][0];
		sy [ic][0] = y[ic][0];
		sxy[ic][0] = y[ic][0] * x[0];
		
		for (int ip=1; ip < n_points; ip++) {
			sy2[ic][ip] = sy2[ic][ip-1] + (y[ic][ip] * y[ic][ip]);
			sy [ic][ip] = sy [ic][ip-1] + (y[ic][ip]);
			sxy[ic][ip] = sxy[ic][ip-1] + (y[ic][ip] * x[ip]);
		}
	}
	
	sx2[0] = x[0] * x[0];
	sx[0] = x[0];
	
	for (int ip=1; ip < n_points; ip++) {
	    sx2[ip] = sx2[ip-1] + (x[ip] * x[ip]);
	    sx [ip] = sx [ip-1]  + x[ip];
	}
    
	double minErr, currErr;
	int    minIndex;
    
	for (int m=1; m < n_segments; m++) {
	
		cout << "segment m=" << m << "\n";  
		for (int n=2; n < n_points; n++) {
			
			minErr = mCost[n-1][m-1];
			minIndex = n - 1;
			
			for (int i=m; i <= n-2; i++) {
				if (hCst[i][n]==INFINITY) {
				
					double temp = -1 * INFINITY;
					
					for (int ic=0; ic < n_curves; ic++)	{
						
						double a = (y[ic][n] - y[ic][0]) / (x[n] - x[0]);
						double b = (y[ic][0] * x[n] - y[ic][n] * x[0]) / (x[n] - x[0]);
						double seg_err = (sy2[ic][n] - sy2[ic][i-1]) - 2 * a * (sxy[ic][n] - sxy[ic][i-1]) - 2 * b * (sy[ic][n] - sy[ic][i-1]) + a * a * (sx2[n] - sx2[i-1]) + 2 * a * b * (sx[n] - sx[i-1]) + b * (n - i);
						
						temp = max(seg_err, temp);
					}
					
					hCst[i][n] = temp;
				}
				
				currErr = mCost[i][m-1] + hCst[i][n];
				
				if (currErr < minErr) {
					minErr   = currErr;
					minIndex = i;
				}
			} 
			mCost[n][m]  = minErr;
			father[n][m] = minIndex;
			//  cout << " n=" << n << " mCost[" << n << "][" << m << "]=" << mCost[n][m] << " father[" << n << "][" << m << "]=" << father[n][m] << "\n";
        }
    }         
	
    vector <int> ib (n_segments+1, 0  );
    vector <double> xb (n_segments+1, 0.0);	
	
    ib[n_segments] = n_points-1;
    xb[n_segments] = x[ib[n_segments]];
    cout << "x[ib[n_segments]]=" << x[ib[n_segments]] << "\n";
    
    for (int i=n_segments-1; i >= 0; i--) {
		ib[i] = father[ib[i+1]][i];
		xb[i] = x[ib[i]];
		// cout << "x" << n_points - i << "=" << xb[i] << "\n";
    }         
	
    for (int i=0; i < n_segments + 1; i++) {
        cout << "x[" << i << "] =" << xb[i] << "\n";
    }
	
    return xb;
}


